/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "BIFImporter.h"

#include "win32def.h"

#include "Compressor.h"
#include "GameData.h"
#include "Interface.h"
#include "System/SlicedStream.h"
#include "System/FileStream.h"

BIFImporter::FileEntry::FileEntry(ieDword Offset, ieDword Size)
	: offset(Offset), size(Size)
{
}

BIFImporter::TileEntry::TileEntry(ieDword Offset, ieDword Count, ieDword Size)
	: offset(Offset), count(Count), size(Size)
{
}

BIFImporter::BIFImporter()
	: stream(NULL)
{
}

BIFImporter::~BIFImporter()
{
	delete stream;
}

int BIFImporter::DecompressSaveGame(DataStream *compressed)
{
	char Signature[8];
	compressed->Read( Signature, 8 );
	if (strncmp( Signature, "SAV V1.0", 8 ) ) {
		return GEM_ERROR;
	}
	int All = compressed->Remains();
	int Current;
	if (!All) return GEM_ERROR;
	do {
		ieDword fnlen, complen, declen;
		compressed->ReadDword( &fnlen );
		if (!fnlen) {
			printMessage("BIFImporter", "Corrupt Save Detected\n", RED);
			return GEM_ERROR;
		}
		char* fname = ( char* ) malloc( fnlen );
		compressed->Read( fname, fnlen );
		strlwr(fname);
		compressed->ReadDword( &declen );
		compressed->ReadDword( &complen );
		print( "Decompressing %s\n", fname );
		DataStream* cached = gamedata->AddCompressedCacheFile(compressed, fname, complen, true);
		free( fname );
		if (!cached)
			return GEM_ERROR;
		delete cached;
		Current = compressed->Remains();
		//starting at 20% going up to 70%
		core->LoadProgress( 20+(All-Current)*50/All );
	}
	while(Current);
	return GEM_OK;
}

//this one can create .sav files only
int BIFImporter::CreateArchive(DataStream *compressed)
{
	delete stream;
	stream = NULL;

	if (!compressed) {
		return GEM_ERROR;
	}
	char Signature[8];

	memcpy(Signature,"SAV V1.0",8);
	compressed->Write(Signature, 8);

	return GEM_OK;
}

int BIFImporter::AddToSaveGame(DataStream *str, DataStream *uncompressed)
{
	ieDword fnlen, declen, complen;

	fnlen = strlen(uncompressed->filename)+1;
	declen = uncompressed->Size();
	str->WriteDword( &fnlen);
	str->Write( uncompressed->filename, fnlen);
	str->WriteDword( &declen);
	//baaah, we dump output right in the stream, we get the compressed length
	//only after the compressed data was written
	complen = 0xcdcdcdcd; //placeholder
	unsigned long Pos = str->GetPos(); //storing the stream position
	str->WriteDword( &complen);

	PluginHolder<Compressor> comp(PLUGIN_COMPRESSION_ZLIB);
	comp->Compress( str, uncompressed );

	//writing compressed length (calculated)
	unsigned long Pos2 = str->GetPos();
	complen = Pos2-Pos-sizeof(ieDword); //calculating the compressed stream size
	str->Seek(Pos, GEM_STREAM_START); //going back to the placeholder
	str->WriteDword( &complen);       //updating size
	str->Seek(Pos2, GEM_STREAM_START);//resuming work
	return GEM_OK;
}

int BIFImporter::OpenArchive(const char* filename)
{
	delete stream;
	stream = NULL;

	FileStream* file = FileStream::OpenFile(filename);
	if( !file) {
		return GEM_ERROR;
	}
	char Signature[8];
	if (file->Read(Signature, 8) == GEM_ERROR) {
		delete file;
		return GEM_ERROR;
	}
	delete file;
	//normal bif, not in cache
	if (strncmp( Signature, "BIFFV1  ", 8 ) == 0) {
		stream = gamedata->AddCacheFile( filename );
		if (!stream)
			return GEM_ERROR;
		stream->Read( Signature, 8 );
		strcpy( path, filename );
		ReadBIF();
		return GEM_OK;
	}
	//not found as normal bif
	//checking compression type
	FileStream* compressed = FileStream::OpenFile( filename );
	if (!compressed)
		return GEM_ERROR;
	compressed->Read( Signature, 8 );
	if (strncmp( Signature, "BIF V1.0", 8 ) == 0) {
		ieDword fnlen, complen, declen;
		compressed->ReadDword( &fnlen );
		char* fname = ( char* ) malloc( fnlen );
		compressed->Read( fname, fnlen );
		strlwr(fname);
		compressed->ReadDword( &declen );
		compressed->ReadDword( &complen );
		print( "Decompressing\n" );
		stream = gamedata->AddCompressedCacheFile(compressed, fname, complen);
		free( fname );
		delete compressed;
		if (!stream)
			return GEM_ERROR;
		stream->Read( Signature, 8 );
		if (strncmp( Signature, "BIFFV1  ", 8 ) == 0)
			ReadBIF();
		else
			return GEM_ERROR;
		return GEM_OK;
	}

	if (strncmp( Signature, "BIFCV1.0", 8 ) == 0) {
		//print("'BIFCV1.0' Compressed File Found\n");
		stream = gamedata->OpenCacheFile(compressed->filename);
		if (stream) {
			//print("Found in Cache\n");
			delete compressed;
			stream->Read( Signature, 8 );
			if (strncmp( Signature, "BIFFV1  ", 8 ) == 0) {
				ReadBIF();
			} else
				return GEM_ERROR;
			return GEM_OK;
		}
		print( "Decompressing\n" );
		if (!core->IsAvailable( PLUGIN_COMPRESSION_ZLIB ))
			return GEM_ERROR;
		PluginHolder<Compressor> comp(PLUGIN_COMPRESSION_ZLIB);
		ieDword unCompBifSize;
		compressed->ReadDword( &unCompBifSize );
		print( "\nDecompressing file: [..........]" );
		fflush(stdout);
		FileStream *out = gamedata->CreateCacheFile(compressed->filename);
		if (!out) {
			printMessage("BIFImporter", "Cannot write %s.\n", RED, path);
			return GEM_ERROR;
		}
		ieDword finalsize = 0;
		int laststep = 0;
		while (finalsize < unCompBifSize) {
			ieDword complen, declen;
			compressed->ReadDword( &declen );
			compressed->ReadDword( &complen );
			if (comp->Decompress( out, compressed, complen ) != GEM_OK) {
				return GEM_ERROR;
			}
			finalsize = out->GetPos();
			if (( int ) ( finalsize * ( 10.0 / unCompBifSize ) ) != laststep) {
				laststep++;
				print( "\b\b\b\b\b\b\b\b\b\b\b" );
				int l;

				for (l = 0; l < laststep; l++)
					print( "|" );
				for (; l < 10; l++)//l starts from laststep
					print( "." );
				print( "]" );
				fflush(stdout);
			}
		}
		print( "\n" );
		stream = gamedata->OpenCacheFile(compressed->filename);
		delete compressed;
		if (!stream)
			return GEM_ERROR;
		stream->Read( Signature, 8 );
		if (strncmp( Signature, "BIFFV1  ", 8 ) == 0)
			ReadBIF();
		else
			return GEM_ERROR;
		return GEM_OK;
	}
	delete compressed;
	return GEM_ERROR;
}

DataStream* BIFImporter::GetStream(unsigned long Resource, unsigned long Type)
{
	if (Type == IE_TIS_CLASS_ID) {
		TileEntryMap::const_iterator it = tiles.find(Resource & 0xfc000);

		if (it == tiles.end())
			return 0;

		const TileEntry &entry = it->second;

		return SliceStream(stream, entry.offset, entry.size * entry.count);
	} else {
		FileEntryMap::const_iterator it = files.find(Resource & 0x3fff);

		if (it == files.end())
			return 0;

		const FileEntry &entry = it->second;

		return SliceStream(stream, entry.offset, entry.size);
	}
	return NULL;
}

void BIFImporter::ReadBIF()
{
	ieDword fileCount, tileCount, offset;

	stream->ReadDword(&fileCount);
	stream->ReadDword(&tileCount);
	stream->ReadDword(&offset);

	stream->Seek(offset, GEM_STREAM_START);

	ieDword locator, count, size;
	ieWord dummy;

	for (ieDword i = 0; i < fileCount; ++i) {
		stream->ReadDword(&locator);
		stream->ReadDword(&offset);
		stream->ReadDword(&size);
		stream->ReadWord(&dummy); // type
		stream->ReadWord(&dummy); // unknown

		files.insert(FileEntryPair(locator & 0x3fff, FileEntry(offset, size)));
	}

	for (ieDword i = 0; i < tileCount; ++i) {
		stream->ReadDword(&locator);
		stream->ReadDword(&offset);
		stream->ReadDword(&count);
		stream->ReadDword(&size);
		stream->ReadWord(&dummy); // type
		stream->ReadWord(&dummy); // unknown

		tiles.insert(TileEntryPair(locator & 0xfc000, TileEntry(offset, count, size)));
	}
}

#include "plugindef.h"

GEMRB_PLUGIN(0xC7F133C, "BIF File Importer")
PLUGIN_CLASS(IE_BIF_CLASS_ID, BIFImporter)
END_PLUGIN()


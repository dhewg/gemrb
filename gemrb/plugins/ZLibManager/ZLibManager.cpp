/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "../../includes/win32def.h"
#include "../../includes/globals.h"
#include "ZLibManager.h"

#include <zlib.h>


ZLibManager::ZLibManager(void)
{
}

ZLibManager::~ZLibManager(void)
{
}


#define INPUTSIZE  4096
#define OUTPUTSIZE 4096

// ZLib Decompression Routine
int ZLibManager::Decompress(FILE* dest, DataStream* source)
{
	unsigned char bufferin[INPUTSIZE], bufferout[OUTPUTSIZE];
	z_stream stream;
	int result;

	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;

	result = inflateInit( &stream );
	if (result != Z_OK) {
		return GEM_ERROR;
	}

	stream.avail_in = 0;
	while (1) {
		stream.next_out = bufferout;
		stream.avail_out = OUTPUTSIZE;
		if (stream.avail_in == 0) {
			stream.next_in = bufferin;
			//Read doesn't allow partial reads, but provides Remains
			stream.avail_in = source->Remains();
			if (stream.avail_in > INPUTSIZE) {
				stream.avail_in=INPUTSIZE;
			}
			if (source->Read( bufferin, stream.avail_in) != (int) stream.avail_in) {
				return GEM_ERROR;
			}
		}
		result = inflate( &stream, Z_NO_FLUSH );
		if (( result != Z_OK ) && ( result != Z_STREAM_END )) {
			return GEM_ERROR;
		}
		if (fwrite( bufferout, 1, OUTPUTSIZE - stream.avail_out, dest ) <
			OUTPUTSIZE - stream.avail_out) {
			return GEM_ERROR;
		}
		if (result == Z_STREAM_END) {
			if (stream.avail_in > 0)
				source->Seek( -stream.avail_in, GEM_CURRENT_POS );
			result = inflateEnd( &stream );
			if (result != Z_OK)
				return GEM_ERROR;
			return GEM_OK;
		}
	}
}

int ZLibManager::Compress(DataStream* dest, DataStream* source)
{
	unsigned char bufferin[INPUTSIZE], bufferout[OUTPUTSIZE];
	z_stream stream;
	int result;

	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;

	//result = deflateInit( &stream, Z_DEFAULT_COMPRESSION );
	result = deflateInit( &stream, Z_BEST_COMPRESSION );
	if (result != Z_OK) {
		return GEM_ERROR;
	}

	stream.avail_in = 0;
	while (1) {
		stream.next_out = bufferout;
		stream.avail_out = OUTPUTSIZE;
		if (stream.avail_in == 0) {
			stream.next_in = bufferin;
			//Read doesn't allow partial reads, but provides Remains
			stream.avail_in = source->Remains();
			if (stream.avail_in > INPUTSIZE) {
				stream.avail_in=INPUTSIZE;
			}
			if (source->Read( bufferin, stream.avail_in) != (int) stream.avail_in) {
				return GEM_ERROR;
			}
		}
		if (stream.avail_in == 0) {
			result = deflate( &stream, Z_FINISH);
		} else {
			result = deflate( &stream, Z_NO_FLUSH );
		}
		if (( result != Z_OK ) && ( result != Z_STREAM_END )) {
			return GEM_ERROR;
		}
		if (dest->Write( bufferout, OUTPUTSIZE - stream.avail_out) == GEM_ERROR) {
			return GEM_ERROR;
		}
		if (result == Z_STREAM_END) {
			if (stream.avail_in > 0)
				source->Seek( -stream.avail_in, GEM_CURRENT_POS );
			result = deflateEnd( &stream );
			if (result != Z_OK)
				return GEM_ERROR;
			return GEM_OK;
		}
	}
}

#include "../../includes/plugindef.h"

GEMRB_PLUGIN(0x2477C688, "ZLib Compression Manager")
PLUGIN_CLASS(IE_COMPRESSION_CLASS_ID, ZLibManager)
END_PLUGIN()
/* GemRB - Infinity Engine Emulator
* Copyright (C) 2003-2005 The GemRB Project
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.

* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*
*/

#include "ResourceManager.h"

#include "System/VFS.h"
#include "System/FileStream.h"
#include "System/DataStream.h"
#include "Compressor.h"
#include "Interface.h"
#include "PluginMgr.h"
#include "Resource.h"
#include "ResourceDesc.h"
#include "ResourceSource.h"

ResourceManager::ResourceManager()
{
}


ResourceManager::~ResourceManager()
{
}

bool ResourceManager::AddSource(const char *path, const char *description, PluginID type, int flags)
{
	PluginHolder<ResourceSource> source(type);
	if (!source->Open(path, description)) {
		return false;
	}

	if (flags & RM_REPLACE_SAME_SOURCE) {
		for (size_t i = 0; i < searchPath.size(); i++) {
			if (!stricmp(description, searchPath[i]->GetDescription())) {
				searchPath[i] = source;
				break;
			}
		}
	} else {
		searchPath.push_back(source);
	}
	return true;
}

static void PrintPossibleFiles(const char* ResRef, const TypeID *type)
{
	const std::vector<ResourceDesc>& types = PluginMgr::Get()->GetResourceDesc(type);
	for (size_t j = 0; j < types.size(); j++) {
		print("%s.%s ", ResRef, types[j].GetExt());
	}
}

bool ResourceManager::Exists(const char *ResRef, SClass_ID type, bool silent) const 
{
	if (ResRef[0] == '\0')
		return false;
	// TODO: check various caches
	for (size_t i = 0; i < searchPath.size(); i++) {
		if (searchPath[i]->HasResource( ResRef, type )) {
			return true;
		}
	}
	if (!silent) {
		printMessage("ResourceManager", "Searching for %s.%s...", WHITE,
			ResRef, core->TypeExt(type));
		printStatus( "NOT FOUND", YELLOW );
	}
	return false;
}

bool ResourceManager::Exists(const char *ResRef, const TypeID *type, bool silent) const
{
	if (ResRef[0] == '\0')
		return false;
	// TODO: check various caches
	const std::vector<ResourceDesc> &types = PluginMgr::Get()->GetResourceDesc(type);
	for (size_t j = 0; j < types.size(); j++) {
		for (size_t i = 0; i < searchPath.size(); i++) {
			if (searchPath[i]->HasResource(ResRef, types[j])) {
				return true;
			}
		}
	}
	if (!silent) {
		printMessage("ResourceManager", "Searching for %s... ", WHITE, ResRef);
		print("Tried ");
		PrintPossibleFiles(ResRef,type);
		printStatus( "NOT FOUND", YELLOW );
	}
	return false;
}

DataStream* ResourceManager::GetResource(const char* ResRef, SClass_ID type, bool silent) const
{
	if (ResRef[0] == '\0')
		return NULL;
	if (!silent) {
		printMessage("ResourceManager", "Searching for %s.%s...", WHITE,
			ResRef, core->TypeExt(type));
	}
	for (size_t i = 0; i < searchPath.size(); i++) {
		DataStream *ds = searchPath[i]->GetResource(ResRef, type);
		if (ds) {
			if (!silent) {
				printStatus( searchPath[i]->GetDescription(), GREEN );
			}
			return ds;
		}
	}
	if (!silent) {
		printStatus( "ERROR", LIGHT_RED );
	}
	return NULL;
}

Resource* ResourceManager::GetResource(const char* ResRef, const TypeID *type, bool silent) const
{
	if (ResRef[0] == '\0')
		return NULL;
	if (!silent) {
		printMessage("ResourceManager", "Searching for %s... ", WHITE, ResRef);
	}
	const std::vector<ResourceDesc> &types = PluginMgr::Get()->GetResourceDesc(type);
	for (size_t j = 0; j < types.size(); j++) {
		for (size_t i = 0; i < searchPath.size(); i++) {
			DataStream *str = searchPath[i]->GetResource(ResRef, types[j]);
			if (str) {
				Resource *res = types[j].Create(str);
				if (res) {
					if (!silent) {
						print( "%s.%s...", ResRef, types[j].GetExt() );
						printStatus( searchPath[i]->GetDescription(), GREEN );
					}
					return res;
				}
			}
		}
	}
	if (!silent) {
		print("Tried ");
		PrintPossibleFiles(ResRef,type);
		printStatus( "ERROR", LIGHT_RED );
	}
	return NULL;
}

FileStream *ResourceManager::OpenCacheFile(const char *filename) const
{
	FileStream *stream = new FileStream();

	if (!stream)
		return NULL;

	char path[_MAX_PATH];
	PathJoin(path, core->CachePath, filename, NULL);

	if (!stream->Open(path)) {
		delete stream;
		return NULL;
	}

	return stream;
}

FileStream *ResourceManager::CreateCacheFile(const char *filename)
{
	FileStream *stream = new FileStream();

	if (!stream)
		return NULL;

	char path[_MAX_PATH];
	PathJoin(path, core->CachePath, filename, NULL);

	if (!stream->Create(path)) {
		delete stream;
		return NULL;
	}

	return stream;
}

FileStream *ResourceManager::CreateCacheFile(const char *filename, SClass_ID ClassID)
{
	FileStream *stream = new FileStream();

	if (!stream)
		return NULL;

	if (!stream->Create(core->CachePath, filename, ClassID)) {
		delete stream;
		return NULL;
	}

	return stream;
}

FileStream *ResourceManager::ModifyCacheFile(const char *filename)
{
	FileStream *stream = new FileStream();

	if (!stream)
		return NULL;

	char path[_MAX_PATH];
	PathJoin(path, core->CachePath, filename, NULL);

	if (!stream->Modify(path)) {
		delete stream;
		return NULL;
	}

	return stream;
}

DataStream* ResourceManager::AddCacheFile(const char *filename)
{
	if (!core->GameOnCD)
		return FileStream::OpenFile(filename);

	char fname[_MAX_PATH];
	ExtractFileFromPath(fname, filename);

	FileStream *dest = OpenCacheFile(fname);

	// already in cache
	if (dest)
		return dest;

	FileStream* src = FileStream::OpenFile(fname);
	dest = CreateCacheFile(fname);

	if (!src || !dest) {
		printMessage("ResourceManager", "Failed to copy file '%s'\n", RED, fname);
		abort();
	}

	size_t blockSize = 1024 * 1000;
	char buff[1024 * 1000];
	do {
		if (blockSize > src->Remains())
			blockSize = src->Remains();
		size_t len = src->Read(buff, blockSize);
		size_t c = dest->Write(buff, len);
		if (c != len) {
			printMessage("ResourceManager", "Failed to write to file '%s'\n", RED, fname);
			abort();
		}
	} while (src->Remains());

	delete src;
	delete dest;

	return OpenCacheFile(fname);
}

DataStream *ResourceManager::AddCompressedCacheFile(DataStream *stream, const char* filename, int length, bool overwrite)
{
	if (!core->IsAvailable(PLUGIN_COMPRESSION_ZLIB)) {
		printMessage("ResourceManager", "No compression manager available.\nCannot load compressed file.\n", RED);
		return NULL;
	}

	char fname[_MAX_PATH];
	ExtractFileFromPath(fname, filename);

	FileStream *str = NULL;

	if (!overwrite)
		str = OpenCacheFile(fname);

	if (!str) {
		FileStream *out = CreateCacheFile(fname);

		if (!out) {
			printMessage("ResourceManager", "Failed to write to file '%s'.\n", RED, fname);
			return NULL;
		}

		PluginHolder<Compressor> comp(PLUGIN_COMPRESSION_ZLIB);
		if (comp->Decompress(out, stream, length) != GEM_OK) {
			delete out;
			return NULL;
		}

		delete out;
	} else {
		stream->Seek(length, GEM_CURRENT_POS);
	}

	delete str;

	return OpenCacheFile(fname);
}


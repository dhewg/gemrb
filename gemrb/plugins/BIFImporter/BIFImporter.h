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

#ifndef BIFIMPORTER_H
#define BIFIMPORTER_H

#include <set>
#include <map>

#include "ArchiveImporter.h"

#include "globals.h"

#include "System/DataStream.h"

class BIFImporter : public ArchiveImporter {
private:
	struct FileEntry {
		ieDword offset;
		ieDword size;

		FileEntry(ieDword Offset, ieDword Size);
	};

	typedef std::pair<ieDword, FileEntry> FileEntryPair;
	typedef std::map<ieDword, FileEntry> FileEntryMap;

	struct TileEntry {
		ieDword offset;
		ieDword count;
		ieDword size;

		TileEntry(ieDword Offset, ieDword Count, ieDword Size);
	};

	typedef std::pair<ieDword, TileEntry> TileEntryPair;
	typedef std::map<ieDword, TileEntry> TileEntryMap;

	FileEntryMap files;
	TileEntryMap tiles;

	char path[_MAX_PATH];
	DataStream* stream;

public:
	BIFImporter();
	~BIFImporter();
	int DecompressSaveGame(DataStream *compressed);
	int AddToSaveGame(DataStream *str, DataStream *uncompressed);
	int OpenArchive(const char* filename);
	int CreateArchive(DataStream *compressed);
	DataStream* GetStream(unsigned long Resource, unsigned long Type);

private:
	void ReadBIF();
};

#endif

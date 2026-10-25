//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#ifndef DOWNLOAD_COMMAND_H
#define DOWNLOAD_COMMAND_H

#include "MD4Hash.h"
#include "Constants.h"

// A selection may outlive the file (notably across a modal confirmation dialog).
// The hash locates the download; ECID distinguishes a replacement with the same hash.
struct CDownloadTarget
{
	CMD4Hash hash;
	uint32 ecid;
	// Remote selections are also scoped to a connection generation.
	uint64 generation = 0;
};

enum class DownloadCommand
{
	Pause,
	Resume,
	Stop,
	Cancel
};

inline bool CanCancelDownload(uint8 status)
{
	return status != PS_WAITING_FOR_HASH && status != PS_HASHING && status != PS_COMPLETING &&
	       status != PS_COMPLETE;
}

// Resolve at execution time. Neither queued commands nor their callers own a file pointer.
// Lookup and apply run on the core/GUI event thread, without yielding between them.
template <typename Lookup, typename Apply>
bool ApplyDownloadCommand(const CDownloadTarget &target,
	DownloadCommand command,
	Lookup lookup,
	Apply apply,
	uint64 generation = 0,
	bool connected = true)
{
	if (!connected || target.generation != generation) {
		return false;
	}
	auto *file = lookup(target);
	if (!file || file->ECID() != target.ecid || file->GetFileHash() != target.hash) {
		return false;
	}
	if (command == DownloadCommand::Cancel && !CanCancelDownload(file->GetStatus())) {
		return false;
	}
	apply(file, command);
	return true;
}

#endif // DOWNLOAD_COMMAND_H

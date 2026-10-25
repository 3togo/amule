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

#include <muleunit/test.h>
#include "DownloadCommand.h"
#include <memory>

using namespace muleunit;

namespace
{
struct File
{
	CMD4Hash hash;
	uint32 id = 7;
	uint8 status = PS_READY;
	uint32 ECID() const { return id; }
	const CMD4Hash &GetFileHash() const { return hash; }
	uint8 GetStatus() const { return status; }
};
} // namespace

DECLARE_SIMPLE(DownloadCommand)

TEST(DownloadCommand, LocalAndRemoteLookupRejectStaleSelections)
{
	for (bool remote : { false, true }) {
		for (DownloadCommand command : { DownloadCommand::Pause,
			     DownloadCommand::Resume,
			     DownloadCommand::Stop,
			     DownloadCommand::Cancel }) {
			auto file = std::make_unique<File>();
			const CDownloadTarget selected{ file->hash, file->id };
			unsigned applied = 0;
			auto lookup = [&](const CDownloadTarget &target) -> File * {
				if (!file) {
					return nullptr;
				}
				return (remote ? file->id == target.ecid : file->hash == target.hash)
					       ? file.get()
					       : nullptr;
			};
			auto apply = [&](File *resolved, DownloadCommand action) {
				ASSERT_TRUE(file.get() == resolved);
				ASSERT_TRUE(action == command);
				++applied;
			};
			ASSERT_TRUE(ApplyDownloadCommand(selected, command, lookup, apply));
			file.reset(); // Removal while a dialog or queued command is outstanding.
			ASSERT_FALSE(ApplyDownloadCommand(selected, command, lookup, apply));
			file = std::make_unique<File>();
			file->id = 8; // A new download with the original hash.
			ASSERT_FALSE(ApplyDownloadCommand(selected, command, lookup, apply));
			file->id = selected.ecid;
			ASSERT_TRUE(file->hash.Decode(std::string("0123456789ABCDEF0123456789ABCDEF")));
			ASSERT_FALSE(ApplyDownloadCommand(selected, command, lookup, apply));
			ASSERT_EQUALS(1u, applied);
		}
	}
}

TEST(DownloadCommand, CancelRechecksStatusAfterConfirmation)
{
	File file;
	const CDownloadTarget selected{ file.hash, file.id };
	unsigned applied = 0;
	for (uint8 status : { PS_WAITING_FOR_HASH, PS_HASHING, PS_COMPLETING, PS_COMPLETE }) {
		file.status = status;
		ASSERT_FALSE(ApplyDownloadCommand(
			selected,
			DownloadCommand::Cancel,
			[&](const CDownloadTarget &) { return &file; },
			[&](File *, DownloadCommand) { ++applied; }));
	}
	ASSERT_EQUALS(0u, applied);
	for (uint8 status : { PS_READY, PS_EMPTY, PS_PAUSED, PS_ERROR, PS_INSUFFICIENT }) {
		ASSERT_TRUE(CanCancelDownload(status));
	}
}

TEST(DownloadCommand, CancelCanDestroyTheResolvedFile)
{
	auto file = std::make_unique<File>();
	const CDownloadTarget selected{ file->hash, file->id };
	ASSERT_TRUE(ApplyDownloadCommand(
		selected,
		DownloadCommand::Cancel,
		[&](const CDownloadTarget &) { return file.get(); },
		[&](File *, DownloadCommand) { file.reset(); }));
	ASSERT_FALSE(file);
}

TEST(DownloadCommand, ReconnectRejectsReusedIdentityBeforeLookup)
{
	File file;
	const CDownloadTarget selected{ file.hash, file.id, 4 };
	unsigned lookups = 0;
	unsigned applied = 0;
	auto lookup = [&](const CDownloadTarget &) {
		++lookups;
		return &file;
	};
	auto apply = [&](File *, DownloadCommand) { ++applied; };
	for (DownloadCommand command : { DownloadCommand::Pause,
		     DownloadCommand::Resume,
		     DownloadCommand::Stop,
		     DownloadCommand::Cancel }) {
		ASSERT_FALSE(ApplyDownloadCommand(selected, command, lookup, apply, 5));
		ASSERT_FALSE(ApplyDownloadCommand(selected, command, lookup, apply, 4, false));
	}
	ASSERT_EQUALS(0u, lookups);
	ASSERT_EQUALS(0u, applied);
	ASSERT_TRUE(ApplyDownloadCommand(selected, DownloadCommand::Cancel, lookup, apply, 4));
	ASSERT_EQUALS(1u, applied);
}

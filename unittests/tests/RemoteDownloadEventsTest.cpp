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
#include "RemoteDownloadEvents.h"

using namespace muleunit;

DECLARE_SIMPLE(RemoteDownloadEvents)

TEST(RemoteDownloadEvents, InitialAndNewCompletedFilesAreSilent)
{
	CRemoteDownloadEvents events;
	ASSERT_FALSE(events.Observe(1, PS_COMPLETE));
	ASSERT_FALSE(events.Observe(1, PS_COMPLETE));
	ASSERT_FALSE(events.Observe(2, PS_READY));
	// First encountered during a later poll: this is still a snapshot.
	ASSERT_FALSE(events.Observe(3, PS_COMPLETE));
	ASSERT_TRUE(events.Observe(2, PS_COMPLETE));
}

TEST(RemoteDownloadEvents, CompletionRunsOnceAfterAllIntermediateStates)
{
	CRemoteDownloadEvents events;
	ASSERT_FALSE(events.Observe(1, PS_EMPTY));
	ASSERT_FALSE(events.Observe(1, PS_READY));
	ASSERT_FALSE(events.Observe(1, PS_HASHING));
	ASSERT_FALSE(events.Observe(1, PS_COMPLETING));
	ASSERT_TRUE(events.Observe(1, PS_COMPLETE));
	ASSERT_FALSE(events.Observe(1, PS_COMPLETE));
}

TEST(RemoteDownloadEvents, FastCompletionDoesNotRequireObservingCompleting)
{
	CRemoteDownloadEvents events;
	ASSERT_FALSE(events.Observe(1, PS_READY));
	ASSERT_TRUE(events.Observe(1, PS_COMPLETE));
}

TEST(RemoteDownloadEvents, ErrorsDoNotMasqueradeAsCompletion)
{
	CRemoteDownloadEvents events;
	ASSERT_FALSE(events.Observe(1, PS_READY));
	ASSERT_FALSE(events.Observe(1, PS_ERROR));
	ASSERT_FALSE(events.Observe(1, PS_READY));
	ASSERT_FALSE(events.Observe(1, PS_COMPLETING));
	ASSERT_FALSE(events.Observe(1, PS_ERROR));
	ASSERT_TRUE(events.Observe(1, PS_COMPLETE));
}

TEST(RemoteDownloadEvents, ReconnectDoesNotReplayOfflineCompletions)
{
	CRemoteDownloadEvents events;
	ASSERT_FALSE(events.Observe(1, PS_READY));
	ASSERT_FALSE(events.Observe(2, PS_READY));
	events.Reset();
	ASSERT_FALSE(events.Observe(1, PS_COMPLETE));
	ASSERT_FALSE(events.Observe(2, PS_READY));
	ASSERT_FALSE(events.Observe(1, PS_COMPLETE));
	ASSERT_TRUE(events.Observe(2, PS_COMPLETE));
	// A daemon restart likewise discards ECID histories.
	events.Reset();
	ASSERT_FALSE(events.Observe(2, PS_COMPLETE));
}

TEST(RemoteDownloadEvents, RemovalDiscardsBaselineBeforeIdReuse)
{
	CRemoteDownloadEvents events;
	ASSERT_FALSE(events.Observe(1, PS_READY));
	events.Forget(1);
	ASSERT_FALSE(events.Observe(1, PS_COMPLETE));
	events.Forget(1);
	ASSERT_FALSE(events.Observe(1, PS_EMPTY));
	ASSERT_TRUE(events.Observe(1, PS_COMPLETE));
}

TEST(RemoteDownloadEvents, FilesHaveIndependentBaselines)
{
	CRemoteDownloadEvents events;
	ASSERT_FALSE(events.Observe(1, PS_READY));
	ASSERT_FALSE(events.Observe(2, PS_READY));
	ASSERT_TRUE(events.Observe(1, PS_COMPLETE));
	ASSERT_TRUE(events.Observe(2, PS_COMPLETE));
	ASSERT_FALSE(events.Observe(1, PS_COMPLETE));
	ASSERT_FALSE(events.Observe(2, PS_COMPLETE));
}

TEST(RemoteDownloadEvents, MissingStatusDoesNotInventOrAdvanceABaseline)
{
	CRemoteDownloadEvents events;
	// A metadata-only first packet leaves the proxy at its default PS_EMPTY.
	ASSERT_FALSE(events.Observe(1, PS_EMPTY, false));
	ASSERT_FALSE(events.Observe(1, PS_COMPLETE));
	ASSERT_FALSE(events.Observe(2, PS_READY));
	// An update without a status cannot consume a completion transition.
	ASSERT_FALSE(events.Observe(2, PS_COMPLETE, false));
	ASSERT_TRUE(events.Observe(2, PS_COMPLETE));
	ASSERT_FALSE(events.Observe(2, PS_COMPLETE));
}

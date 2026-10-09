// This file is part of the aMule Project.
// Copyright (c) 2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

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

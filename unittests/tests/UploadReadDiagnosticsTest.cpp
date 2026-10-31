// Copyright (c) 2026 aMule Team
// SPDX-License-Identifier: GPL-2.0-or-later
#include <muleunit/test.h>
#include "UploadReadDiagnostics.h"
#include <limits>
#include <stdexcept>

using namespace muleunit;
using namespace UploadReadDiagnostics;
DECLARE_SIMPLE(UploadReadDiagnostics)

TEST(UploadReadDiagnostics, HistogramBoundsAndReset)
{
	Histogram histogram;
	ASSERT_EQUALS(uint64_t{ 0 }, histogram.Median());
	for (uint64_t value : { 0, 1, 3, 8, 9 }) {
		histogram.Add(value);
	}
	ASSERT_EQUALS(uint64_t{ 4 }, histogram.Median());
	ASSERT_EQUALS(uint64_t{ 9 }, histogram.maximum);
	ASSERT_EQUALS(uint64_t{ 5 }, histogram.count);
	histogram = {};
	ASSERT_EQUALS(uint64_t{ 0 }, histogram.count);
	histogram.Add(std::numeric_limits<uint64_t>::max());
	ASSERT_EQUALS(std::numeric_limits<uint64_t>::max(), histogram.Median());
}

TEST(UploadReadDiagnostics, EverySampleHasOneState)
{
	for (bool pending : { false, true }) {
		for (bool requests : { false, true }) {
			for (bool target : { false, true }) {
				const auto expected = pending     ? SlotState::Reading
						      : !requests ? SlotState::Peer
						      : target    ? SlotState::Receiver
								  : SlotState::Storage;
				ASSERT_TRUE(Classify(pending, requests, target) == expected);
			}
		}
	}
}

TEST(UploadReadDiagnostics, TimingIsOptInAndDoesNotCrossIntervals)
{
	std::atomic<uint64_t> active{ 1 };
	std::atomic<uint64_t> started{ 0 };
	uint64_t finished = 0;
	unsigned clocks = 0;
	unsigned calls = 0;
	auto clock = [&] { return uint64_t{ ++clocks }; };
	auto work = [&] { ++calls; };
	RunTimed(0, active, started, finished, work, clock);
	ASSERT_EQUALS(0u, clocks);
	RunTimed(1, active, started, finished, work, clock);
	ASSERT_EQUALS(2u, clocks);
	ASSERT_EQUALS(uint64_t{ 1 }, started.load());
	ASSERT_EQUALS(uint64_t{ 2 }, finished);
	active = 0;
	RunTimed(1, active, started, finished, work, clock);
	ASSERT_EQUALS(2u, clocks);
	active = 2;
	RunTimed(1, active, started, finished, work, clock);
	ASSERT_EQUALS(2u, clocks);
	ASSERT_EQUALS(4u, calls);
	finished = 0;
	RunTimed(2, active, started, finished, [&] { active = 0; }, clock);
	ASSERT_EQUALS(3u, clocks);
	ASSERT_EQUALS(uint64_t{ 0 }, finished);
}

TEST(UploadReadDiagnostics, FailedReadsStillHaveTiming)
{
	std::atomic<uint64_t> active{ 1 };
	std::atomic<uint64_t> started{ 0 };
	uint64_t finished = 0;
	uint64_t now = 10;
	bool caught = false;
	try {
		RunTimed(
			1,
			active,
			started,
			finished,
			[] { throw std::runtime_error("read failed"); },
			[&] { return now++; });
	} catch (const std::runtime_error &) {
		caught = true;
	}
	ASSERT_TRUE(caught);
	ASSERT_EQUALS(uint64_t{ 10 }, started.load());
	ASSERT_EQUALS(uint64_t{ 11 }, finished);
}

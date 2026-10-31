// Copyright (c) 2026 aMule Team
// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef UPLOADREADDIAGNOSTICS_H
#define UPLOADREADDIAGNOSTICS_H

#include <array>
#include <atomic>
#include <cstdint>
#include <limits>

namespace UploadReadDiagnostics
{
// The coordinator publishes zero when disabled. A read spanning a toggle is
// excluded from the new interval; an untimed job never acquires timestamps.
template <typename Work, typename Clock>
void RunTimed(uint64_t epoch,
	const std::atomic<uint64_t> &activeEpoch,
	std::atomic<uint64_t> &started,
	uint64_t &finished,
	Work work,
	Clock clock)
{
	const bool timed = epoch != 0 && activeEpoch.load(std::memory_order_relaxed) == epoch;
	if (timed) {
		started.store(clock(), std::memory_order_relaxed);
	}
	const auto finish = [&] {
		if (timed && activeEpoch.load(std::memory_order_relaxed) == epoch) {
			finished = clock();
		}
	};
	try {
		work();
	} catch (...) {
		finish();
		throw;
	}
	finish();
}

// Millisecond buckets with power-of-two upper bounds. Median is an upper
// estimate (less than twice the observed value); maximum remains exact.
struct Histogram
{
	std::array<uint64_t, 65> buckets{};
	uint64_t count = 0;
	uint64_t maximum = 0;
	void Add(uint64_t value)
	{
		unsigned bucket = 0;
		uint64_t bound = 0;
		while (bound < value && bucket < 64) {
			++bucket;
			bound = bucket == 64 ? std::numeric_limits<uint64_t>::max()
					     : uint64_t{ 1 } << (bucket - 1);
		}
		++buckets[bucket];
		++count;
		if (value > maximum) {
			maximum = value;
		}
	}
	uint64_t Median() const
	{
		uint64_t cumulative = 0;
		for (unsigned i = 0; i < buckets.size(); ++i) {
			cumulative += buckets[i];
			if (cumulative >= (count + 1) / 2) {
				return i == 0 ? 0
					      : (i == 64 ? std::numeric_limits<uint64_t>::max()
							 : uint64_t{ 1 } << (i - 1));
			}
		}
		return 0;
	}
};
enum class SlotState
{
	Storage,
	Reading,
	Receiver,
	Peer
};
inline SlotState Classify(bool pending, bool requests, bool atTarget)
{
	if (pending) {
		return SlotState::Reading;
	}
	if (!requests) {
		return SlotState::Peer;
	}
	return atTarget ? SlotState::Receiver : SlotState::Storage;
}
struct Volume
{
	Histogram read;
	Histogram dispatch;
	uint64_t bytes = 0;
	uint64_t deliveryMaximum = 0;
	uint64_t turns = 0;
	uint64_t turnReads = 0;
	uint64_t busySkips = 0;
};
} // namespace UploadReadDiagnostics
#endif

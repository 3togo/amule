//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//

#include "DownloadBandwidthThrottler.h"

#include "EMSocket.h" // Needed for CEMSocket::WakeIfPaused

#include <climits>
#include <algorithm>

CDownloadBandwidthThrottler &CDownloadBandwidthThrottler::Get()
{
	static CDownloadBandwidthThrottler s_instance;
	return s_instance;
}

void CDownloadBandwidthThrottler::RefillBudget(
	uint32 maxDownloadKBps, uint32 tickPeriodMs, uint64 memoryHeadroom)
{
	m_memoryAvailable.store(
		static_cast<int64_t>(std::min<uint64>(memoryHeadroom, INT64_MAX)), std::memory_order_release);
	m_memoryUnlimited.store(memoryHeadroom == UINT64_MAX, std::memory_order_release);
	if (maxDownloadKBps == 0) {
		// MaxDownload=0 means literally unlimited. Saturate the bucket so even a Reserve()
		// that raced past the m_unlimited check still returns the full request.
		m_unlimited.store(true, std::memory_order_release);
		m_bytesAvailable.store(INT64_MAX, std::memory_order_release);
		return;
	}

	// uint64 intermediate so a MaxDownload up to the UI cap (1 000 000 KB/s)
	// doesn't overflow uint32: 1 000 000 * 1024 * 300 = 3 * 10^11 > 2^32.
	const int64_t budget = (int64_t)maxDownloadKBps * 1024 * tickPeriodMs / 1000;

	m_unlimited.store(false, std::memory_order_release);
	// Add this tick's budget to whatever was left unconsumed last tick, but cap the bucket at
	// 2x budget so a long quiet period cannot bank capacity that bursts well past the average
	// cap. A strict overwrite with no carry-over starves TCP: the receiver pauses reads when
	// the bucket empties mid-tick, the seeder's TCP flow control reads that as "consumer
	// overloaded" and slows down, and by the time the bucket refills the seeder is not sending
	// fast enough to consume the new budget. A small carry-over keeps reads flowing across tick
	// boundaries.
	int64_t current = m_bytesAvailable.load(std::memory_order_acquire);
	if (current < 0) {
		current = 0;
	}
	int64_t newBudget = current >= budget ? budget * 2 : current + budget;
	const int64_t cap = budget * 2;
	if (newBudget > cap) {
		newBudget = cap;
	}
	m_bytesAvailable.store(newBudget, std::memory_order_release);
}

namespace
{
uint32 ReserveFrom(std::atomic<int64_t> &bucket, uint32 wantBytes)
{
	int64_t current = bucket.load(std::memory_order_acquire);
	while (current > 0) {
		const uint32 granted = static_cast<uint32>(std::min<int64_t>(current, wantBytes));
		if (bucket.compare_exchange_weak(
			    current, current - granted, std::memory_order_acq_rel, std::memory_order_acquire))
			return granted;
	}
	return 0;
}
} // namespace

uint32 CDownloadBandwidthThrottler::Reserve(uint32 wantBytes, bool fileData)
{
	const uint32 bandwidth = m_unlimited.load(std::memory_order_acquire)
					 ? wantBytes
					 : ReserveFrom(m_bytesAvailable, wantBytes);
	if (!fileData || m_memoryUnlimited.load(std::memory_order_acquire))
		return bandwidth;
	const uint32 granted = ReserveFrom(m_memoryAvailable, bandwidth);
	if (bandwidth > granted && !m_unlimited.load(std::memory_order_acquire))
		m_bytesAvailable.fetch_add(bandwidth - granted, std::memory_order_acq_rel);
	return granted;
}

void CDownloadBandwidthThrottler::PauseUntilRefill(CEMSocket *socket)
{
	std::lock_guard<std::mutex> lock(m_pausedLock);
	m_paused.insert(socket);
}

void CDownloadBandwidthThrottler::Forget(CEMSocket *socket)
{
	std::lock_guard<std::mutex> lock(m_pausedLock);
	m_paused.erase(socket);
	// Also while a wake pass is walking it, which is how a socket destroyed
	// as a side effect of waking another never gets handed out.
	m_waking.erase(socket);
}

void CDownloadBandwidthThrottler::WakePaused()
{
	{
		std::lock_guard<std::mutex> lock(m_pausedLock);
		// Move rather than copy: anything that suspends again during this pass accumulates
		// in m_paused for the next tick instead of being retried now against a bucket it
		// has just emptied.
		m_waking.swap(m_paused);
	}

	// One at a time, each taken under the lock and woken outside it, because
	// waking runs the whole receive path and can destroy other sockets.
	for (;;) {
		CEMSocket *socket = nullptr;
		{
			std::lock_guard<std::mutex> lock(m_pausedLock);
			if (m_waking.empty()) {
				return;
			}
			const std::set<CEMSocket *>::iterator it = m_waking.begin();
			socket = *it;
			m_waking.erase(it);
		}
		socket->WakeIfPaused();
	}
}

void CDownloadBandwidthThrottler::Refund(uint32 bytes, bool fileData)
{
	if (!bytes)
		return;
	if (!m_unlimited.load(std::memory_order_acquire))
		m_bytesAvailable.fetch_add(bytes, std::memory_order_acq_rel);
	if (fileData && !m_memoryUnlimited.load(std::memory_order_acquire))
		m_memoryAvailable.fetch_add(bytes, std::memory_order_acq_rel);
}

// This file is part of the aMule Project.
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

#pragma once
#include <algorithm>
#include <cstdint>
namespace BroadbandUpload
{
constexpr uint64_t kReadAheadBudget = 64ull * 1024 * 1024;
inline uint64_t ReadAheadTarget(uint64_t rate, bool fast, uint64_t block, uint64_t slots)
{
	const uint64_t floor = (fast ? 10 : 1) * block;
	const uint64_t cap = 32 * block;
	const uint64_t desired =
		fast ? std::max(floor, std::min(cap, rate > cap / 2 ? cap : rate * 2)) : floor;
	return std::min(desired, kReadAheadBudget / std::max<uint64_t>(1, slots));
}
inline bool CanReadAhead(uint64_t queued, uint64_t request)
{
	return queued <= kReadAheadBudget && request <= kReadAheadBudget - queued;
}
} // namespace BroadbandUpload

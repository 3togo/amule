// This file is part of the aMule Project.
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

#ifndef SOURCESEARCHPOLICY_H
#define SOURCESEARCHPOLICY_H

#include <cstdint>

namespace SourceSearchPolicy
{
// Age takes precedence after thirty minutes beyond eligibility. This bounds
// starvation without increasing either the server or Kad request budget.
constexpr uint64_t kMaximumWait = 30 * 60 * 1000;
struct Candidate
{
	uint32_t validSources;
	uint32_t totalSources;
	uint64_t eligibleSince;
	uint8_t priority;
};
inline bool Better(const Candidate &left, const Candidate &right, uint64_t now)
{
	const uint64_t leftAge = now > left.eligibleSince ? now - left.eligibleSince : 0;
	const uint64_t rightAge = now > right.eligibleSince ? now - right.eligibleSince : 0;
	const bool leftOverdue = leftAge >= kMaximumWait;
	const bool rightOverdue = rightAge >= kMaximumWait;
	if (leftOverdue != rightOverdue)
		return leftOverdue;
	if (leftOverdue && leftAge != rightAge)
		return leftAge > rightAge;
	if (left.validSources != right.validSources)
		return left.validSources < right.validSources;
	if (left.totalSources != right.totalSources)
		return left.totalSources < right.totalSources;
	if (left.priority != right.priority)
		return left.priority > right.priority;
	return leftAge > rightAge;
}
} // namespace SourceSearchPolicy
#endif

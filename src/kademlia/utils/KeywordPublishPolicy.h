// This file is part of the aMule Project.
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

#ifndef KEYWORDPUBLISHPOLICY_H
#define KEYWORDPUBLISHPOLICY_H
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>
namespace Kademlia
{
constexpr size_t kKeywordPublishLimit = 150;
struct KeywordPublishCandidate
{
	size_t index;
	uint64_t lastRound;
	uint8_t priority;
	uint64_t transferred;
	uint64_t fileSize;
};
inline bool BetterKeywordCandidate(const KeywordPublishCandidate &a, const KeywordPublishCandidate &b)
{
	// Fairness first: every eligible file gets a turn before recently offered
	// files. Within the same round prefer upload priority and under-shared files.
	if (a.lastRound != b.lastRound)
		return a.lastRound < b.lastRound;
	if (a.priority != b.priority)
		return a.priority > b.priority;
	const long double aRatio = a.fileSize ? static_cast<long double>(a.transferred) / a.fileSize : 0;
	const long double bRatio = b.fileSize ? static_cast<long double>(b.transferred) / b.fileSize : 0;
	if (aRatio != bRatio)
		return aRatio < bRatio;
	return a.index < b.index;
}
inline void SelectKeywordCandidates(std::vector<KeywordPublishCandidate> &candidates)
{
	const size_t count = std::min(candidates.size(), kKeywordPublishLimit);
	std::partial_sort(
		candidates.begin(), candidates.begin() + count, candidates.end(), BetterKeywordCandidate);
	candidates.resize(count);
}
} // namespace Kademlia
#endif

// This file is part of the aMule Project.
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

#include <muleunit/test.h>
#include "kademlia/utils/KeywordPublishPolicy.h"
#include <set>
using namespace muleunit;
using namespace Kademlia;
DECLARE_SIMPLE(KadKeywordPublish)
TEST(KadKeywordPublish, ExactLimitAndFairCoverage)
{
	std::vector<uint64_t> last(301, 0);
	std::set<size_t> seen;
	for (uint64_t round = 1; round <= 3; ++round) {
		std::vector<KeywordPublishCandidate> candidates;
		for (size_t i = 0; i < last.size(); ++i)
			candidates.push_back({ i, last[i], uint8_t(i < 150 ? 4 : 0), 0, 100 });
		SelectKeywordCandidates(candidates);
		ASSERT_EQUALS(size_t(150), candidates.size());
		std::set<size_t> batch;
		for (const auto &candidate : candidates) {
			ASSERT_TRUE(batch.insert(candidate.index).second);
			seen.insert(candidate.index);
			last[candidate.index] = round;
		}
	}
	ASSERT_EQUALS(size_t(301), seen.size());
}
TEST(KadKeywordPublish, PriorityAndUploadRatioWithinSameRound)
{
	KeywordPublishCandidate high = { 0, 0, 4, 1000, 100 };
	KeywordPublishCandidate low = { 1, 0, 0, 0, 100 };
	ASSERT_TRUE(BetterKeywordCandidate(high, low));
	low.priority = 4;
	ASSERT_TRUE(BetterKeywordCandidate(low, high));
	low.lastRound = 1;
	ASSERT_TRUE(BetterKeywordCandidate(high, low));
	ASSERT_FALSE(BetterKeywordCandidate(high, high));
}
TEST(KadKeywordPublish, EmptySmallAndZeroSizeLists)
{
	std::vector<KeywordPublishCandidate> empty;
	SelectKeywordCandidates(empty);
	ASSERT_TRUE(empty.empty());
	std::vector<KeywordPublishCandidate> small = { { 0, 0, 0, 0, 0 }, { 1, 0, 0, 1, 1 } };
	SelectKeywordCandidates(small);
	ASSERT_EQUALS(size_t(2), small.size());
	ASSERT_EQUALS(size_t(0), small.front().index);
}

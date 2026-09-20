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
// it under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2 of the License, or
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

#include "search/SearchIdGenerator.h"

using namespace muleunit;

DECLARE_SIMPLE(SearchIdGenerator)

// SearchIdGenerator is a process-wide singleton; reset between tests so they
// do not influence each other.
TEST(SearchIdGenerator, GeneratesUniqueMonotonicNonZeroIds)
{
	auto& gen = search::SearchIdGenerator::Instance();
	gen.clearAll();

	uint32_t a = gen.generateId();
	uint32_t b = gen.generateId();
	uint32_t c = gen.generateId();

	ASSERT_TRUE(a != 0);
	ASSERT_TRUE(b != 0);
	ASSERT_TRUE(c != 0);
	ASSERT_TRUE(a != b && b != c && a != c);

	// Monotonic from 1 (0 is the reserved invalid id).
	ASSERT_EQUALS(uint32_t(1), a);
	ASSERT_EQUALS(uint32_t(2), b);
	ASSERT_EQUALS(uint32_t(3), c);
}

TEST(SearchIdGenerator, ActiveCountAndPeek)
{
	auto& gen = search::SearchIdGenerator::Instance();
	gen.clearAll();

	gen.generateId();
	gen.generateId();

	ASSERT_EQUALS(size_t(2), gen.getActiveCount());
	ASSERT_EQUALS(size_t(2), gen.getActiveIds().size());

	// generateNewId never reuses released ids, so peekNextId is the next
	// monotonic value, not a recycled one.
	ASSERT_EQUALS(uint32_t(3), gen.peekNextId());
}

TEST(SearchIdGenerator, ReserveRejectsZeroAndDuplicates)
{
	auto& gen = search::SearchIdGenerator::Instance();
	gen.clearAll();

	ASSERT_TRUE(!gen.reserveId(0));            // 0 is invalid
	ASSERT_TRUE(gen.reserveId(100));
	ASSERT_TRUE(gen.isValidId(100));
	ASSERT_TRUE(!gen.reserveId(100));          // already active
	ASSERT_TRUE(!gen.isValidId(555));          // unknown id
}

TEST(SearchIdGenerator, ReleaseRemovesActive)
{
	auto& gen = search::SearchIdGenerator::Instance();
	gen.clearAll();

	uint32_t id = gen.generateId();
	ASSERT_TRUE(gen.isValidId(id));

	ASSERT_TRUE(gen.releaseId(id));
	ASSERT_TRUE(!gen.isValidId(id));
	ASSERT_TRUE(!gen.releaseId(id));           // not found after release
}

TEST(SearchIdGenerator, ClearAllResetsState)
{
	auto& gen = search::SearchIdGenerator::Instance();
	gen.clearAll();

	gen.generateId();
	gen.generateId();
	gen.clearAll();

	ASSERT_EQUALS(size_t(0), gen.getActiveCount());
	ASSERT_EQUALS(uint32_t(1), gen.peekNextId());
}

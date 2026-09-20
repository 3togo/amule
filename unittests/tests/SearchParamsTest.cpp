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

#include "search/SearchModel.h"

using namespace muleunit;

DECLARE_SIMPLE(SearchParams)

TEST(SearchParams, DefaultAndValidity)
{
	search::SearchParams p;

	ASSERT_TRUE(!p.isValid());              // empty search string is invalid
	p.searchString = wxT("ubuntu");
	ASSERT_TRUE(p.isValid());

	ASSERT_EQUALS((long)-1, p.getSearchId());
	p.setSearchId(123);
	ASSERT_EQUALS((long)123, p.getSearchId());

	ASSERT_TRUE(p.searchType == search::ModernSearchType::GlobalSearch);
	ASSERT_EQUALS((uint64_t)0, p.minSize);
	ASSERT_EQUALS((uint64_t)0, p.maxSize);
	ASSERT_EQUALS((uint32_t)0, p.availability);
}

TEST(SearchParams, Equality)
{
	search::SearchParams a, b;
	a.searchString = wxT("linux");
	b.searchString = wxT("linux");
	ASSERT_TRUE(a == b);

	a.searchString = wxT("different");
	ASSERT_TRUE(a != b);
}

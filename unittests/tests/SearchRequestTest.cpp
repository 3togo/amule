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
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
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
#include "SearchList.h"
#include "SearchRequest.h"

using namespace muleunit;
DECLARE_SIMPLE(SearchRequest)

TEST(SearchRequest, OnlyRunningRequestsAreReused)
{
	CSearchList::CSearchParams params;
	params.searchString = "ubuntu";
	const CSearchRequest request(GlobalSearch, params);
	ASSERT_TRUE(request.CanReuse(request, 0));
	ASSERT_TRUE(request.CanReuse(request, 50));
	ASSERT_TRUE(request.CanReuse(request, 100));
	ASSERT_TRUE(!request.CanReuse(request, 0xffff));
	ASSERT_TRUE(!request.CanReuse(request, 0xfffe));
	ASSERT_TRUE(!request.CanReuse(request, 0xffffffff));
}

TEST(SearchRequest, EverySubmittedFilterDistinguishesRequests)
{
	CSearchList::CSearchParams params;
	params.searchString = "ubuntu";
	params.extension = "iso";
	params.typeText = "Pro";
	params.minSize = UINT64_C(1) << 33;
	params.maxSize = UINT64_C(1) << 34;
	params.availability = 3;
	const CSearchRequest request(GlobalSearch, params);
	ASSERT_TRUE(request.CanReuse(CSearchRequest(GlobalSearch, params), 25));
	ASSERT_TRUE(!request.CanReuse(CSearchRequest(LocalSearch, params), 25));
	ASSERT_TRUE(!request.CanReuse(CSearchRequest(KadSearch, params), 25));
	{
		auto different = params;
		different.searchString = "debian";
		ASSERT_TRUE(!request.CanReuse(CSearchRequest(GlobalSearch, different), 25));
	}
	{
		auto different = params;
		different.extension = "zip";
		ASSERT_TRUE(!request.CanReuse(CSearchRequest(GlobalSearch, different), 25));
	}
	{
		auto different = params;
		different.typeText = "Audio";
		ASSERT_TRUE(!request.CanReuse(CSearchRequest(GlobalSearch, different), 25));
	}
	{
		auto different = params;
		different.minSize = params.minSize + 1;
		ASSERT_TRUE(!request.CanReuse(CSearchRequest(GlobalSearch, different), 25));
	}
	{
		auto different = params;
		different.maxSize = params.maxSize + 1;
		ASSERT_TRUE(!request.CanReuse(CSearchRequest(GlobalSearch, different), 25));
	}
	{
		auto different = params;
		different.availability = 4;
		ASSERT_TRUE(!request.CanReuse(CSearchRequest(GlobalSearch, different), 25));
	}
}

TEST(SearchRequest, RequestOwnsItsValuesAndPreservesQuerySyntax)
{
	CSearchList::CSearchParams params;
	params.searchString = "ubuntu OR debian";
	const CSearchRequest submitted(KadSearch, params);
	params.searchString = "ubuntu or debian";
	ASSERT_TRUE(!submitted.CanReuse(CSearchRequest(KadSearch, params), 0));
	params.searchString = "ubuntu OR debian";
	ASSERT_TRUE(submitted.CanReuse(CSearchRequest(KadSearch, params), 0));
	// The Kad keyword is derived by the core after submission, not another filter.
	params.strKeyword = "ubuntu";
	ASSERT_TRUE(submitted.CanReuse(CSearchRequest(KadSearch, params), 0));
}

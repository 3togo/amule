//                                                       -*- C++ -*-
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

#ifndef SEARCHREQUEST_H
#define SEARCHREQUEST_H

#include <wx/string.h>
#include <cstdint>

// The submitted request, independent of the result label and the search ID.
// Keep exact text: merging requests with different spelling or Boolean syntax
// is less safe than occasionally leaving two equivalent searches open.
class CSearchRequest
{
public:
	template <typename Params>
	CSearchRequest(int type, const Params &params)
	: m_type(type)
	, m_query(params.searchString)
	, m_fileType(params.typeText)
	, m_extension(params.extension)
	, m_minSize(params.minSize)
	, m_maxSize(params.maxSize)
	, m_availability(params.availability)
	{
	}

	bool CanReuse(const CSearchRequest &other, uint32_t progress) const
	{
		// Only a reported running search is reusable. The eD2k/Kad terminal
		// sentinels (0xffff/0xfffe) must allow a fresh request.
		return progress <= 100 && m_type == other.m_type && m_query == other.m_query &&
		       m_fileType == other.m_fileType && m_extension == other.m_extension &&
		       m_minSize == other.m_minSize && m_maxSize == other.m_maxSize &&
		       m_availability == other.m_availability;
	}

private:
	int m_type;
	wxString m_query;
	wxString m_fileType;
	wxString m_extension;
	uint64_t m_minSize;
	uint64_t m_maxSize;
	uint32_t m_availability;
};

#endif

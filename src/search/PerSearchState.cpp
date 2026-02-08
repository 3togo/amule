//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include "PerSearchState.h"
#include "../Logger.h"
#include <common/Format.h>

PerSearchState::PerSearchState(uint32_t searchId, uint8_t searchType, const wxString& searchString)
    : m_searchId(searchId)
    , m_searchType(searchType)
    , m_searchString(searchString)
    , m_searchPacket(nullptr)
    , m_64bitSearchPacket(false)
    , m_moreResultsMode(false)
    , m_moreResultsMaxServers(0)
    , m_KadSearchFinished(false)
    , m_KadSearchRetryCount(0)
{
    AddDebugLogLineC(logSearch, CFormat(wxT("PerSearchState created: ID=%u, Type=%u, String='%s'"))
        % searchId % searchType % searchString);
}

PerSearchState::~PerSearchState()
{
    AddDebugLogLineC(logSearch, CFormat(wxT("PerSearchState destroyed: ID=%u"))
        % m_searchId);
}

void PerSearchState::setSearchPacket(std::unique_ptr<CPacket> packet, bool is64bit)
{
    wxMutexLocker lock(m_mutex);
    m_searchPacket = std::move(packet);
    m_64bitSearchPacket = is64bit;
    
    AddDebugLogLineC(logSearch, CFormat(wxT("Search packet set for ID=%u: size=%u, 64bit=%d"))
        % m_searchId % (m_searchPacket ? m_searchPacket->GetPacketSize() : 0) % is64bit);
}

void PerSearchState::clearSearchPacket()
{
    wxMutexLocker lock(m_mutex);
    m_searchPacket.reset();
    m_64bitSearchPacket = false;
    
    AddDebugLogLineC(logSearch, CFormat(wxT("Search packet cleared for ID=%u"))
        % m_searchId);
}

void PerSearchState::addQueriedServer(uint32_t serverId)
{
    wxMutexLocker lock(m_mutex);
    m_queriedServers.insert(serverId);
    
    AddDebugLogLineC(logSearch, CFormat(wxT("Server added to queried set for ID=%u: server=%u"))
        % m_searchId % serverId);
}

bool PerSearchState::hasQueriedServer(uint32_t serverId) const
{
    wxMutexLocker lock(m_mutex);
    return m_queriedServers.find(serverId) != m_queriedServers.end();
}

size_t PerSearchState::getQueriedServerCount() const
{
    wxMutexLocker lock(m_mutex);
    return m_queriedServers.size();
}

void PerSearchState::clearQueriedServers()
{
    wxMutexLocker lock(m_mutex);
    m_queriedServers.clear();
    
    AddDebugLogLineC(logSearch, CFormat(wxT("Queried servers cleared for ID=%u"))
        % m_searchId);
}

void PerSearchState::setMoreResultsMode(bool enabled, int maxServers)
{
    wxMutexLocker lock(m_mutex);
    m_moreResultsMode = enabled;
    m_moreResultsMaxServers = maxServers;
    
    AddDebugLogLineC(logSearch, CFormat(wxT("More results mode set for ID=%u: enabled=%d, maxServers=%d"))
        % m_searchId % enabled % maxServers);
}

void PerSearchState::setKadSearchFinished(bool finished)
{
    wxMutexLocker lock(m_mutex);
    m_KadSearchFinished = finished;
    
    AddDebugLogLineC(logSearch, CFormat(wxT("Kad search finished state set for ID=%u: finished=%d"))
        % m_searchId % finished);
}

void PerSearchState::setKadSearchRetryCount(int retryCount)
{
    wxMutexLocker lock(m_mutex);
    m_KadSearchRetryCount = retryCount;
    
    AddDebugLogLineC(logSearch, CFormat(wxT("Kad search retry count set for ID=%u: retryCount=%d"))
        % m_searchId % retryCount);
}

void PerSearchState::incrementKadSearchRetryCount()
{
    wxMutexLocker lock(m_mutex);
    m_KadSearchRetryCount++;
    
    AddDebugLogLineC(logSearch, CFormat(wxT("Kad search retry count incremented for ID=%u: newCount=%d"))
        % m_searchId % m_KadSearchRetryCount);
}
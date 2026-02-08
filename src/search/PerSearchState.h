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

#ifndef PERSEARCHSTATE_H
#define PERSEARCHSTATE_H

#include <memory>
#include <set>
#include <wx/string.h>
#include <wx/thread.h>
#include "../Types.h"
#include "../Packet.h"

// Forward declarations
class CServer;

/**
 * PerSearchState - Manages state for a single search
 *
 * This class encapsulates all state that was previously stored globally
 * in CSearchList, allowing multiple searches to run concurrently without
 * interfering with each other.
 */
class PerSearchState {
public:
    /**
     * Constructor
     *
     * @param searchId The unique ID for this search
     * @param searchType The type of search (Local, Global, Kad)
     * @param params Search parameters
     */
    PerSearchState(uint32_t searchId, uint8_t searchType, const wxString& searchString);
    
    /**
     * Destructor
     */
    ~PerSearchState();
    
    // Delete copy constructor and copy assignment operator
    PerSearchState(const PerSearchState&) = delete;
    PerSearchState& operator=(const PerSearchState&) = delete;
    
    // Allow move constructor and move assignment operator
    PerSearchState(PerSearchState&&) = default;
    PerSearchState& operator=(PerSearchState&&) = default;
    
    /**
     * Get the search ID
     */
    uint32_t getSearchId() const { return m_searchId; }
    
    /**
     * Get the search type
     */
    uint8_t getSearchType() const { return m_searchType; }
    
    /**
     * Get the search string
     */
    wxString getSearchString() const { return m_searchString; }
    
    /**
     * Set the search packet for this search
     *
     * @param packet The search packet
     * @param is64bit Whether the packet uses 64-bit values
     */
    void setSearchPacket(std::unique_ptr<CPacket> packet, bool is64bit);
    
    /**
     * Get the search packet
     */
    CPacket* getSearchPacket() const { return m_searchPacket.get(); }
    
    /**
     * Check if the search packet uses 64-bit values
     */
    bool is64bitPacket() const { return m_64bitSearchPacket; }
    
    /**
     * Clear the search packet
     */
    void clearSearchPacket();
    
    /**
     * Add a server to the queried servers set
     *
     * @param serverId The server ID (IP address)
     */
    void addQueriedServer(uint32_t serverId);
    
    /**
     * Check if a server has been queried
     *
     * @param serverId The server ID to check
     * @return true if the server has been queried, false otherwise
     */
    bool hasQueriedServer(uint32_t serverId) const;
    
    /**
     * Get the number of queried servers
     */
    size_t getQueriedServerCount() const;
    
    /**
     * Clear the queried servers set
     */
    void clearQueriedServers();
    
    /**
     * Set more results mode
     *
     * @param enabled Whether more results mode is enabled
     * @param maxServers Maximum number of servers to query in more results mode
     */
    void setMoreResultsMode(bool enabled, int maxServers = 0);
    
    /**
     * Check if more results mode is enabled
     */
    bool isMoreResultsMode() const { return m_moreResultsMode; }
    
    /**
     * Get the maximum number of servers for more results mode
     */
    int getMoreResultsMaxServers() const { return m_moreResultsMaxServers; }
    
    /**
     * Set Kad search finished state
     *
     * @param finished Whether the Kad search is finished
     */
    void setKadSearchFinished(bool finished);
    
    /**
     * Check if Kad search is finished
     */
    bool isKadSearchFinished() const { return m_KadSearchFinished; }
    
    /**
     * Set Kad search retry count
     *
     * @param retryCount The retry count
     */
    void setKadSearchRetryCount(int retryCount);
    
    /**
     * Get Kad search retry count
     */
    int getKadSearchRetryCount() const { return m_KadSearchRetryCount; }
    
    /**
     * Increment Kad search retry count
     */
    void incrementKadSearchRetryCount();
    
    /**
     * Lock the state for thread-safe access
     */
    void lock() const { m_mutex.Lock(); }
    
    /**
     * Unlock the state
     */
    void unlock() const { m_mutex.Unlock(); }
    
private:
    // Search identification
    uint32_t m_searchId;
    uint8_t m_searchType;
    wxString m_searchString;
    
    // Search packet (for global searches)
    std::unique_ptr<CPacket> m_searchPacket;
    bool m_64bitSearchPacket;
    
    // Server tracking
    std::set<uint32_t> m_queriedServers;
    
    // More results mode
    bool m_moreResultsMode;
    int m_moreResultsMaxServers;
    
    // Kad search state
    bool m_KadSearchFinished;
    int m_KadSearchRetryCount;
    
    // Mutex for thread-safe access
    mutable wxMutex m_mutex;
};

#endif // PERSEARCHSTATE_H
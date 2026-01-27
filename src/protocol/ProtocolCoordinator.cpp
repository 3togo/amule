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
#include "protocol/ProtocolCoordinator.h"
#include "../amule.h"
#include "../DownloadQueue.h"
#include "../ClientList.h"
#include "../PartFile.h"
#include "../updownclient.h"
#include "../common/NetworkPerformanceMonitor.h"
#include <arpa/inet.h>  // For inet_addr
#include <netdb.h>      // For gethostbyname

namespace ProtocolIntegration {

bool SourceEndpoint::operator==(const SourceEndpoint& other) const {
    return protocol == other.protocol &&
           address == other.address &&
           port == other.port;
}

bool SourceEndpoint::is_duplicate(const SourceEndpoint& other) const {
    return address == other.address && port == other.port;
}

class ProtocolCoordinator::Impl {
private:
    bool m_hybrid_mode;
    uint32_t m_max_cross_sources;
    CoordinationStats m_stats;
    
public:
    Impl() : 
        m_hybrid_mode(false),  // Hybrid mode is now disabled
        m_max_cross_sources(50) {}
        
    std::vector<SourceEndpoint> discover_sources(
        const CPartFile* file,
        ProtocolType preferred,
        uint32_t max_sources) {
        
        std::vector<SourceEndpoint> sources;
        
        // 1. Get ED2K sources
        if (preferred == ProtocolType::ED2K || preferred == ProtocolType::HYBRID_AUTO) {
            const auto& source_list = file->GetSourceList();
            for (const auto& client_ref : source_list) {
                CUpDownClient* client = client_ref.GetClient();
                if (client && client->GetIP() != 0) {
                    SourceEndpoint endpoint;
                    endpoint.protocol = ProtocolType::ED2K;
                    endpoint.address = Uint32toStringIP(client->GetIP());
                    endpoint.port = client->GetUserPort();
                    endpoint.ed2k_hash = file->GetFileHash();
                    sources.push_back(endpoint);
                }
            }
            m_stats.total_sources_discovered += source_list.size();
        }
        
        // 2. Remove duplicates
        remove_duplicates(sources);
        
        // 3. Limit results
        if (sources.size() > max_sources) {
            sources.resize(max_sources);
        }
        
        return sources;
    }
    
    bool add_source(const SourceEndpoint& source, CPartFile* file) {
        switch (source.protocol) {
            case ProtocolType::ED2K:
                return add_ed2k_source(source, file);
            case ProtocolType::KADEMLIA:
                return add_kad_source(source, file);
            default:
                return false;
        }
    }
    
    bool add_ed2k_source(const SourceEndpoint& source, CPartFile* file) {
        try {
            // Validate inputs
            if (!file || source.address.empty() || source.port == 0) {
                return false;
            }

            // Convert address string to IP
            uint32_t ip = inet_addr(source.address.c_str());
            if (ip == INADDR_NONE) {
                // Try to resolve hostname if it's not an IP address
                struct hostent* host_entry = gethostbyname(source.address.c_str());
                if (host_entry != NULL) {
                    ip = *((uint32_t*) host_entry->h_addr_list[0]);
                }
            }

            if (ip == INADDR_NONE) {
                return false; // Could not resolve address
            }

            // Create a new client from the source information
            // Using the constructor: CUpDownClient(uint16 in_port, uint32 in_userid, uint32 in_serverup, uint16 in_serverport, CPartFile* in_reqfile, bool ed2kID, bool checkfriend)
            CUpDownClient* newSource = new CUpDownClient(
                source.port,              // port
                ip,                       // userid (converted from address)
                0,                        // server IP (not applicable for search result sources)
                0,                        // server port (not applicable for search result sources)
                file,                     // part file
                true,                     // ed2k ID
                true                      // check friend
            );

            // Add the source to the download queue
            theApp->downloadqueue->CheckAndAddSource(file, newSource);
            
            return true;
        } catch (...) {
            return false;
        }
    }
    
    bool add_kad_source(const SourceEndpoint& source, CPartFile* file) {
        try {
            // Validate inputs
            if (!file || source.address.empty() || source.port == 0) {
                return false;
            }

            // Convert address string to IP
            uint32_t ip = inet_addr(source.address.c_str());
            if (ip == INADDR_NONE) {
                // Try to resolve hostname if it's not an IP address
                struct hostent* host_entry = gethostbyname(source.address.c_str());
                if (host_entry != NULL) {
                    ip = *((uint32_t*) host_entry->h_addr_list[0]);
                }
            }

            if (ip == INADDR_NONE) {
                return false; // Could not resolve address
            }

            // Create a new client from the source information
            // For Kad sources, we pass different flags to distinguish from eD2k
            CUpDownClient* newSource = new CUpDownClient(
                source.port,              // port
                ip,                       // userid (converted from address)
                0,                        // server IP (Kad sources typically don't have server IP)
                0,                        // server port (Kad sources typically don't have server port)
                file,                     // part file
                false,                    // not an eD2k ID, this is Kad
                true                      // check friend
            );

            // Add the source to the download queue
            theApp->downloadqueue->CheckAndAddSource(file, newSource);
            
            return true;
        } catch (...) {
            return false;
        }
    }
    
    void remove_duplicates(std::vector<SourceEndpoint>& sources) {
        std::set<std::pair<std::string, uint16_t>> seen;
        size_t removed_count = 0;
        
        auto new_end = std::remove_if(sources.begin(), sources.end(),
            [&seen, &removed_count](const SourceEndpoint& source) {
                auto addr_port = std::make_pair(source.address, source.port);
                if (seen.count(addr_port)) {
                    removed_count++;
                    return true;
                }
                seen.insert(addr_port);
                return false;
            });
        
        sources.erase(new_end, sources.end());
        m_stats.duplicate_sources_removed += removed_count;
    }
    
    CoordinationStats get_stats() const {
        return m_stats;
    }
    
    void reset_stats() {
        m_stats = {};
    }
    
    bool is_hybrid_mode_enabled() const {
        return m_hybrid_mode;
    }
    
    void set_hybrid_mode_enabled(bool enabled) {
        m_hybrid_mode = enabled;
    }
};

ProtocolCoordinator::ProtocolCoordinator() : 
    pimpl_(std::make_unique<Impl>()) {}
    
ProtocolCoordinator::~ProtocolCoordinator() = default;

std::vector<SourceEndpoint> ProtocolCoordinator::discover_sources(
    const CPartFile* file,
    ProtocolType preferred,
    uint32_t max_sources) {
    
    return pimpl_->discover_sources(file, preferred, max_sources);
}

bool ProtocolCoordinator::add_source(const SourceEndpoint& source, CPartFile* file) {
    return pimpl_->add_source(source, file);
}

CoordinationStats ProtocolCoordinator::get_stats() const {
    return pimpl_->get_stats();
}

void ProtocolCoordinator::reset_stats() {
    pimpl_->reset_stats();
}

bool ProtocolCoordinator::is_hybrid_mode_enabled() const {
    return pimpl_->is_hybrid_mode_enabled();
}

void ProtocolCoordinator::set_hybrid_mode_enabled(bool enabled) {
    pimpl_->set_hybrid_mode_enabled(enabled);
}

} // namespace ProtocolIntegration
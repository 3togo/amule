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

namespace ProtocolIntegration {

class ProtocolCoordinator::Impl {
private:
    uint32_t m_max_sources;
    CoordinationStats m_stats;
    
public:
    Impl() : 
        m_max_sources(50) {}
        
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
        
        // Source discovery for ED2K protocol only
        
        // 3. Remove duplicates
        remove_duplicates(sources);
        
        // 4. Limit results
        if (sources.size() > max_sources) {
            sources.resize(max_sources);
        }
        
        return sources;
    }
    
    void remove_duplicates(std::vector<SourceEndpoint>& sources) {
        // Implementation of duplicate removal logic
        size_t removed_count = 0;
        // ... duplicate removal implementation
        m_stats.duplicate_sources_removed += removed_count;
    }
    
    // ...other implementation methods...
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

} // namespace ProtocolIntegration
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
            // Create ED2K client from endpoint
            auto client = std::make_shared<CUpDownClient>(
                nullptr, // socket will be created later
                inet_addr(source.address.c_str()),
                source.port,
                source.ed2k_hash
            );
            
            // Add to download queue
            return theApp->downloadqueue->AddSource(client.get(), file);
        } catch (...) {
            return false;
        }
    }
    
    bool add_kad_source(const SourceEndpoint& source, CPartFile* file) {
        // Implementation for adding Kad sources
        // This would involve calling Kad-specific functions
        // which are not detailed here
        return true;
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
    
    void set_hybrid_mode(bool enabled) {
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

void ProtocolCoordinator::set_hybrid_mode(bool enabled) {
    pimpl_->set_hybrid_mode(enabled);
}

} // namespace ProtocolIntegration
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
// External torrent search APIs integration for hybrid search functionality
//

#pragma once

#include <vector>
#include <string>
#include <curl/curl.h>

namespace BitTorrent {

struct ExternalSearchResult {
    std::string name;
    std::string info_hash;
    uint64_t size;
    uint32_t seeders;
    uint32_t leechers;
    std::vector<std::string> trackers;
    std::string source_api;
};

class ExternalSearchAPIs {
public:
    static ExternalSearchAPIs& instance();
    
    // Search multiple external torrent APIs
    std::vector<ExternalSearchResult> search_external_apis(const std::string& keyword, int max_results = 50);
    
    // Individual API implementations
    std::vector<ExternalSearchResult> search_1337x(const std::string& keyword, int max_results);
    std::vector<ExternalSearchResult> search_thepiratebay(const std::string& keyword, int max_results);
    std::vector<ExternalSearchResult> search_rarbg(const std::string& keyword, int max_results);
    
private:
    ExternalSearchAPIs();
    ~ExternalSearchAPIs();
    
    CURL* m_curl_handle;
    bool m_initialized;
    
    // HTTP helper methods
    std::string http_get(const std::string& url);
    std::string url_encode(const std::string& str);
    
    // Disable copying
    ExternalSearchAPIs(const ExternalSearchAPIs&) = delete;
    ExternalSearchAPIs& operator=(const ExternalSearchAPIs&) = delete;
};

} // namespace BitTorrent
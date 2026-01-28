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

#ifndef SEARCHCONTROLLER_H
#define SEARCHCONTROLLER_H

#include <functional>
#include <wx/string.h>
#include "SearchModel.h"

// Forward declarations
class CSearchFile;

namespace search {

class SearchController {
public:
    using SearchStartedCallback = std::function<void()>;
    using SearchCompletedCallback = std::function<void()>;
    using ResultsReceivedCallback = std::function<void(const std::vector<CSearchFile*>&)>;
    using ErrorCallback = std::function<void(const wxString&)>;
    using ProgressCallback = std::function<void(int)>;
    
    // Detailed progress information
    struct ProgressInfo {
        int percentage = 0;
        int serversContacted = 0;
        int resultsReceived = 0;
        wxString currentStatus;
    };
    using DetailedProgressCallback = std::function<void(const ProgressInfo&)>;

    virtual ~SearchController() = default;
    
    // Core search operations
    virtual void startSearch(const SearchParams& params) = 0;
    virtual void stopSearch() = 0;
    virtual void requestMoreResults() = 0;
    
    // State information
    virtual SearchState getState() const = 0;
    virtual SearchParams getSearchParams() const = 0;
    virtual long getSearchId() const = 0;
    
    // Result access
    virtual const std::vector<CSearchFile*>& getResults() const = 0;
    virtual size_t getResultCount() const = 0;
    
    // Callback setters
    void setOnSearchStarted(SearchStartedCallback callback) { m_onSearchStarted = callback; }
    void setOnSearchCompleted(SearchCompletedCallback callback) { m_onSearchCompleted = callback; }
    void setOnResultsReceived(ResultsReceivedCallback callback) { m_onResultsReceived = callback; }
    void setOnError(ErrorCallback callback) { m_onError = callback; }
    void setOnProgress(ProgressCallback callback) { m_onProgress = callback; }
    void setOnDetailedProgress(DetailedProgressCallback callback) { m_onDetailedProgress = callback; }
    
protected:
    // Protected callbacks for derived classes to trigger
    void notifySearchStarted() { if (m_onSearchStarted) m_onSearchStarted(); }
    void notifySearchCompleted() { if (m_onSearchCompleted) m_onSearchCompleted(); }
    void notifyResultsReceived(const std::vector<CSearchFile*>& results) { 
        if (m_onResultsReceived) m_onResultsReceived(results); 
    }
    void notifyError(const wxString& error) { if (m_onError) m_onError(error); }
    void notifyProgress(int progress) { if (m_onProgress) m_onProgress(progress); }
    void notifyDetailedProgress(const ProgressInfo& info) { if (m_onDetailedProgress) m_onDetailedProgress(info); }
    
private:
    SearchStartedCallback m_onSearchStarted;
    SearchCompletedCallback m_onSearchCompleted;
    ResultsReceivedCallback m_onResultsReceived;
    ErrorCallback m_onError;
    ProgressCallback m_onProgress;
    DetailedProgressCallback m_onDetailedProgress;
};

} // namespace search

#endif // SEARCHCONTROLLER_H
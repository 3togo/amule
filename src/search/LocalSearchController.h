#ifndef LOCAL_SEARCH_CONTROLLER_H
#define LOCAL_SEARCH_CONTROLLER_H

#include "SearchControllerBase.h"
#include "../SearchList.h"

namespace search {

/**
 * LocalSearchController handles Local (ED2K TCP) searches.
 * 
 * Unlike Global/Kad searches, Local searches do NOT register with SearchResultRouter.
 * They use the legacy SearchList direct integration as specified in the architecture:
 * "Local搜索由SearchList直接处理，不经过SearchResultRouter"
 */
class LocalSearchController : public SearchControllerBase {
public:
    LocalSearchController();
    ~LocalSearchController() override;

    /**
     * Start a Local search using the legacy SearchList interface
     */
    void startSearch(const SearchParams& params) override;
    
    /**
     * Stop the current Local search
     */
    void stopSearch() override;

    /**
     * Request more results (not supported for Local searches)
     */
    void requestMoreResults() override;
    
    /**
     * Get the search ID assigned by SearchList
     */
    long getSearchId() const override;

private:
    /**
     * Handle search completion callback from SearchList
     */
    void onLocalSearchCompleted(uint32_t searchId);
    
    /**
     * Handle search error callback from SearchList  
     */
    void onLocalSearchError(uint32_t searchId, const wxString& error);
    
    // Flag to track if search is active
    bool m_isActive = false;
};

} // namespace search

#endif // LOCAL_SEARCH_CONTROLLER_H
#include "../amule.h"
#include "../OtherFunctions.h"
#include "../SearchList.h"
#include "LocalSearchController.h"
#include "SearchLogging.h"

namespace search {

LocalSearchController::LocalSearchController() 
    : SearchControllerBase()
{
    // Set the search state in the model (not search type)
    m_model->setSearchState(SearchState::Idle);
    SEARCH_DEBUG(wxT("LocalSearchController created"));
}

LocalSearchController::~LocalSearchController()
{
    if (m_isActive) {
        stopSearch();
    }
    SEARCH_DEBUG(wxT("LocalSearchController destroyed"));
}

void LocalSearchController::startSearch(const SearchParams& params)
{
    if (m_isActive) {
        SEARCH_CRITICAL(wxT("Attempted to start Local search while another is active"));
        notifyError(static_cast<uint32_t>(getSearchId()), wxT("Search already in progress"));
        return;
    }

    // Validate search parameters first
    if (!validateSearchParams(params)) {
        // validateSearchParams already calls handleSearchError internally
        return;
    }

    // Create CSearchParams for legacy SearchList interface
    CSearchList::CSearchParams legacyParams;
    legacyParams.searchString = params.searchString;
    legacyParams.strKeyword = params.strKeyword;
    legacyParams.typeText = params.typeText;
    legacyParams.extension = params.extension;
    legacyParams.minSize = params.minSize;
    legacyParams.maxSize = params.maxSize;
    legacyParams.availability = params.availability;
    legacyParams.searchType = LocalSearch;

    // Store parameters in model
    m_model->setSearchParams(params);
    
    // Start search through legacy SearchList interface
    uint32 searchId = 0;
    wxString error = theApp->searchlist->StartNewSearch(&searchId, LocalSearch, legacyParams);
    
    if (!error.empty()) {
        SEARCH_CRITICAL(
            CFormat(wxT("Local search failed to start: %s")) % error);
        handleSearchError(0, error);  // Pass uint32_t directly
        return;
    }

    // Store the search ID assigned by SearchList
    m_model->setSearchId(searchId);
    m_isActive = true;
    
    SEARCH_DEBUG(
        CFormat(wxT("Local search started successfully with ID=%u")) % searchId);

    // Notify listeners that search has started
    notifySearchStarted(searchId);
}

void LocalSearchController::stopSearch()
{
    if (!m_isActive) {
        return;
    }

    uint32_t searchId = m_model->getSearchId();
    SEARCH_DEBUG(
        CFormat(wxT("Stopping Local search with ID=%u")) % searchId);

    // Stop search through legacy SearchList interface
    theApp->searchlist->StopSearch(searchId);
    
    m_isActive = false;
    
    // Notify listeners that search has stopped
    notifySearchCompleted(static_cast<uint32_t>(searchId));
}

long LocalSearchController::getSearchId() const
{
    return static_cast<long>(m_model->getSearchId());
}

void LocalSearchController::requestMoreResults()
{
    // Local searches don't support requesting more results
    // They return all results in a single response
    SEARCH_DEBUG(wxT("Local search does not support requesting more results"));
}

void LocalSearchController::onLocalSearchCompleted(uint32_t searchId)
{
    if (searchId != m_model->getSearchId()) {
        return;
    }
    
    m_isActive = false;
    
    notifySearchCompleted(searchId);
}

void LocalSearchController::onLocalSearchError(uint32_t searchId, const wxString& error)
{
    if (searchId != m_model->getSearchId()) {
        return;
    }
    
    m_isActive = false;
    
    notifyError(searchId, error);
}

} // namespace search
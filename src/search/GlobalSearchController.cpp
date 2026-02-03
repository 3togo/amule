#include "GlobalSearchController.h"
#include "../amule.h"
#include "../kademlia/kademlia/Kademlia.h"

namespace search {

GlobalSearchController::GlobalSearchController()
    : m_ed2kController(std::make_unique<ED2KSearchController>())
    , m_kadController(std::make_unique<KadSearchController>())
{
}

GlobalSearchController::~GlobalSearchController()
{
    stopSearch();
}

void GlobalSearchController::startSearch(const SearchParams& params)
{
    // Store search parameters in model
    m_model->setSearchParams(params);
    
    // Generate unique search ID for global search
    uint32_t searchId = m_model->getSearchId();
    updateSearchState(params, searchId, SearchState::Searching);
    
    // Start both ED2K and Kad searches
    if (theApp && theApp->IsConnectedED2K()) {
        m_ed2kController->startSearch(params);
    }
    
    if (Kademlia::CKademlia::IsRunning()) {
        m_kadController->startSearch(params);
    }
    
    // If neither network is available, report error
    if (!theApp->IsConnectedED2K() && !Kademlia::CKademlia::IsRunning()) {
        handleSearchError(searchId, _("Neither ED2K nor Kad networks are available"));
    }
}

void GlobalSearchController::stopSearch()
{
    m_ed2kController->stopSearch();
    m_kadController->stopSearch();
    m_model->setSearchState(SearchState::Idle);
}

void GlobalSearchController::requestMoreResults()
{
    // For global search, request more results from both networks
    m_ed2kController->requestMoreResults();
    m_kadController->requestMoreResults();
}

} // namespace search
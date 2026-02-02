#ifndef GLOBALSEARCHCONTROLLER_H
#define GLOBALSEARCHCONTROLLER_H
#include <memory>
#include <set>
#include <wx/string.h>
#include <wx/timer.h>
#include "SearchControllerBase.h"
#include "SearchControllerFactory.h"
#include "SearchModel.h"
#include "SearchResultRouter.h"
#include "../Packet.h"

namespace search {

class GlobalSearchController : public SearchControllerBase
{
public:
    explicit GlobalSearchController();
    ~GlobalSearchController() override;

    // Delete copy constructor and copy assignment operator
    GlobalSearchController(const GlobalSearchController&) = delete;
    GlobalSearchController& operator=(const GlobalSearchController&) = delete;

    // Implement SearchController interface
    void startSearch(const SearchParams& params) override;
    void stopSearch() override;
    void requestMoreResults() override { /* Not applicable for global search */ }
    SearchState getState() const override { return isSearching() ? SearchState::Searching : SearchState::Idle; }

    // Global search specific methods
    void onTimer();
    void addQueriedServer(uint32_t serverIP);
    bool hasQueriedServer(uint32_t serverIP) const;

protected:
    // Check if searching is in progress
    bool isSearching() const;

private:
    // Timer for querying servers periodically
    wxTimer m_searchTimer;

    // Data for search request
    std::unique_ptr<CPacket> m_searchPacket;
    bool m_64bitSearchPacket;

    // Track queried servers to avoid duplicates
    std::set<uint32_t> m_queriedServers;
};

} // namespace search

#endif // GLOBALSEARCHCONTROLLER_H
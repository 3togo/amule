#ifndef LOCALSEARCHCONTROLLER_H
#define LOCALSEARCHCONTROLLER_H

#include <memory>
#include "SearchControllerBase.h"
#include "../Packet.h"

namespace search {

class LocalSearchController : public SearchControllerBase
{
public:
    explicit LocalSearchController();
    ~LocalSearchController() override;

    // Delete copy constructor and copy assignment operator
    LocalSearchController(const LocalSearchController&) = delete;
    LocalSearchController& operator=(const LocalSearchController&) = delete;

    // Implement SearchController interface
    void startSearch(const SearchParams& params) override;
    void stopSearch() override;
    void requestMoreResults() override;
    SearchState getState() const override { return isSearching() ? SearchState::Searching : SearchState::Idle; }

    // Method to generate search ID for local searches
    uint32_t GenerateSearchId();

protected:
    // Check if searching is in progress
    bool isSearching() const;

private:
    // Data for search request
    std::unique_ptr<CPacket> m_searchPacket;
    bool m_64bitSearchPacket;

    // Helper method to create search packet without SearchList dependency
    bool CreateSearchPacket(const SearchParams& params, uint8_t*& packetData, uint32_t& packetSize);
};

} // namespace search

#endif // LOCALSEARCHCONTROLLER_H
#ifndef GLOBAL_SEARCH_CONTROLLER_H
#define GLOBAL_SEARCH_CONTROLLER_H

#include "SearchControllerBase.h"
#include <cstdint>

class CPacket;

namespace search {

class GlobalSearchController : public SearchControllerBase
{
public:
    GlobalSearchController();
    virtual ~GlobalSearchController();

    // SearchController interface
    virtual void startSearch(const SearchParams& params) override;
    virtual void stopSearch() override;
    virtual void requestMoreResults() override;

private:
    uint32_t m_searchId;
    uint32_t GenerateSearchId();
};

} // namespace search

#endif // GLOBAL_SEARCH_CONTROLLER_H
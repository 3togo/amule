#pragma once

#include "SearchControllerBase.h"

namespace search {

class LocalSearchController : public SearchControllerBase
{
public:
    LocalSearchController();
    ~LocalSearchController() override;
    
    void startSearch(const SearchParams& params) override;
    void stopSearch() override;
    void requestMoreResults() override;
    
private:
    bool validateSearchParams(const SearchParams& params);
    
    static uint32_t s_searchIdCounter;
};

} // namespace search
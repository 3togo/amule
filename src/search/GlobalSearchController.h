#ifndef __GLOBAL_SEARCH_CONTROLLER_H__
#define __GLOBAL_SEARCH_CONTROLLER_H__

#include "SearchControllerBase.h"
#include "ED2KSearchController.h"
#include "KadSearchController.h"
#include <memory>

namespace search {

class GlobalSearchController : public SearchControllerBase
{
public:
    GlobalSearchController();
    virtual ~GlobalSearchController();
    
    virtual void startSearch(const SearchParams& params) override;
    virtual void stopSearch() override;
    virtual void requestMoreResults() override;
    
private:
    std::unique_ptr<ED2KSearchController> m_ed2kController;
    std::unique_ptr<KadSearchController> m_kadController;
};

} // namespace search

#endif // __GLOBAL_SEARCH_CONTROLLER_H__
#include "DefaultSearchRoutingStrategy.h"
#include "../amule.h"
#include "../SearchList.h"


bool DefaultSearchRoutingStrategy::ShouldRouteResult(const CSearchFile* result) const {
    // Apply standard filtering rules for non-KAD searches
    return !theApp->searchlist->IsFiltered(result);
}

void DefaultSearchRoutingStrategy::RouteResult(uint32_t searchId, const CSearchFile* result) const {
    theApp->searchlist->AddToList(result, false);
}

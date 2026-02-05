#include "KadSearchRoutingStrategy.h"
#include "../amule.h"


bool KadSearchRoutingStrategy::ShouldRouteResult(const CSearchFile* result) const {
    // KAD search results bypass strict filtering
    return true;
}

void KadSearchRoutingStrategy::RouteResult(uint32_t searchId, const CSearchFile* result) const {
    theApp->searchlist->AddToList(result, false);
}

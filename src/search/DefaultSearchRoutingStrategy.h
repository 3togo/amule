#pragma once
#include "SearchRoutingStrategy.h"

class CSearchFile;

class DefaultSearchRoutingStrategy : public SearchRoutingStrategy {
public:
    bool ShouldRouteResult(const CSearchFile* result) const override;
    void RouteResult(uint32_t searchId, const CSearchFile* result) const override;
};
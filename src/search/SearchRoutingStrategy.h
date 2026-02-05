#pragma once

#include "../SearchFile.h"
#include <vector>

// Forward declaration
class CSearchFile;

class SearchRoutingStrategy {
public:
    virtual ~SearchRoutingStrategy() = default;
    virtual bool ShouldRouteResult(const CSearchFile* result) const = 0;
    virtual void RouteResult(uint32_t searchId, const CSearchFile* result) const = 0;
};

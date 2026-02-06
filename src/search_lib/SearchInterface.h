#pragma once
#include <string>
#include <memory>
#include <vector>
#include <functional>

namespace search {

struct SearchResult {
    std::string fileName;
    uint64_t fileSize;
    std::string hash;
    int sourceCount;
    std::string networkType;
    std::string ed2kLink;  // Optional ED2K link
    std::string kadLink;   // Optional KAD link
};

enum class SearchType {
    LocalSearch,
    GlobalSearch,
    KadSearch
};

class SearchEventListener {
public:
    virtual void OnResultsAvailable(const std::vector<SearchResult>& results) = 0;
    virtual void OnSearchProgress(uint32_t current, uint32_t total) = 0;
    virtual void OnSearchComplete() = 0;
    virtual void OnError(const std::string& error) = 0;
    virtual ~SearchEventListener() = default;
};

class SearchEngine {
public:
    virtual void StartSearch(SearchType type, const std::string& query, SearchEventListener* listener) = 0;
    virtual void CancelSearch() = 0;
    virtual bool IsActive() const = 0;
    virtual ~SearchEngine() = default;
};

// Factory function to create platform-appropriate implementation
std::unique_ptr<SearchEngine> CreateSearchEngine();

// Forward declaration for the real implementation factory
std::unique_ptr<SearchEngine> CreateRealSearchEngine();

}

// Include implementation details
#ifdef UNIT_TESTING
#include "MockSearchEngine.h"
#else
#include "AMuleSearchEngine.h"
#endif

#ifndef UNIT_TESTING
namespace search {
inline std::unique_ptr<SearchEngine> CreateSearchEngine() {
    return CreateRealSearchEngine();
}
}
#endif
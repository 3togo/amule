#pragma once
#include "SearchInterface.h"
#include <thread>
#include <atomic>
#include <mutex>

namespace search {

class AMuleSearchEngine : public SearchEngine {
public:
    AMuleSearchEngine();
    ~AMuleSearchEngine();
    
    void StartSearch(SearchType type, const std::string& query, SearchEventListener* listener) override;
    void CancelSearch() override;
    bool IsActive() const override;

private:
    mutable std::mutex engineMutex;
    std::atomic<bool> isActiveFlag;
    std::thread searchThread;
    SearchEventListener* searchListener = nullptr;
    std::string searchTerm;
    uint32_t searchId;
};

// Factory function for creating the real search engine
std::unique_ptr<SearchEngine> CreateRealSearchEngine();

} // namespace search
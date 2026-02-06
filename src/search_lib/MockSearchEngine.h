#pragma once
#include "SearchInterface.h"
#include <thread>
#include <atomic>
#include <mutex>

namespace search {

class MockSearchEngine : public SearchEngine {
public:
    MockSearchEngine() : isActiveFlag(false) {}
    
    ~MockSearchEngine() {
        CancelSearch();
    }
    
    void StartSearch(SearchType type, const std::string& query, SearchEventListener* listener) override {
        std::lock_guard<std::mutex> lock(engineMutex);
        
        if (isActiveFlag) {
            listener->OnError("Search already in progress");
            return;
        }
        
        isActiveFlag = true;
        searchThread = std::thread([this, type, query, listener]() {
            // Simulate network delay
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            
            if (!isActiveFlag.load()) return; // Cancelled
            
            // Send progress update
            listener->OnSearchProgress(10, 100);
            
            // Simulate more processing
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            if (!isActiveFlag.load()) return; // Cancelled
            
            // Generate mock results
            std::vector<SearchResult> results;
            for (int i = 0; i < 5; ++i) {
                SearchResult r;
                r.fileName = query + "_part_" + std::to_string(i) + ".dat";
                r.fileSize = 1024 * 1024 * (i + 1); // 1MB, 2MB, etc
                r.hash = "mock_hash_" + std::to_string(i) + "_" + query;
                r.sourceCount = i * 2 + 1;
                
                switch(type) {
                    case SearchType::LocalSearch:
                        r.networkType = "Local";
                        r.ed2kLink = "ed2k://|file|" + r.fileName + "|" + std::to_string(r.fileSize) + "|" + r.hash + "|";
                        break;
                    case SearchType::GlobalSearch:
                        r.networkType = "ED2K";
                        r.ed2kLink = "ed2k://|file|" + r.fileName + "|" + std::to_string(r.fileSize) + "|" + r.hash + "|";
                        break;
                    case SearchType::KadSearch:
                        r.networkType = "KAD";
                        r.kadLink = "kad://" + r.hash;
                        break;
                }
                
                results.push_back(r);
            }
            
            if (!isActiveFlag.load()) return; // Cancelled
            
            listener->OnResultsAvailable(results);
            listener->OnSearchProgress(100, 100);
            listener->OnSearchComplete();
            isActiveFlag = false;
        });
    }

    void CancelSearch() override {
        isActiveFlag = false;
        if (searchThread.joinable()) {
            // In a real implementation we'd have a way to interrupt the thread
            // For this mock, we just wait for it to finish naturally
            searchThread.join();
        }
    }

    bool IsActive() const override {
        return isActiveFlag;
    }

private:
    mutable std::mutex engineMutex;
    std::atomic<bool> isActiveFlag;
    std::thread searchThread;
};

#ifdef UNIT_TESTING
inline std::unique_ptr<SearchEngine> CreateSearchEngine() {
    return std::make_unique<MockSearchEngine>();
}
#endif

}
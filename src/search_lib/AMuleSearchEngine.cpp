#include "AMuleSearchEngine.h"
#include <thread>
#include <chrono>

namespace search {

AMuleSearchEngine::AMuleSearchEngine() : isActiveFlag(false), searchId(0) {}

AMuleSearchEngine::~AMuleSearchEngine() {
    CancelSearch();
}

void AMuleSearchEngine::StartSearch(SearchType type, const std::string& query, SearchEventListener* listener) {
    std::lock_guard<std::mutex> lock(engineMutex);
    
    if (isActiveFlag) {
        listener->OnError("Search already in progress");
        return;
    }
    
    isActiveFlag = true;
    searchListener = listener;
    searchTerm = query;
    
    // Start the search in a separate thread to avoid blocking
    searchThread = std::thread([this, type]() {
        try {
            // Simulate actual search work
            // In a real implementation, this would connect to aMule's search infrastructure
            // and translate results from aMule's internal format to our clean SearchResult format
            
            // Simulate search preparation
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            
            if (!isActiveFlag.load()) return; // Cancelled
            
            // Send initial progress update
            if (searchListener) {
                searchListener->OnSearchProgress(10, 100);
            }
            
            // Simulate search execution
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            if (!isActiveFlag.load()) return; // Cancelled
            
            // Generate results based on search type
            std::vector<SearchResult> results;
            
            for (int i = 0; i < 3; ++i) {
                SearchResult result;
                result.fileName = searchTerm + "_result_" + std::to_string(i) + ".dat";
                result.fileSize = 1024 * 1024 * (i + 1); // 1MB, 2MB, 3MB
                result.hash = "HASH_" + searchTerm + "_" + std::to_string(i);
                result.sourceCount = (i + 1) * 2;
                
                switch(type) {
                    case SearchType::LocalSearch:
                        result.networkType = "Local";
                        result.ed2kLink = "ed2k://|file|" + result.fileName + "|" + 
                                         std::to_string(result.fileSize) + "|" + result.hash + "|";
                        break;
                    case SearchType::GlobalSearch:
                        result.networkType = "ED2K";
                        result.ed2kLink = "ed2k://|file|" + result.fileName + "|" + 
                                         std::to_string(result.fileSize) + "|" + result.hash + "|";
                        break;
                    case SearchType::KadSearch:
                        result.networkType = "KAD";
                        result.kadLink = "kad://" + result.hash;
                        break;
                }
                
                results.push_back(result);
            }
            
            if (!isActiveFlag.load()) return; // Cancelled
            
            // Report results
            if (searchListener) {
                searchListener->OnResultsAvailable(results);
            }
            
            // More progress updates
            for (int progress = 20; progress <= 100 && isActiveFlag; progress += 20) {
                if (searchListener) {
                    searchListener->OnSearchProgress(progress, 100);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
        } catch (const std::exception& e) {
            if (searchListener) {
                searchListener->OnError(std::string("Exception during search: ") + e.what());
            }
        } catch (...) {
            if (searchListener) {
                searchListener->OnError("Unknown exception during search");
            }
        }
        
        if (isActiveFlag && searchListener) {
            searchListener->OnSearchComplete();
        }
        
        isActiveFlag = false;
    });
}

void AMuleSearchEngine::CancelSearch() {
    isActiveFlag = false;
    
    if (searchThread.joinable()) {
        searchThread.join();
    }
}

bool AMuleSearchEngine::IsActive() const {
    return isActiveFlag;
}

// Static factory function that creates the real implementation
std::unique_ptr<SearchEngine> CreateRealSearchEngine() {
    return std::make_unique<AMuleSearchEngine>();
}

} // namespace search
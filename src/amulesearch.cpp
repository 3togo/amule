#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "search_lib/SearchInterface.h"

class SearchToolListener : public search::SearchEventListener {
public:
    void OnResultsAvailable(const std::vector<search::SearchResult>& results) override {
        std::cout << "\nFound " << results.size() << " results:\n";
        std::cout << "========================================\n";
        
        for (const auto& result : results) {
            std::cout << "Name: " << result.fileName << "\n";
            std::cout << "Size: " << result.fileSize << " bytes\n";
            std::cout << "Sources: " << result.sourceCount << "\n";
            std::cout << "Network: " << result.networkType << "\n";
            std::cout << "Hash: " << result.hash << "\n";
            if (!result.ed2kLink.empty()) {
                std::cout << "ED2K: " << result.ed2kLink << "\n";
            }
            if (!result.kadLink.empty()) {
                std::cout << "KAD: " << result.kadLink << "\n";
            }
            std::cout << "----------------------------------------\n";
        }
    }

    void OnSearchProgress(uint32_t current, uint32_t total) override {
        std::cout << "Progress: " << current << "/" << total << std::endl;
    }

    void OnSearchComplete() override {
        std::cout << "\nSearch completed.\n";
    }

    void OnError(const std::string& error) override {
        std::cerr << "Error: " << error << std::endl;
    }
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <search_type> <search_text>\n";
        std::cerr << "Search types: local, global, kad\n";
        return 1;
    }

    std::string searchTypeStr = argv[1];
    std::string searchText = argv[2];
    
    search::SearchType searchType;
    if (searchTypeStr == "local") {
        searchType = search::SearchType::LocalSearch;
    } else if (searchTypeStr == "global") {
        searchType = search::SearchType::GlobalSearch;
    } else if (searchTypeStr == "kad") {
        searchType = search::SearchType::KadSearch;
    } else {
        std::cerr << "Invalid search type. Use: local, global, or kad\n";
        return 1;
    }

    // Create search engine and listener
    auto engine = search::CreateSearchEngine();
    SearchToolListener listener;

    std::cout << "Starting " << searchTypeStr << " search for: " << searchText << std::endl;
    
    engine->StartSearch(searchType, searchText, &listener);
    
    // Wait for completion with timeout
    const int maxWaitSeconds = 30;
    for (int i = 0; i < maxWaitSeconds && engine->IsActive(); ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    if (engine->IsActive()) {
        std::cout << "Search still in progress, cancelling...\n";
        engine->CancelSearch();
    }

    return 0;
}
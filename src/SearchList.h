//
// SearchList.h - Search results management class
//

#ifndef SEARCHLIST_H
#define SEARCHLIST_H

#include "MD4Hash.h"
#include "SearchFile.h"
#include <vector>
#include <map>

// Forward declarations
class CUpDownClient;
class CServer;

// Search types enumeration
enum SearchType {
    LocalSearch = 0,
    GlobalSearch = 1,
    KadSearch = 2
};

class CSearchList
{
public:
    // Nested parameter class
    class CSearchParams
    {
    public:
        wxString searchString;
        wxString fileType;
        uint64 minSize;
        uint64 maxSize;
        uint32 availability;
        uint32 extension;
        bool completeSourcesOnly;
        uint32 minBitrate;
        uint32 maxBitrate;
        uint32 duration;
        uint32 codec;
    };

    // Search results type
    typedef std::vector<CSearchFile*> CSearchResultList;

    // Minimal implementation to satisfy dependencies
    void UpdateSearchFileByHash(const CMD4Hash& hash) {}
    CSearchFile* GetSearchFileByHash(const CMD4Hash& hash) { return nullptr; }
    
    // Methods needed for compilation
    const CSearchResultList& GetSearchResults(uint32 searchID) const;
    void AddFileToDownloadByHash(const CMD4Hash& hash, uint8 category);
    void StopSearch(bool globalOnly = false);
    void RemoveResults(uint32 searchID);
    wxString StartNewSearch(uint32* searchID, SearchType searchType, const CSearchParams& params);
    uint32 GetSearchProgress() const;
    void ProcessSharedFileList(const uint8_t* data, uint32 size, CUpDownClient* client, CServer* server, const wxString& directory);
    void SetKadSearchFinished();
    void ProcessSearchAnswer(const uint8_t* packet, uint32 size, CServer* server, uint32 ip, uint16 port);
    void ProcessUDPSearchAnswer(const uint8_t* packet, bool isExtended, uint32 ip, uint16 port);
    void LocalSearchEnd();
    void KademliaSearchKeyword(uint32 searchID, CSearchFile* answer, const wxString& name, uint64 size, const wxString& type, uint32 publishInfo, const TagPtrList& taglist);
    
    // Additional methods needed from search results
    CSearchFile* GetByID(uint32 id) { return nullptr; }
    
    // Member variables needed for compilation
    int m_curr_search;
    
private:
    std::map<uint32, CSearchResultList> m_results;
};

#endif // SEARCHLIST_H
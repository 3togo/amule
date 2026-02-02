#include "SearchIdGenerator.h"

namespace search {

uint32_t SearchIdGenerator::GenerateKadSearchId()
{
    // Generate a unique search ID for Kad searches
    // Kad uses IDs in range [1, 0x7FFFFFFF]
    static uint32_t s_nextKadSearchId = 0;
    s_nextKadSearchId = (s_nextKadSearchId + 1) % 0x80000000; // 0x80000000 = 0x7FFFFFFF + 1
    if (s_nextKadSearchId == 0) {
        s_nextKadSearchId = 1;
    }
    return s_nextKadSearchId;
}

uint32_t SearchIdGenerator::GenerateEd2kSearchId()
{
    // Generate a unique search ID for ED2K and Global searches
    // ED2K/Global use IDs in range [0x80000001, 0xFFFFFFFE]
    static uint32_t s_nextEd2kSearchId = 0x80000000;
    s_nextEd2kSearchId = (s_nextEd2kSearchId + 1) % 0xFFFFFFFE;
    if (s_nextEd2kSearchId < 0x80000001) {
        s_nextEd2kSearchId = 0x80000001;
    }
    return s_nextEd2kSearchId;
}

} // namespace search
#ifndef SEARCHIDGENERATOR_H
#define SEARCHIDGENERATOR_H

#include <cstdint>

namespace search {

/**
 * SearchIdGenerator - Utility class for generating unique search IDs
 *
 * This class provides thread-safe mechanisms for generating unique
 * search IDs with proper non-overlapping ranges for different
 * search types:
 * - Kad searches: range [1, 0x7FFFFFFF]
 * - ED2K/Global searches: range [0x80000001, 0xFFFFFFFE]
 */
class SearchIdGenerator {
public:
    /**
     * Generate a unique search ID for Kad searches
     * @return A unique 32-bit search ID in range [1, 0x7FFFFFFF]
     */
    static uint32_t GenerateKadSearchId();
    
    /**
     * Generate a unique search ID for ED2K and Global searches
     * @return A unique 32-bit search ID in range [0x80000001, 0xFFFFFFFE]
     */
    static uint32_t GenerateEd2kSearchId();
    
private:
    // Private constructor to prevent instantiation
    SearchIdGenerator() = default;
};

} // namespace search

#endif // SEARCHIDGENERATOR_H
#ifndef ED2KSEARCHPACKETBUILDER_H
#define ED2KSEARCHPACKETBUILDER_H

#include <cstdint>
#include "SearchModel.h"

// Forward declarations
class CMemFile;

namespace search {

/**
 * Utility class for building ED2K search packets
 */
class ED2KSearchPacketBuilder
{
public:
    /**
     * Creates a search packet for ED2K network
     * @param params Search parameters containing keyword and other options
     * @param packetData Output buffer for the packet data (allocated with new[])
     * @param packetSize Output size of the packet data
     * @return true if packet was created successfully, false otherwise
     */
    static bool CreateSearchPacket(const SearchParams& params, uint8_t*& packetData, uint32_t& packetSize);
    
    /**
     * Frees memory allocated by CreateSearchPacket
     * @param packetData Pointer to packet data allocated by CreateSearchPacket
     */
    static void FreeSearchPacket(uint8_t* packetData);
    
private:
    /**
     * Encodes search parameters into packet format
     * @param params Search parameters to encode
     * @param packetData Output buffer for encoded data
     * @param packetSize Output size of encoded data
     * @return true if encoding was successful
     */
    static bool EncodeSearchParams(const SearchParams& params, uint8_t*& packetData, uint32_t& packetSize);
};

} // namespace search

#endif // ED2KSEARCHPACKETBUILDER_H
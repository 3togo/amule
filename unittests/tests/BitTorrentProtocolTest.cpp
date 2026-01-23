#include "muleunit/test.h"
#include "protocol/bt/BitTorrentSession.h"
#include "MD4Hash.h"

using namespace muleunit;

DECLARE(BitTorrentProtocol)
    void testHashConversion() {
        CMD4Hash ed2k_hash;
        memset(ed2k_hash.GetHash(), 0xAB, 16);
        
        std::string info_hash = BitTorrent::ed2k_hash_to_info_hash(ed2k_hash);
        ASSERT_EQUALS(20, (int)info_hash.length());
    }
END_DECLARE;

TEST(BitTorrentProtocol, testHashConversion)
{
    // 测试哈希转换功能
}

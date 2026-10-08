#include <muleunit/test.h>
#include "ProtocolBounds.h"
#include "Packet.h"
#include "SafeFile.h"
#include <protocol/Protocols.h>
using namespace muleunit;
DECLARE_SIMPLE(ProtocolBounds)
TEST(ProtocolBounds, OverflowAndImpossibleCounts) {
    ASSERT_FALSE(ProtocolBounds::Span(10, 11, 0));
    ASSERT_FALSE(ProtocolBounds::Span(UINT64_MAX, UINT64_MAX - 1, 10));
    ASSERT_TRUE(ProtocolBounds::Span(10, 10, 0));
    ASSERT_FALSE(ProtocolBounds::Records(12, 3, UINT32_MAX, 3));
    ASSERT_TRUE(ProtocolBounds::Records(12, 3, 3, 3));
    uint64_t result = 7;
    ASSERT_FALSE(ProtocolBounds::Add(UINT64_MAX, 1, result));
    ASSERT_EQUALS(uint64_t(7), result);
}
TEST(ProtocolBounds, ZeroLengthHeaderIsNotOpcodeOnly) {
    uint8_t header[6] = { OP_EDONKEYPROT, 0, 0, 0, 0, 1 };
    ASSERT_EQUALS(uint32(UINT32_MAX), CPacket::GetPacketSizeFromHeader(header));
    bool rejected = false;
    try { CPacket packet(header, nullptr); } catch (const CInvalidPacket &) { rejected = true; }
    ASSERT_TRUE(rejected);
    header[1] = 1;
    ASSERT_EQUALS(uint32(0), CPacket::GetPacketSizeFromHeader(header));
    CPacket packet(header, nullptr);
    ASSERT_EQUALS(uint32(0), packet.GetPacketSize());
}
TEST(ProtocolBounds, ReleaseBuildCopyGuard) {
    CPacket packet(1, 4, OP_EDONKEYPROT);
    uint8_t data = 1;
    bool rejected = false;
    try { packet.CopyToDataBuffer(UINT32_MAX, &data, 2); }
    catch (const CInvalidPacket &) { rejected = true; }
    ASSERT_TRUE(rejected);
}

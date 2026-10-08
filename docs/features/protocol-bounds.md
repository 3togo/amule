# Protocol and metadata bounds

Shared subtraction-based span and record checks reject impossible metadata counts
before parsing known.met, part.met, and server.met records. There is no arbitrary
new tag-count ceiling: counts must fit the remaining serialized bytes. Blob tags
check the offset before subtracting remaining space.

TCP length 0 is malformed, whereas length 1 is a valid opcode-only packet.
`CPacket::GetPacketSizeFromHeader` returns UINT32_MAX for malformed lengths so the
receive path rejects them before allocation. Receive assembly rejects impossible
saved header/payload sizes. Packet copy bounds are enforced in release builds.

Validated with the daemon build, ProtocolBoundsTest, and CTagTest. Tests include
zero-length versus opcode-only TCP headers, overflowing copies, impossible counts,
and a truncated blob claiming a 4 GiB allocation.

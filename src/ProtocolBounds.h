// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstdint>
namespace ProtocolBounds {
inline bool Span(uint64_t length, uint64_t offset, uint64_t bytes) {
    return offset <= length && bytes <= length - offset;
}
inline bool Records(uint64_t length, uint64_t offset, uint64_t count, uint64_t minimum) {
    return offset <= length && minimum && count <= (length - offset) / minimum;
}
inline bool Add(uint64_t a, uint64_t b, uint64_t &result) {
    if (b > UINT64_MAX - a) return false;
    result = a + b; return true;
}
// UINT32_MAX is distinct from a valid header containing only the opcode.
inline uint32_t TcpPayload(uint32_t wireLength) {
    return wireLength && wireLength < 0x7ffffff0u ? wireLength - 1 : UINT32_MAX;
}
}

// SPDX-License-Identifier: GPL-2.0-or-later
// Adapted from eMuleBB PartFileEndgameSeams: scheduling decisions only.
#pragma once
#include <algorithm>
#include <cstdint>
namespace EndgamePolicy {
inline bool AtLeast(uint64_t done, uint64_t size, uint64_t permille) {
    if (!size) return false;
    return done >= (size / 1000) * permille + ((size % 1000) * permille + 999) / 1000;
}
inline bool IsEndgame(uint64_t done, uint64_t size, uint64_t rate) {
    return size > done && (AtLeast(done, size, 999)
        || (rate && rate >= (size - done) / 30 + ((size - done) % 30 != 0)));
}
inline bool Faster(uint64_t slow, uint64_t fast) { return fast && fast / 5 >= slow; }
inline uint64_t ReservationBytes(uint64_t rate, uint64_t full) {
    return std::min(full, std::max<uint64_t>(16 * 1024, rate > UINT64_MAX / 10 ? UINT64_MAX : rate * 10));
}
inline uint64_t ClampEnd(uint64_t start, uint64_t end, uint64_t cap) {
    if (end < start || !cap) return end;
    const uint64_t span = end - start;
    if (span < cap || span - cap + 1 < 3 * 1024) return end;
    return start + cap - 1;
}
inline bool MaySteal(uint64_t now, uint64_t last, bool started, uint64_t slow, uint64_t fast) {
    return !started && Faster(slow, fast) && (!last || (now >= last && now - last >= 60000));
}
}

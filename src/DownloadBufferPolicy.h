// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <algorithm>
#include <cstdint>
namespace DownloadBufferPolicy {
inline uint64_t Budget(uint32_t configuredMiB, uint64_t availableBytes) {
    if (!configuredMiB) return 0;
    const uint64_t cap = std::min<uint64_t>(configuredMiB, 512) * 1024 * 1024;
    return availableBytes ? std::min(cap, std::max<uint64_t>(1024 * 1024, availableBytes / 8)) : cap;
}
inline uint64_t Headroom(uint64_t budget, uint64_t used) { return used >= budget ? 0 : budget - used; }
inline uint64_t FileThreshold(uint64_t configured, uint64_t budget, uint64_t used,
                              uint64_t current, uint64_t files) {
    if (!budget) return configured;
    const uint64_t minimum = std::min<uint64_t>(64 * 1024, budget / std::max<uint64_t>(1, files));
    return std::max<uint64_t>(std::max<uint64_t>(1, minimum), current + Headroom(budget, used));
}
}

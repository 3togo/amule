// This file is part of the aMule Project.
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

#ifndef UPLOADADMISSIONPOLICY_H
#define UPLOADADMISSIONPOLICY_H
#include <algorithm>
#include <cstdint>
namespace BroadbandUpload
{
inline uint32_t AdmissionLimit(uint32_t base, uint32_t hardLimit, bool sustainedUnderfill)
{
	base = std::min(base, hardLimit);
	if (!sustainedUnderfill || base == hardLimit)
		return base;
	const uint32_t extra = std::max<uint32_t>(1, base / 4);
	return base + std::min(extra, hardLimit - base);
}
} // namespace BroadbandUpload
#endif

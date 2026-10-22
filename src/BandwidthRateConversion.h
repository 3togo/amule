//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#ifndef AMULE_BANDWIDTH_RATE_CONVERSION_H
#define AMULE_BANDWIDTH_RATE_CONVERSION_H
#include <algorithm>
#include <cmath>
#include <limits>
namespace BandwidthRateConversion
{
// Preserve the percentage until the tray renderer caps it at 100, but keep the
// conversion to int defined for a zero capacity, non-finite rates or huge ratios.
inline int ToPercent(double rate, unsigned int capacity)
{
	if (!(rate > 0) || capacity == 0) {
		return 0;
	}
	const double percent = std::ceil(rate * 100 / capacity);
	const int maximum = std::numeric_limits<int>::max();
	return percent >= maximum ? maximum : static_cast<int>(percent);
}

// Network connection speeds use decimal megabits; aMule limits use binary KiB/s.
inline double ToMegabits(int rate)
{
	return rate * 8192.0 / 1000000.0;
}
inline int FromMegabits(double rate, int maximum)
{
	if (!std::isfinite(rate) || rate <= 0) {
		return 0;
	}
	// Keep zero exclusively for unlimited, and respect the preference control's range.
	return std::max(
		1, static_cast<int>(std::lround(std::min(rate, ToMegabits(maximum)) * 1000000.0 / 8192.0)));
}
} // namespace BandwidthRateConversion
#endif

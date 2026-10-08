// This file is part of the aMule Project.
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

#ifndef UPLOADRETENTIONPOLICY_H
#define UPLOADRETENTIONPOLICY_H
#include <cstdint>
namespace BroadbandUpload
{
class SessionRetention
{
public:
	static constexpr uint64_t kMaximumExtensionMs = 120000;
	bool Retain(uint64_t now,
		bool limitReached,
		bool sustainedUnderfill,
		uint64_t rate,
		uint64_t productiveThreshold)
	{
		if (!limitReached || !sustainedUnderfill || rate < productiveThreshold || rate == 0)
			return false;
		if (!m_started) {
			m_started = true;
			m_since = now;
		}
		return now >= m_since && now - m_since < kMaximumExtensionMs;
	}

private:
	bool m_started = false;
	uint64_t m_since = 0;
};
} // namespace BroadbandUpload
#endif

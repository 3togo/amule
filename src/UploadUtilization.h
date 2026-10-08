// This file is part of the aMule Project.
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

#ifndef UPLOADUTILIZATION_H
#define UPLOADUTILIZATION_H
#include <algorithm>
#include <cstdint>
namespace BroadbandUpload
{
constexpr uint64_t kMinimumBudget = 512 * 1024;
constexpr uint64_t kUnderfillDelayMs = 10000;
class Utilization
{
public:
	void Update(uint64_t now, uint64_t budget, uint64_t rate)
	{
		const bool reset = !m_initialized || budget != m_budget || now < m_lastUpdate ||
				   now - m_lastUpdate > 5000;
		m_initialized = true;
		m_budget = budget;
		m_rate = rate;
		m_lastUpdate = now;
		const uint64_t margin = std::max<uint64_t>(1024, budget / 20);
		const bool low = budget >= kMinimumBudget && rate < budget - margin;
		if (reset || !low || !m_underfilled)
			m_since = now;
		m_underfilled = low;
	}
	bool Sustained(uint64_t now) const
	{
		return m_underfilled && now >= m_since && now >= m_lastUpdate && now - m_lastUpdate <= 5000 &&
		       now - m_since >= kUnderfillDelayMs;
	}
	bool Underfilled() const { return m_underfilled; }
	uint64_t Age(uint64_t now) const { return m_underfilled && now >= m_since ? now - m_since : 0; }
	uint64_t Budget() const { return m_budget; }
	uint64_t Rate() const { return m_rate; }

private:
	bool m_initialized = false;
	bool m_underfilled = false;
	uint64_t m_budget = 0, m_rate = 0, m_lastUpdate = 0, m_since = 0;
};
} // namespace BroadbandUpload
#endif

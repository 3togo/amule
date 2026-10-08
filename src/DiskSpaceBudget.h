// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstdint>
#include <map>
#include <string>

// A snapshot budget. Reservations are all-or-nothing across the two volumes.
class CDiskSpaceBudget
{
public:
	void Set(const std::string &volume, uint64_t free, uint64_t floor)
	{
		m_available.emplace(volume, free > floor ? free - floor : 0);
	}
	bool Reserve(
		const std::string &temp, uint64_t growth, const std::string &incoming, uint64_t copyBytes)
	{
		const auto t = m_available.find(temp), i = m_available.find(incoming);
		if (t == m_available.end() || i == m_available.end())
			return false;
		if (t->second < growth || (temp != incoming && i->second < copyBytes))
			return false;
		t->second -= growth;
		if (temp != incoming)
			i->second -= copyBytes;
		return true;
	}
	void AccountActive(const std::string &volume, uint64_t bytes)
	{
		auto it = m_available.find(volume);
		if (it != m_available.end())
			it->second = bytes >= it->second ? 0 : it->second - bytes;
	}

private:
	std::map<std::string, uint64_t> m_available;
};

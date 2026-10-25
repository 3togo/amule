// Copyright (c) 2026 aMule Team
// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef WRITEBUFFERBUDGET_H
#define WRITEBUFFERBUDGET_H

#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>

// Credits cover only buffered, unwritten payload. Already requested responses
// remain admissible during pressure; each fragment releases its own charge when
// written and freed, or discarded. Failed writes keep their charge for retry.
class WriteBufferBudget
{
	struct Account
	{
		std::string volume;
		uint64_t bytes = 0;
	};
	struct State
	{
		std::mutex mutex;
		std::map<std::string, Account> paths;
		uint64_t used = 0;
		bool globalPressured = false;
		std::map<std::string, bool> volumePressured;
	};

public:
	static constexpr uint64_t kGlobalLimit = 64 * 1024 * 1024;
	static constexpr uint64_t kVolumeLimit = 16 * 1024 * 1024;
	class Lease
	{
	public:
		~Lease()
		{
			std::lock_guard<std::mutex> lock(m_state->mutex);
			m_state->used -= m_bytes;
			m_state->paths.at(m_path).bytes -= m_bytes;
		}
		Lease(const Lease &) = delete;
		Lease &operator=(const Lease &) = delete;

		Lease(std::shared_ptr<State> state, std::string path, uint64_t bytes)
		: m_state(std::move(state))
		, m_path(std::move(path))
		, m_bytes(bytes)
		{
		}

	private:
		std::shared_ptr<State> m_state;
		std::string m_path;
		uint64_t m_bytes;
	};
	using Credit = std::shared_ptr<Lease>;

	explicit WriteBufferBudget(uint64_t global = kGlobalLimit, uint64_t volume = kVolumeLimit)
	: m_global(global)
	, m_volume(volume)
	{
	}
	static WriteBufferBudget &Get()
	{
		static WriteBufferBudget budget;
		return budget;
	}

	Credit Charge(const std::string &path, uint64_t bytes)
	{
		std::lock_guard<std::mutex> lock(m_state->mutex);
		auto &account = m_state->paths[path];
		auto lease = std::make_shared<Lease>(m_state, path, bytes);
		account.bytes += bytes;
		m_state->used += bytes;
		return lease;
	}

	// Worker-side discovery only. Never migrate live credits between two known
	// volumes (e.g. a remount); wait until the previous ownership drains.
	bool Identify(const std::string &path, const std::string &volume)
	{
		if (volume.empty()) {
			return false;
		}
		std::lock_guard<std::mutex> lock(m_state->mutex);
		auto &account = m_state->paths[path];
		if (account.bytes != 0 && !account.volume.empty() && account.volume != volume) {
			return false;
		}
		account.volume = volume;
		return true;
	}
	uint64_t Used() const
	{
		std::lock_guard<std::mutex> lock(m_state->mutex);
		return m_state->used;
	}
	// Global pressure flushes every volume; local pressure flushes only paths
	// that can share the constrained volume, including unresolved identities.
	bool IsPressured(const std::string &path) const { return CheckPressure(path, false); }
	bool ShouldFlush(const std::string &path) const { return CheckPressure(path, true); }

private:
	bool CheckPressure(const std::string &path, bool earlyFlush) const
	{
		std::lock_guard<std::mutex> lock(m_state->mutex);
		const bool global =
			earlyFlush ? m_state->used >= m_global / 8
				   : UpdatePressure(m_state->used, m_global, m_state->globalPressured);
		std::map<std::string, uint64_t> volumes;
		for (const auto &entry : m_state->paths) {
			volumes[entry.second.volume] += entry.second.bytes;
		}
		const auto found = m_state->paths.find(path);
		const std::string volume = found == m_state->paths.end() ? "" : found->second.volume;
		uint64_t known = 0;
		if (volume.empty()) {
			for (const auto &entry : volumes) {
				if (!entry.first.empty()) {
					known = std::max(known, entry.second);
				}
			}
		} else {
			known = volumes[volume];
		}
		const bool local =
			earlyFlush ? volumes[""] + known >= m_volume / 8
				   : UpdatePressure(
					     volumes[""] + known, m_volume, m_state->volumePressured[volume]);
		return global || local;
	}

	// Stop selection at the budget, and resume below half. Flush much earlier
	// to leave room for arrivals between main-thread scheduling ticks.
	static bool UpdatePressure(uint64_t used, uint64_t limit, bool &pressured)
	{
		if (used >= limit) {
			pressured = true;
		} else if (used <= limit / 2) {
			pressured = false;
		}
		return pressured;
	}
	uint64_t m_global;
	uint64_t m_volume;
	std::shared_ptr<State> m_state = std::make_shared<State>();
};
#endif

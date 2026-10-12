//								-*- C++ -*-
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

#ifndef AMULE_BACKGROUNDLOAD_H
#define AMULE_BACKGROUNDLOAD_H
#include <atomic>
#include <chrono>
#include <future>
#include <string>
namespace Kademlia
{
// The worker owns its result until Take succeeds on the main thread. Destruction
// requests cancellation and joins before releasing the captured state.
template <class T> class CBackgroundLoad
{
public:
	enum class State
	{
		Loading,
		Ready,
		Failed,
		Cancelled
	};
	template <class F>
	explicit CBackgroundLoad(F work)
	: m_future(std::async(std::launch::async, [this, work]() { return work(m_cancel); }))
	{
	}
	~CBackgroundLoad()
	{
		Cancel();
		if (m_future.valid()) {
			m_future.wait();
		}
	}
	CBackgroundLoad(const CBackgroundLoad &) = delete;
	CBackgroundLoad &operator=(const CBackgroundLoad &) = delete;
	void Cancel() { m_cancel.store(true); }
	bool Take(T &result)
	{
		if (m_state != State::Loading ||
			m_future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
			return false;
		}
		try {
			T loaded = m_future.get();
			if (m_cancel.load()) {
				m_state = State::Cancelled;
				return false;
			}
			result = std::move(loaded);
			m_state = State::Ready;
			return true;
		} catch (const std::exception &error) {
			m_error = error.what();
		} catch (...) {
			m_error = "unknown index loading error";
		}
		m_state = m_cancel.load() ? State::Cancelled : State::Failed;
		return false;
	}
	State GetState() const { return m_state; }
	const std::string &Error() const { return m_error; }

private:
	std::atomic<bool> m_cancel{ false };
	std::future<T> m_future;
	State m_state = State::Loading;
	std::string m_error;
};
} // namespace Kademlia
#endif

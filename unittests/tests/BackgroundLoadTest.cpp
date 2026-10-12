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

#include <muleunit/test.h>
#include <kademlia/utils/BackgroundLoad.h>
#include <thread>
#include <stdexcept>
using namespace muleunit;
using Task = Kademlia::CBackgroundLoad<int>;
DECLARE_SIMPLE(BackgroundLoad)
TEST(BackgroundLoad, WorkerDoesNotPublishUntilTaken)
{
	std::promise<void> started, release;
	auto gate = release.get_future().share();
	Task task([&](const std::atomic<bool> &) {
		started.set_value();
		gate.wait();
		return 42;
	});
	started.get_future().wait();
	int result = 7;
	ASSERT_FALSE(task.Take(result));
	ASSERT_EQUALS(7, result);
	release.set_value();
	while (!task.Take(result)) {
		std::this_thread::yield();
	}
	ASSERT_EQUALS(42, result);
	ASSERT_FALSE(task.Take(result));
	ASSERT_TRUE(task.GetState() == Task::State::Ready);
}
TEST(BackgroundLoad, FailedLoadDoesNotReplaceLiveState)
{
	Task task([](const std::atomic<bool> &) -> int { throw std::runtime_error("truncated index"); });
	int result = 7;
	while (task.GetState() == Task::State::Loading) {
		task.Take(result);
		std::this_thread::yield();
	}
	ASSERT_EQUALS(7, result);
	ASSERT_TRUE(task.GetState() == Task::State::Failed);
	ASSERT_EQUALS(std::string("truncated index"), task.Error());
}
TEST(BackgroundLoad, DestructionCancelsAndJoinsWorker)
{
	std::atomic<bool> finished{ false };
	{
		Task task([&](const std::atomic<bool> &cancel) {
			while (!cancel.load()) {
				std::this_thread::yield();
			}
			finished.store(true);
			return 42;
		});
	}
	ASSERT_TRUE(finished.load());
}
TEST(BackgroundLoad, CancelledResultIsDiscarded)
{
	Task task([](const std::atomic<bool> &cancel) {
		while (!cancel.load()) {
			std::this_thread::yield();
		}
		return 42;
	});
	task.Cancel();
	int result = 7;
	while (task.GetState() == Task::State::Loading) {
		task.Take(result);
		std::this_thread::yield();
	}
	ASSERT_EQUALS(7, result);
	ASSERT_TRUE(task.GetState() == Task::State::Cancelled);
}

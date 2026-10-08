// This file is part of the aMule Project.
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

#pragma once
#include <wx/thread.h>
// The disk worker may hold a block-list mutex throughout a synchronous read.
// Main-loop accounting defers when it is busy; it must not wait for disk I/O.
class UploadSnapshotLock
{
public:
	explicit UploadSnapshotLock(wxMutex &mutex, bool wait = false)
	: m_mutex(mutex)
	, m_locked((wait ? mutex.Lock() : mutex.TryLock()) == wxMUTEX_NO_ERROR)
	{
	}
	~UploadSnapshotLock()
	{
		if (m_locked)
			m_mutex.Unlock();
	}
	explicit operator bool() const { return m_locked; }
	UploadSnapshotLock(const UploadSnapshotLock &) = delete;
	UploadSnapshotLock &operator=(const UploadSnapshotLock &) = delete;

private:
	wxMutex &m_mutex;
	bool m_locked;
};

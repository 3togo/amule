// This file is part of aMule, licensed under the GNU GPL version 2 or later.
#ifndef EC_USER_EVENTS_H
#define EC_USER_EVENTS_H

#include "ECPacket.h"
#include <deque>
#include <mutex>
#include <utility>
#include <vector>

// Values, never a command: execution uses only the GUI's local preferences.
struct CUserEventData
{
	wxString key;
	std::vector<std::pair<wxString, wxString>> variables;
};

wxString ExpandUserEventCommand(const wxString &command, const CUserEventData &data);

// A bounded process-local history. Independent readers supply their own cursor.
class CUserEventStream
{
public:
	static constexpr size_t Capacity = 256;
	void Add(const CUserEventData &data);
	uint64 Latest() const;
	CECPacket Read(uint64 after, bool synchronize = false) const;

private:
	mutable std::mutex m_mutex;
	uint64 m_latest = 0;
	std::deque<std::pair<uint64, CUserEventData>> m_events;
};

class CUserEventCursor
{
public:
	void Reset(uint64 baseline) { m_last = baseline; }
	uint64 Last() const { return m_last; }
	// A repeated reply cannot return the same event twice. Malformed data is ignored.
	std::vector<CUserEventData> Read(const CECPacket &packet);

private:
	uint64 m_last = 0;
};

// Only poll while a GUI command is enabled. Changes to the enabled event set
// establish a fresh baseline before executing anything, including in-flight replies.
class CUserEventSubscription
{
public:
	// Login already supplies a baseline; enabled commands can poll from it directly.
	// Preference changes leave enabled at zero and synchronize on the next poll.
	void Reset(uint64 baseline, uint32 enabled = 0);
	void Abort() { m_pending = m_discardPending = false; }
	bool BeginPoll(uint32 enabled, bool supported, CECPacket &request);
	std::vector<CUserEventData> Read(const CECPacket &packet, uint32 enabled);

private:
	CUserEventCursor m_cursor;
	uint32 m_active = 0;
	uint32 m_requested = 0;
	bool m_pending = false;
	bool m_discardPending = false;
	bool m_synchronizing = false;
};

#endif

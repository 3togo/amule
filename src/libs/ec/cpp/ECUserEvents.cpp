// This file is part of aMule, licensed under the GNU GPL version 2 or later.
#include "ECUserEvents.h"

#include <algorithm>
#include <set>

namespace
{
bool ValidRemoteEvent(const CUserEventData &data)
{
	std::set<wxString> expected;
	if (data.key == "DownloadCompleted") {
		expected = { "FILE", "NAME", "HASH", "SIZE", "DLACTIVETIME" };
	} else if (data.key == "ErrorOnCompletion") {
		expected = { "FILE" };
	} else if (data.key == "OutOfDiskSpace") {
		expected = { "PARTITION" };
	} else {
		return false;
	}
	for (const auto &variable : data.variables) {
		if (expected.erase(variable.first) != 1) {
			return false;
		}
	}
	return expected.empty();
}
} // namespace

wxString ExpandUserEventCommand(const wxString &command, const CUserEventData &data)
{
	wxString result;
	for (size_t pos = 0; pos < command.length();) {
		bool replaced = false;
		if (command[pos] == '%') {
			for (const auto &variable : data.variables) {
				if (command.Mid(pos + 1).StartsWith(variable.first)) {
					result += variable.second;
					pos += variable.first.length() + 1;
					replaced = true;
					break;
				}
			}
		}
		if (!replaced) {
			result += command[pos++];
		}
	}
	return result;
}

void CUserEventStream::Add(const CUserEventData &data)
{
	if (!ValidRemoteEvent(data)) {
		return;
	}
	std::lock_guard<std::mutex> lock(m_mutex);
	m_events.emplace_back(++m_latest, data);
	if (m_events.size() > Capacity) {
		m_events.pop_front();
	}
}

uint64 CUserEventStream::Latest() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_latest;
}

CECPacket CUserEventStream::Read(uint64 after, bool synchronize) const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	CECPacket packet(EC_OP_USER_EVENTS);
	packet.AddTag(CECTag(EC_TAG_USER_EVENT_CURSOR, m_latest));
	if (synchronize) {
		return packet;
	}
	for (const auto &event : m_events) {
		if (event.first <= after) {
			continue;
		}
		CECTag tag(EC_TAG_USER_EVENT, event.first);
		tag.AddTag(CECTag(EC_TAG_USER_EVENT_KEY, event.second.key));
		for (const auto &variable : event.second.variables) {
			CECTag value(EC_TAG_USER_EVENT_VARIABLE, variable.first);
			value.AddTag(CECTag(EC_TAG_USER_EVENT_VALUE, variable.second));
			tag.AddTag(value);
		}
		packet.AddTag(tag);
	}
	return packet;
}

std::vector<CUserEventData> CUserEventCursor::Read(const CECPacket &packet)
{
	std::vector<CUserEventData> result;
	const CECTag *cursor = packet.GetTagByName(EC_TAG_USER_EVENT_CURSOR);
	if (packet.GetOpCode() != EC_OP_USER_EVENTS || !cursor || !cursor->IsInt() ||
		cursor->GetInt() < m_last) {
		return result;
	}
	const uint64 latest = cursor->GetInt();
	for (const CECTag &tag : packet) {
		if (tag.GetTagName() != EC_TAG_USER_EVENT || !tag.IsInt() || tag.GetInt() <= m_last ||
			tag.GetInt() > latest) {
			continue;
		}
		const CECTag *key = tag.GetTagByName(EC_TAG_USER_EVENT_KEY);
		if (!key || !key->IsString()) {
			continue;
		}
		CUserEventData data{ key->GetStringData(), {} };
		bool valid = true;
		for (const CECTag &variable : tag) {
			if (variable.GetTagName() != EC_TAG_USER_EVENT_VARIABLE) {
				continue;
			}
			const CECTag *value = variable.GetTagByName(EC_TAG_USER_EVENT_VALUE);
			if (!variable.IsString() || !value || !value->IsString()) {
				valid = false;
				break;
			}
			data.variables.emplace_back(variable.GetStringData(), value->GetStringData());
		}
		if (valid && ValidRemoteEvent(data)) {
			result.push_back(std::move(data));
			m_last = tag.GetInt();
		}
	}
	m_last = latest;
	return result;
}

void CUserEventSubscription::Reset(uint64 baseline)
{
	m_cursor.Reset(baseline);
	m_active = 0;
	// A preference change can happen while the previous request is in flight.
	// Drain its reply before issuing a new request with this same handler.
	m_discardPending = m_pending;
}

bool CUserEventSubscription::BeginPoll(uint32 enabled, bool supported, CECPacket &request)
{
	if (!enabled || !supported) {
		m_active = 0;
		return false;
	}
	if (m_pending) {
		return false;
	}
	m_synchronizing = m_active != enabled;
	request = CECPacket(EC_OP_GET_USER_EVENTS);
	if (!m_synchronizing) {
		request.AddTag(CECTag(EC_TAG_USER_EVENT_CURSOR, m_cursor.Last()));
	}
	m_requested = enabled;
	m_pending = true;
	return true;
}

std::vector<CUserEventData> CUserEventSubscription::Read(const CECPacket &packet, uint32 enabled)
{
	if (!m_pending) {
		return {};
	}
	m_pending = false;
	if (m_discardPending) {
		m_discardPending = false;
		return {};
	}
	if (enabled != m_requested || packet.GetOpCode() != EC_OP_USER_EVENTS) {
		m_active = 0;
		return {};
	}
	if (m_synchronizing) {
		const CECTag *cursor = packet.GetTagByName(EC_TAG_USER_EVENT_CURSOR);
		if (cursor && cursor->IsInt()) {
			m_cursor.Reset(cursor->GetInt());
			m_active = enabled;
		}
		return {};
	}
	return m_cursor.Read(packet);
}

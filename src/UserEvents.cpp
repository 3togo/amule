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

#include "UserEvents.h"
#include <ec/cpp/ECUserEvents.h>

#include <common/Format.h>
#include "AppImageEnv.h" // Needed for GetSanitizedExecEnv
#include "Logger.h"
#include "Preferences.h"
#include "PartFile.h"
#include "TerminationProcess.h" // Needed for CTerminationProcess

#include <wx/process.h>
#include <wx/utils.h> // Needed for wxExecuteEnv

#define USEREVENTS_EVENT(ID, NAME, VARS) { #ID, NAME, false, "", false, "" },
static struct
{
	const wxString key;
	const wxString name;
	bool core_enabled;
	wxString core_command;
	bool gui_enabled;
	wxString gui_command;
} s_EventList[] = {
	USEREVENTS_EVENTLIST()
	/* This macro expands to initialise the list of user event types. Example:
	   { "NewChatSession", wxTRANSLATE("New chat session started"), false, "", false, "" }, */
};
#undef USEREVENTS_EVENT

// Always defined: the wxCHECK_* macros below evaluate their condition in both debug and release,
// unlike the wxASSERT_* family this used to be paired with. Returns false for any out-of-range
// event, so the accessors below can fall through to a safe sentinel instead of reading past
// s_EventList.
inline bool CheckIndex(const unsigned int idx)
{
	return (idx < itemsof(s_EventList));
}

unsigned int CUserEvents::GetCount()
{
	return itemsof(s_EventList);
}

// wxCHECK rather than wxASSERT in the accessors below: a release build would otherwise read past
// s_EventList on a bad index instead of aborting, wxASSERT being a no-op in release. Each accessor
// returns a safe sentinel, entry [0], when the index is out of range; the debug-build assert +
// abort on first misuse is unchanged.

const wxString &CUserEvents::GetDisplayName(enum EventType event)
{
	wxCHECK_MSG(CheckIndex(event),
		s_EventList[0].name,
		"CUserEvents::GetDisplayName: event index out of range");
	return s_EventList[event].name;
}

bool CUserEvents::IsCoreCommandEnabled(enum EventType event)
{
	wxCHECK_MSG(CheckIndex(event), false, "CUserEvents::IsCoreCommandEnabled: event index out of range");
	return s_EventList[event].core_enabled;
}

bool CUserEvents::IsGUICommandEnabled(enum EventType event)
{
	wxCHECK_MSG(CheckIndex(event), false, "CUserEvents::IsGUICommandEnabled: event index out of range");
	return s_EventList[event].gui_enabled;
}

const wxString &CUserEvents::GetKey(const unsigned int event)
{
	wxCHECK_MSG(CheckIndex(event), s_EventList[0].key, "CUserEvents::GetKey: event index out of range");
	return s_EventList[event].key;
}

bool &CUserEvents::GetCoreEnableVar(const unsigned int event)
{
	wxCHECK_MSG(CheckIndex(event),
		s_EventList[0].core_enabled,
		"CUserEvents::GetCoreEnableVar: event index out of range");
	return s_EventList[event].core_enabled;
}

wxString &CUserEvents::GetCoreCommandVar(const unsigned int event)
{
	wxCHECK_MSG(CheckIndex(event),
		s_EventList[0].core_command,
		"CUserEvents::GetCoreCommandVar: event index out of range");
	return s_EventList[event].core_command;
}

bool &CUserEvents::GetGUIEnableVar(const unsigned int event)
{
	wxCHECK_MSG(CheckIndex(event),
		s_EventList[0].gui_enabled,
		"CUserEvents::GetGUIEnableVar: event index out of range");
	return s_EventList[event].gui_enabled;
}

wxString &CUserEvents::GetGUICommandVar(const unsigned int event)
{
	wxCHECK_MSG(CheckIndex(event),
		s_EventList[0].gui_command,
		"CUserEvents::GetGUICommandVar: event index out of range");
	return s_EventList[event].gui_command;
}

#define USEREVENTS_EVENT(ID, NAME, VARS) \
	case CUserEvents::ID: { \
		VARS break; \
	}
#define USEREVENTS_REPLACE_VAR(VAR, DESC, CODE) data.variables.emplace_back(VAR, CODE);
static CUserEventData SnapshotEvent(enum CUserEvents::EventType event, const void *object)
{
	CUserEventData data;
	switch (event) {
		USEREVENTS_EVENTLIST()
	}
	return data;
}
#undef USEREVENTS_EVENT
#undef USEREVENTS_REPLACE_VAR

static void ExecuteCommand(enum CUserEvents::EventType event,
	const CUserEventData &data,
	const wxString &cmd,
	bool remote = false)
{
	wxString command = cmd;
	if (remote) {
		command = ExpandUserEventCommand(cmd, data);
	} else {
		// Preserve the existing replacement order for locally raised events.
		for (const auto &variable : data.variables) {
			command.Replace("%" + variable.first, variable.second);
		}
	}
	if (!command.empty()) {
		// Inside an AppImage, run the user command with a sanitized environment so it loads
		// system libraries rather than the bundled ones (#334); a no-op copy elsewhere.
		CTerminationProcess *p = new CTerminationProcess(cmd);
		wxExecuteEnv execEnv;
		const bool sanitized = AppImageEnv::GetSanitizedExecEnv(execEnv);
		if (!wxExecute(command, wxEXEC_ASYNC, p, sanitized ? &execEnv : nullptr)) {
			// If wxExecute fails, we need to delete the CTerminationProcess
			// otherwise it will leak.
			delete p;
			AddLogLineC(CFormat(_("Failed to execute command '%s' on '%s' event.")) % command %
				    s_EventList[event].name);
		}
	}
}

void CUserEvents::ProcessEvent(enum EventType event, const void *object)
{
	wxCHECK_RET(CheckIndex(event), "CUserEvents::ProcessEvent: event index out of range");
	wxCHECK_RET(object != NULL, "CUserEvents::ProcessEvent: NULL object");
	CUserEventData data = SnapshotEvent(event, object);
	data.key = s_EventList[event].key;

#ifndef CLIENT_GUI
	// Capture even when the daemon's Core command is disabled: a connected GUI
	// may independently have its own command enabled for this event.
	RemoteEvents().Add(data);
	if (s_EventList[event].core_enabled) {
		ExecuteCommand(event, data, s_EventList[event].core_command);
	}
#endif
#ifndef AMULE_DAEMON
	if (s_EventList[event].gui_enabled) {
		ExecuteCommand(event, data, s_EventList[event].gui_command);
	}
#endif
}

CUserEventStream &CUserEvents::RemoteEvents()
{
	static CUserEventStream events;
	return events;
}

void CUserEvents::ProcessRemoteEvent(const CUserEventData &data)
{
#ifndef AMULE_DAEMON
	for (unsigned int i = 0; i < GetCount(); ++i) {
		if (data.key == s_EventList[i].key && s_EventList[i].gui_enabled) {
			ExecuteCommand(static_cast<EventType>(i), data, s_EventList[i].gui_command, true);
			return;
		}
	}
#else
	(void)data;
#endif
}

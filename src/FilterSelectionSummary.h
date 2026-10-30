//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
//

#ifndef AMULE_FILTER_SELECTION_SUMMARY_H
#define AMULE_FILTER_SELECTION_SUMMARY_H

#include <wx/arrstr.h>
#include <wx/intl.h>
#include <common/StringFunctions.h> // Needed for SplitKeywords

namespace FilterSelectionSummary
{
struct Settings
{
	bool messages = false;
	bool allMessages = false;
	bool nonFriends = false;
	bool unknownClients = false;
	bool messageWords = false;
	wxString messageTerms;
	bool advancedSpamSupported = true;
	bool advancedSpam = false;
	bool captcha = false;
	bool logMessages = false;
	bool comments = false;
	wxString commentTerms;
};

inline wxString Join(const wxArrayString &parts, const wxString &separator)
{
	wxString text;
	for (const auto &part : parts) {
		if (!text.empty()) {
			text += separator;
		}
		text += part;
	}
	return text;
}

inline wxString Describe(const Settings &settings)
{
	wxArrayString messages;
	const auto messageTerms = SplitKeywords(settings.messageTerms);
	// The legacy '*' shortcut tests the complete, untrimmed message list.
	const bool all = settings.messages &&
			 (settings.allMessages || (settings.messageWords && settings.messageTerms == "*"));
	if (!settings.messages) {
		messages.Add(_("The message filter is off; its selected rules are saved."));
	} else if (all) {
		messages.Add(_("All incoming messages outside current chats will be discarded."));
	} else {
		if (settings.nonFriends) {
			messages.Add(_("Messages from people not on your friend list will be discarded."));
		}
		if (settings.unknownClients) {
			messages.Add(_("Messages from unknown clients will be discarded."));
		}
		if (settings.messageWords && !messageTerms.empty()) {
			messages.Add(wxString::Format(_("Messages containing any of these terms will be "
							"discarded (ignoring case): %s."),
				Join(messageTerms, ", ")));
		}
		if (messages.empty()) {
			messages.Add(settings.messageWords ? _("No messages will be discarded by the message "
							       "filter: the word list is empty.")
							   : _("No messages will be discarded by the message "
							       "filter: select at least one option."));
		} else {
			messages.Add(_("Current chats are exempt from these rules."));
		}
	}
	if (settings.advancedSpamSupported && settings.advancedSpam && !all) {
		messages.Add(_("Advanced spam filtering for non-friends is also enabled."));
		if (settings.captcha) {
			messages.Add(_("First messages from non-friends require a CAPTCHA."));
		}
	}
	if (settings.logMessages) {
		messages.Add(_("Received message text will also be written to the log."));
	}

	wxString comments;
	const auto commentTerms = SplitKeywords(settings.commentTerms);
	if (!settings.comments) {
		comments = _("File comment filtering is off.");
	} else if (commentTerms.empty()) {
		comments = _("No file comments will be hidden: the word list is empty.");
	} else {
		comments = wxString::Format(_("File comments containing any of these literal terms will be "
					      "hidden (ignoring case): %s."),
			Join(commentTerms, ", "));
	}
	return Join(messages, " ") + "\n\n" + comments;
}
} // namespace FilterSelectionSummary

#endif

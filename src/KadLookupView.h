// Copyright (c) 2026 aMule Team
// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef KAD_LOOKUP_VIEW_H
#define KAD_LOOKUP_VIEW_H

#include <wx/wx.h>
#include <wx/weakref.h>

// Parent-owned, modeless snapshot: connection controls stay accessible while reading.
class CKadLookupView final : public wxDialog
{
	wxTextCtrl *m_text;

public:
	explicit CKadLookupView(wxWindow *parent)
	: wxDialog(parent,
		  wxID_ANY,
		  _("Kad lookup diagnostics"),
		  wxDefaultPosition,
		  parent->FromDIP(wxSize(850, 500)),
		  wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
	{
		auto *sizer = new wxBoxSizer(wxVERTICAL);
		m_text = new wxTextCtrl(this,
			wxID_ANY,
			wxEmptyString,
			wxDefaultPosition,
			wxDefaultSize,
			wxTE_MULTILINE | wxTE_READONLY | wxHSCROLL);
		sizer->Add(m_text, 1, wxEXPAND | wxALL, FromDIP(8));
		sizer->Add(CreateButtonSizer(wxOK), 0, wxEXPAND | wxALL, FromDIP(8));
		SetSizer(sizer);
		Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { Destroy(); }, wxID_OK);
		Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent &) { Destroy(); });
	}
	void SetSnapshot(const wxString &text) { m_text->ChangeValue(text); }
	wxString GetSnapshot() const { return m_text->GetValue(); }
};

#ifdef CLIENT_GUI
#include "libs/ec/cpp/RemoteConnect.h"

class CKadLookupReply final : public CECPacketHandlerBase
{
	wxWeakRef<CKadLookupView> m_view;
	wxWeakRef<wxButton> m_button;

public:
	CKadLookupReply(CKadLookupView *view, wxButton *button)
	: m_view(view)
	, m_button(button)
	{
	}
	~CKadLookupReply() override
	{
		if (m_button) {
			m_button->Enable();
		}
	}
	void HandlePacket(const CECPacket *packet) override
	{
		const auto *tag = packet->GetTagByName(EC_TAG_STRING);
		if (m_view) {
			m_view->SetSnapshot(
				packet->GetOpCode() == EC_OP_GET_KAD_LOOKUPS && tag && tag->IsString()
					? tag->GetStringData()
					: _("The core does not support Kad lookup diagnostics."));
		}
		delete this;
	}
	void AbortPendingRequest() override
	{
		if (m_view) {
			m_view->SetSnapshot(
				_("Connection lost. Reconnect and request lookup diagnostics again."));
		}
		delete this;
	}
};
#endif
#endif

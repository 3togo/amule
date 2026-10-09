// Copyright (c) 2026 aMule Team
// SPDX-License-Identifier: GPL-2.0-or-later
#include "KadLookupView.h"
#include <stdexcept>
#include <iostream>

class LookupTestApp : public wxApp
{
public:
	bool OnInit() override { return true; }
};
wxIMPLEMENT_APP_NO_MAIN(LookupTestApp);
static void Check(bool value, const char *message)
{
	if (!value) {
		throw std::runtime_error(message);
	}
}
static void VerifyLifecycle()
{
	auto *frame = new wxFrame(nullptr, wxID_ANY, "Lookup lifecycle test");
	auto *button = new wxButton(frame, wxID_ANY, "Request");
	auto *view = new CKadLookupView(frame);
	view->Show();
	Check(!view->IsModal(), "Diagnostics blocks connection controls");
	button->Disable();
	(new CKadLookupReply(view, button))->AbortPendingRequest();
	Check(button->IsEnabled(), "Abort prevents retry");
	Check(view->GetSnapshot().Contains("Connection lost"), "Abort leaves stale snapshot");
	CECPacket unsupported(EC_OP_FAILED);
	button->Disable();
	(new CKadLookupReply(view, button))->HandlePacket(&unsupported);
	Check(button->IsEnabled(), "Unsupported core prevents retry");
	Check(view->GetSnapshot().Contains("does not support"), "Unsupported core is not explained");
	CECPacket valid(EC_OP_GET_KAD_LOOKUPS);
	valid.AddTag(CECTag(EC_TAG_STRING, wxString("Snapshot")));
	(new CKadLookupReply(view, button))->HandlePacket(&valid);
	Check(view->GetSnapshot() == "Snapshot", "Valid reply missing");
	CECPacket malformed(EC_OP_GET_KAD_LOOKUPS);
	malformed.AddTag(CECTag(EC_TAG_STRING, uint32_t(42)));
	(new CKadLookupReply(view, button))->HandlePacket(&malformed);
	Check(view->GetSnapshot().Contains("does not support"), "Malformed reply interpreted as text");
	auto *closedReply = new CKadLookupReply(view, button);
	delete view; // response may arrive after the parent has destroyed the view
	closedReply->HandlePacket(&valid);
	Check(button->IsEnabled(), "Closed view prevents retry");
	view = new CKadLookupView(frame);
	auto *parentReply = new CKadLookupReply(view, button);
	delete frame;
	parentReply->AbortPendingRequest(); // both weak references must already be invalid
}
int main(int argc, char **argv)
{
	if (!wxEntryStart(argc, argv)) {
		return 77;
	}
	int result = 0;
	if (!wxTheApp->CallOnInit()) {
		result = 1;
	} else {
		try {
			VerifyLifecycle();
		} catch (const std::exception &e) {
			std::cerr << e.what() << '\n';
			result = 1;
		}
		wxTheApp->OnExit();
	}
	wxEntryCleanup();
	return result;
}

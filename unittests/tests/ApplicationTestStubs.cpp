// This file is part of the aMule Project.
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

#include <amule.h>
#include <cstdio>
#include <cstdlib>
#ifdef ENABLE_UPNP
#include <UPnPBase.h>
#endif

// Emit genuine application vtables/RTTI for headless production-code tests.
// Lifetimes are permitted for fixtures; entering the real application is not.
namespace
{
[[noreturn]] void UnexpectedApplicationCall(const char *method) noexcept
{
	std::fprintf(stderr, "Headless fixture unexpectedly called %s\n", method);
	std::abort();
}
} // namespace

CamuleAppCommon::CamuleAppCommon() = default;
CamuleAppCommon::~CamuleAppCommon() = default;
CamuleApp::CamuleApp() = default;
CamuleApp::~CamuleApp() = default;

bool CamuleApp::OnInit()
{
	UnexpectedApplicationCall("CamuleApp::OnInit");
}
int CamuleApp::OnExit()
{
	UnexpectedApplicationCall("CamuleApp::OnExit");
}
#if wxUSE_ON_FATAL_EXCEPTION
void CamuleApp::OnFatalException()
{
	UnexpectedApplicationCall("CamuleApp::OnFatalException");
}
#endif
void CamuleApp::OnUnhandledException()
{
	UnexpectedApplicationCall("CamuleApp::OnUnhandledException");
}
void CamuleApp::OnAssertFailure(const wxChar *, int, const wxChar *, const wxChar *, const wxChar *)
{
	UnexpectedApplicationCall("CamuleApp::OnAssertFailure");
}
void CamuleApp::EnableIP2Country(bool, bool)
{
	UnexpectedApplicationCall("CamuleApp::EnableIP2Country");
}
int CamuleApp::InitGui(bool, wxString &)
{
	UnexpectedApplicationCall("CamuleApp::InitGui");
}

uint32 CamuleApp::GetPublicIP(bool) const
{
	UnexpectedApplicationCall("CamuleApp::GetPublicIP");
}

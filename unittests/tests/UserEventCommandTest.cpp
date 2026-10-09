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

#include <muleunit/test.h>

#include "ExternalCommand.h"

#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/utils.h>

#ifdef __WINDOWS__
#include <wx/msw/wrapwin.h>
#include <shellapi.h>
#endif

using namespace muleunit;

DECLARE_SIMPLE(UserEventCommand)

TEST(UserEventCommand, HostileValueRemainsOneArgument)
{
#ifdef __WINDOWS__
	const wxString value = "file ' $HOME `id`; & --extra=1 %HASH";
#else
	const wxString value = "file \" ' $HOME `id`; & --extra=1 %HASH";
#endif
	const auto args = ExternalCommand::Build(
		"notify --file=%FILE %HASH", { { "%FILE", value }, { "%HASH", "0123" } });
	ASSERT_EQUALS(size_t(3), args.size());
	ASSERT_EQUALS(wxString("--file=") + value, args[1]);
	ASSERT_EQUALS(wxString("0123"), args[2]);
}

TEST(UserEventCommand, QuotesBelongToTemplate)
{
#ifdef __WINDOWS__
	const wxString command = "\"C:\\Program Files\\notify.exe\" \"fixed argument\" \"%NAME\"";
	const wxString executable = "C:\\Program Files\\notify.exe";
#else
	const wxString command = "'/opt/my tools/notify' 'fixed argument' '%NAME'";
	const wxString executable = "/opt/my tools/notify";
#endif
#ifdef __WINDOWS__
	const wxString value = "punctuation & and spaces";
#else
	const wxString value = "quotes \" and spaces";
#endif
	const auto args = ExternalCommand::Build(command, { { "%NAME", value } });
	ASSERT_EQUALS(size_t(3), args.size());
	ASSERT_EQUALS(executable, args[0]);
	ASSERT_EQUALS(wxString("fixed argument"), args[1]);
	ASSERT_EQUALS(value, args[2]);
}

TEST(UserEventCommand, EmptyAndRepeatedValues)
{
	const auto args = ExternalCommand::Build("notify %NAME %NAME:%NAME %UNKNOWN", { { "%NAME", "" } });
	ASSERT_EQUALS(size_t(4), args.size());
	ASSERT_EQUALS(wxString(""), args[1]);
	ASSERT_EQUALS(wxString(":"), args[2]);
	ASSERT_EQUALS(wxString("%UNKNOWN"), args[3]);
}

TEST(UserEventCommand, UnicodeAndBackslashesStayLiteral)
{
#ifdef __WINDOWS__
	const wxString value = wxString::FromUTF8("文件\\folder\\a b.txt");
#else
	const wxString value = wxString::FromUTF8("文件\\folder\\a \"b\".txt");
#endif
	const auto args = ExternalCommand::Build("notify %FILE", { { "%FILE", value } });
	ASSERT_EQUALS(size_t(2), args.size());
	ASSERT_EQUALS(value, args[1]);
}

TEST(UserEventCommand, EmptyTemplateDoesNotSpawn)
{
	ASSERT_TRUE(ExternalCommand::Build("", {}).IsEmpty());
	ASSERT_TRUE(ExternalCommand::Build("   ", {}).IsEmpty());
}

#ifndef __WINDOWS__
TEST(UserEventCommand, ShellPositionalArgumentIsData)
{
	const wxString output = wxFileName::CreateTempFileName("amule-event-command-");
	const wxString value = "\"; exit 99; # $(exit 99) `exit 99` & %HASH\nsecond line";
	const auto args = ExternalCommand::Build("sh -c 'printf \"%s\" \"$1\" > \"$2\"' _ %FILE %OUTPUT",
		{ { "%FILE", value }, { "%OUTPUT", output } });
	ASSERT_EQUALS(size_t(6), args.size());
	std::vector<wxWCharBuffer> buffers;
	std::vector<const wchar_t *> argv;
	buffers.reserve(args.size());
	for (const auto &arg : args) {
		buffers.emplace_back(arg.wc_str());
		argv.push_back(buffers.back().data());
	}
	argv.push_back(nullptr);
	const long status = wxExecute(argv.data(), wxEXEC_SYNC);
	wxString received;
	{
		wxFFile file(output, "rb");
		ASSERT_TRUE(file.IsOpened());
		ASSERT_TRUE(file.ReadAll(&received));
	}
	wxRemoveFile(output);
	ASSERT_EQUALS(0L, status);
	ASSERT_EQUALS(value, received);
}

TEST(UserEventCommand, DetachedShellReceivesMultipleHostileValues)
{
	const wxString output = wxFileName::CreateTempFileName("amule-event-detached-");
	const wxString file = "a\"; exit 99; # $(exit 99) `exit 99` & %HASH";
	const wxString name = "name with spaces ' \\ $HOME ; | %FILE";
	const wxString hash = "0123456789";
	const auto args = ExternalCommand::Build(
		"sh -c 'printf \"%s|%s|%s\" \"$1\" \"$2\" \"$3\" > \"$4\"' _ %FILE %HASH %NAME %OUTPUT",
		{ { "%FILE", file }, { "%HASH", hash }, { "%NAME", name }, { "%OUTPUT", output } });
	ASSERT_TRUE(ExternalCommand::RunDetached("hostile positional values", args));
	wxString received;
	for (unsigned attempt = 0; attempt < 100; ++attempt) {
		wxFFile result(output, "rb");
		if (result.IsOpened()) {
			result.ReadAll(&received);
		}
		if (received == file + "|" + hash + "|" + name) {
			break;
		}
		wxMilliSleep(10);
	}
	wxRemoveFile(output);
	ASSERT_EQUALS(file + "|" + hash + "|" + name, received);
}
#endif

TEST(UserEventCommand, WindowsEscapesTrailingBackslashes)
{
	wxArrayString args;
	args.Add("notify.exe");
	args.Add("C:\\folder with spaces\\");
	args.Add("next");
	ASSERT_EQUALS(wxString("\"notify.exe\" \"C:\\folder with spaces\\\\\" \"next\""),
		ExternalCommand::BuildWindowsCommandLine(args));
}

TEST(UserEventCommand, WindowsEscapesBackslashesBeforeQuotes)
{
	wxArrayString args;
	args.Add("notify.exe");
	args.Add("sender\\\" --extra");
	args.Add("");
	ASSERT_EQUALS(wxString("\"notify.exe\" \"sender\\\\\\\" --extra\" \"\""),
		ExternalCommand::BuildWindowsCommandLine(args));
}

#ifdef __WINDOWS__
TEST(UserEventCommand, WindowsNativeArgumentRoundTrip)
{
	wxArrayString args;
	args.Add("C:\\Program Files\\notify.exe");
	args.Add("C:\\folder with spaces\\");
	args.Add("sender\\\" --extra");
	args.Add("two\\\\\"quotes\"");
	args.Add("");
	int count = 0;
	wchar_t **parsed =
		CommandLineToArgvW(ExternalCommand::BuildWindowsCommandLine(args).wc_str(), &count);
	ASSERT_TRUE(parsed != nullptr);
	wxArrayString received;
	for (int i = 0; i < count; ++i) {
		received.Add(parsed[i]);
	}
	LocalFree(parsed);
	ASSERT_EQUALS(args.size(), received.size());
	for (size_t i = 0; i < args.size(); ++i) {
		ASSERT_EQUALS(args[i], received[i]);
	}
}
#endif

TEST(UserEventCommand, PlayerAliasesExpandOnce)
{
	bool substituted = false;
	const auto args = ExternalCommand::Build("player %PARTFILE %PARTNAME $file",
		{ { "%PARTFILE", "x%PARTNAME.avi" },
			{ "%PARTNAME", "name with spaces" },
			{ "$file", "x%PARTNAME.avi" } },
		&substituted);
	ASSERT_TRUE(substituted);
	ASSERT_EQUALS(size_t(4), args.size());
	ASSERT_EQUALS(wxString("x%PARTNAME.avi"), args[1]);
	ASSERT_EQUALS(wxString("name with spaces"), args[2]);
	ASSERT_EQUALS(wxString("x%PARTNAME.avi"), args[3]);
	ExternalCommand::Build("player --fullscreen", {}, &substituted);
	ASSERT_FALSE(substituted);
}

#ifdef __WINDOWS__
TEST(UserEventCommand, RejectQuotesBeforeCmdOrBatchCanInterpretThem)
{
	for (const wxString &value : { wxString("x\"&echo INJECTED"),
		     wxString("x\\\"&echo INJECTED"),
		     wxString("\""),
		     wxString("x\"\r\necho INJECTED") }) {
		for (const wxString &command : { wxString("cmd /c echo %SENDER"),
			     wxString("on-chat.bat %SENDER"),
			     wxString("on-chat.cmd fixed%SENDER"),
			     wxString("notify.exe %SENDER %SENDER") }) {
			ASSERT_TRUE(ExternalCommand::Build(command, { { "%SENDER", value } }).IsEmpty());
		}
	}
	// Unused event values do not affect a command containing only fixed arguments.
	ASSERT_EQUALS(size_t(2),
		ExternalCommand::Build("notify.exe fixed", { { "%SENDER", "x\"&echo INJECTED" } }).size());
}
#endif

TEST(UserEventCommand, EmptyArgumentsDoNotSpawn)
{
	ASSERT_FALSE(ExternalCommand::RunDetached("empty", {}));
	wxArrayString args;
	args.Add("");
	args.Add("ignored");
	ASSERT_FALSE(ExternalCommand::RunDetached("empty executable", args));
}

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

#include "UserEventCommand.h"

#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/utils.h>

using namespace muleunit;

DECLARE_SIMPLE(UserEventCommand)

TEST(UserEventCommand, HostileValueRemainsOneArgument)
{
	const wxString value = "file \" ' $HOME `id`; & --extra=1 %HASH";
	const auto args = BuildUserEventCommand(
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
	const auto args = BuildUserEventCommand(command, { { "%NAME", "quotes \" and spaces" } });
	ASSERT_EQUALS(size_t(3), args.size());
	ASSERT_EQUALS(executable, args[0]);
	ASSERT_EQUALS(wxString("fixed argument"), args[1]);
	ASSERT_EQUALS(wxString("quotes \" and spaces"), args[2]);
}

TEST(UserEventCommand, EmptyAndRepeatedValues)
{
	const auto args = BuildUserEventCommand("notify %NAME %NAME:%NAME %UNKNOWN", { { "%NAME", "" } });
	ASSERT_EQUALS(size_t(4), args.size());
	ASSERT_EQUALS(wxString(""), args[1]);
	ASSERT_EQUALS(wxString(":"), args[2]);
	ASSERT_EQUALS(wxString("%UNKNOWN"), args[3]);
}

TEST(UserEventCommand, UnicodeAndBackslashesStayLiteral)
{
	const wxString value = wxString::FromUTF8("文件\\folder\\a \"b\".txt");
	const auto args = BuildUserEventCommand("notify %FILE", { { "%FILE", value } });
	ASSERT_EQUALS(size_t(2), args.size());
	ASSERT_EQUALS(value, args[1]);
}

TEST(UserEventCommand, EmptyTemplateDoesNotSpawn)
{
	ASSERT_TRUE(BuildUserEventCommand("", {}).IsEmpty());
	ASSERT_TRUE(BuildUserEventCommand("   ", {}).IsEmpty());
}

#ifndef __WINDOWS__
TEST(UserEventCommand, ShellPositionalArgumentIsData)
{
	const wxString output = wxFileName::CreateTempFileName("amule-event-command-");
	const wxString value = "\"; exit 99; # $(exit 99) `exit 99` & %HASH\nsecond line";
	const auto args = BuildUserEventCommand("sh -c 'printf \"%s\" \"$1\" > \"$2\"' _ %FILE %OUTPUT",
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
#endif

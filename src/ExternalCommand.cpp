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

#include "ExternalCommand.h"
#include "AppImageEnv.h"
#include "TerminationProcess.h"

#include <wx/cmdline.h>
#include <wx/utils.h>

namespace ExternalCommand
{

wxArrayString Build(
	const wxString &command, const std::vector<std::pair<wxString, wxString>> &values, bool *substituted)
{
	if (substituted != nullptr) {
		*substituted = false;
	}
#ifdef __WINDOWS__
	wxArrayString args = wxCmdLineParser::ConvertStringToArgs(command, wxCMD_LINE_SPLIT_DOS);
#else
	wxArrayString args = wxCmdLineParser::ConvertStringToArgs(command, wxCMD_LINE_SPLIT_UNIX);
#endif
	for (wxString &arg : args) {
		wxString expanded;
		for (size_t i = 0; i < arg.length();) {
			bool replaced = false;
			if (arg[i] == '%' || arg[i] == '$') {
				for (const auto &value : values) {
					if (!value.first.empty() &&
						arg.Mid(i, value.first.length()) == value.first) {
#ifdef __WINDOWS__
						// CRT escaping cannot protect a quote from cmd.exe or a batch
						// file. Reject the command rather than change the event data.
						if (value.second.Find('"') != wxNOT_FOUND) {
							return {};
						}
#endif
						if (substituted != nullptr) {
							*substituted = true;
						}
						expanded += value.second;
						i += value.first.length();
						replaced = true;
						break;
					}
				}
			}
			if (!replaced) {
				expanded += arg[i++];
			}
		}
		arg = expanded;
	}
	return args;
}

// Windows CreateProcess accepts a string, and wxWidgets 3.2/3.3.1's argv overload
// does not double backslashes before embedded quotes or the closing quote.
// Quote each argument using the Windows C runtime rules instead.
wxString BuildWindowsCommandLine(const wxArrayString &args)
{
	wxString command;
	for (const wxString &arg : args) {
		if (!command.empty()) {
			command += ' ';
		}
		command += '"';
		size_t backslashes = 0;
		for (const wxUniChar ch : arg) {
			if (ch == '\\') {
				++backslashes;
				continue;
			}
			const size_t count = ch == '"' ? backslashes * 2 + 1 : backslashes;
			command += wxString('\\', count);
			command += ch;
			backslashes = 0;
		}
		command += wxString('\\', backslashes * 2);
		command += '"';
	}
	return command;
}

// Preserve non-ASCII arguments and use host libraries when running inside an AppImage.
bool RunDetached(const wxString &description, const wxArrayString &args)
{
	if (args.IsEmpty() || args[0].empty()) {
		return false;
	}
#ifndef __WINDOWS__
	std::vector<wxWCharBuffer> buffers;
	std::vector<const wchar_t *> argv;
	buffers.reserve(args.size()); // no reallocation, so the pointers below stay valid
	argv.reserve(args.size() + 1);
	for (const wxString &arg : args) {
		buffers.emplace_back(arg.wc_str());
		argv.push_back(buffers.back().data());
	}
	argv.push_back(nullptr);
#endif

	wxExecuteEnv execEnv;
	const bool sanitized = AppImageEnv::GetSanitizedExecEnv(execEnv);
	CTerminationProcess *process = new CTerminationProcess(description);
	long ret = 0;
	try {
#ifdef __WINDOWS__
		ret = wxExecute(
			BuildWindowsCommandLine(args), wxEXEC_ASYNC, process, sanitized ? &execEnv : nullptr);
#else
		ret = wxExecute(argv.data(), wxEXEC_ASYNC, process, sanitized ? &execEnv : nullptr);
#endif
	} catch (...) {
		// wxExecute throws, rather than returning an error, when the environment it has to
		// hand the child is unusable -- a working directory that no longer exists, for one.
		// Uncaught it unwinds out of the menu handler and wx terminates the application
		// cleanly: no signal, no backtrace, no crash report.
		delete process;
		return false;
	}
	if (ret <= 0) {
		delete process;
		return false;
	}
	// True means the child was spawned, not that it did anything useful: an async wxExecute
	// returns the pid as soon as the fork succeeds, so a failed exec (no xdg-open installed,
	// say) is not reported here. macOS and Windows do surface a failure, since neither reaches
	// the desktop opener this way.
	return true;
}

} // namespace ExternalCommand

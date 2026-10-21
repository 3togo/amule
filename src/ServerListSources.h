// This file is part of the aMule Project.
// Copyright (c) 2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

#ifndef SERVERLISTSOURCES_H
#define SERVERLISTSOURCES_H

#include <wx/arrstr.h>
#include <wx/uri.h>
#include <wx/ffile.h>
#include <wx/convauto.h>
#include <wx/filefn.h>
#include <wx/tokenzr.h>

namespace ServerListSources
{
inline bool IsValid(const wxString &url)
{
	const wxURI uri(url);
	const wxString scheme = uri.GetScheme().Lower();
	return url.Find('\n') == wxNOT_FOUND && url.Find('\r') == wxNOT_FOUND &&
	       url.Find('\0') == wxNOT_FOUND && url.Find(' ') == wxNOT_FOUND &&
	       url.Find('\t') == wxNOT_FOUND && uri.HasServer() && !uri.GetServer().empty() &&
	       (scheme == "http" || scheme == "https");
}

// Read on each new update, so changes made by the editor or a daemon administrator
// take effect without restarting. Never create or rewrite the user's file.
inline bool Read(const wxString &path, wxArrayString &sources)
{
	sources.clear();
	if (!wxFileExists(path)) {
		return false;
	}
	wxFFile file(path, "rb");
	wxString contents;
	if (!file.IsOpened() || !file.ReadAll(&contents, wxConvAuto())) {
		return false;
	}
	wxStringTokenizer lines(contents, "\r\n");
	while (lines.HasMoreTokens()) {
		wxString url = lines.GetNextToken();
		url.Trim().Trim(false);
		if (IsValid(url) && sources.Index(url) == wxNOT_FOUND) {
			sources.Add(url);
		}
	}
	return true;
}

inline wxArrayString Read(const wxString &path)
{
	wxArrayString sources;
	Read(path, sources);
	return sources;
}
} // namespace ServerListSources

#endif // SERVERLISTSOURCES_H

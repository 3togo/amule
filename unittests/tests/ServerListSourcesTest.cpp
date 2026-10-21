// This file is part of the aMule Project.
// Copyright (c) 2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

#include <muleunit/test.h>
#include <ServerListSources.h>
#include <common/TextFile.h>
#include <wx/filename.h>
#include <wx/filefn.h>

using namespace muleunit;
DECLARE_SIMPLE(ServerListSources)

namespace
{
struct SourceFile
{
	wxString path = wxFileName::CreateTempFileName("amule-sources-");
	~SourceFile() { wxRemoveFile(path); }
	void Write(const wxString &text) const
	{
		wxFFile file(path, "wb");
		ASSERT_TRUE(file.Write(text));
	}
};
} // namespace

TEST(ServerListSources, FiltersCommentsInvalidSchemesAndDuplicatesInOrder)
{
	SourceFile file;
	file.Write(" # preserved comment\r\n  https://one.example/server.met \r\n\r\n"
		   "ftp://unsupported.example/server.met\nnot a URL\nhttp:///missing-host\n"
		   "http://two.example/list\nhttps://one.example/server.met\n");
	const auto sources = ServerListSources::Read(file.path);
	ASSERT_EQUALS(size_t(2), sources.size());
	ASSERT_EQUALS(wxString("https://one.example/server.met"), sources[0]);
	ASSERT_EQUALS(wxString("http://two.example/list"), sources[1]);
	CTextFile original;
	ASSERT_TRUE(original.Open(file.path, CTextFile::read));
	ASSERT_EQUALS(wxString(" # preserved comment"), original.GetNextLine());
}

TEST(ServerListSources, ReloadsEditsAndDoesNotLimitSourcesTo255)
{
	SourceFile file;
	wxString text;
	for (unsigned i = 0; i < 300; ++i) {
		text += wxString::Format("https://source.example/%u\n", i);
	}
	file.Write(text);
	ASSERT_EQUALS(size_t(300), ServerListSources::Read(file.path).size());
	file.Write("https://changed.example/list\n");
	const auto changed = ServerListSources::Read(file.path);
	ASSERT_EQUALS(size_t(1), changed.size());
	ASSERT_EQUALS(wxString("https://changed.example/list"), changed[0]);
	file.Write("");
	ASSERT_TRUE(ServerListSources::Read(file.path).empty());
}

TEST(ServerListSources, MissingFileDoesNotCreateDefaults)
{
	SourceFile file;
	ASSERT_TRUE(wxRemoveFile(file.path));
	ASSERT_TRUE(ServerListSources::Read(file.path).empty());
	ASSERT_FALSE(wxFileExists(file.path));
	file.Write(""); // Restore for RAII cleanup.
}

TEST(ServerListSources, RejectsLineInjectionAndWhitespace)
{
	ASSERT_FALSE(ServerListSources::IsValid("https://example.com/list\nhttp://injected/list"));
	ASSERT_FALSE(ServerListSources::IsValid("https://example.com/list\r"));
	ASSERT_FALSE(ServerListSources::IsValid("https://example.com/a b"));
	ASSERT_FALSE(ServerListSources::IsValid("https://example.com/\t"));
	ASSERT_TRUE(ServerListSources::IsValid("HTTPS://example.com/a%20b?token=value"));
}

TEST(ServerListSources, DirectoryDoesNotHangTheReader)
{
	SourceFile file;
	ASSERT_TRUE(wxRemoveFile(file.path));
	ASSERT_TRUE(wxMkdir(file.path));
	ASSERT_TRUE(ServerListSources::Read(file.path).empty());
	ASSERT_TRUE(wxRmdir(file.path));
	file.Write("");
}

TEST(ServerListSources, LegacyCommentEncodingDoesNotHideValidURLs)
{
	SourceFile file;
	{
		wxFFile raw(file.path, "wb");
		const char contents[] = "# caf\xe9\nhttps://example.com/list\n";
		ASSERT_EQUALS(sizeof(contents) - 1, raw.Write(contents, sizeof(contents) - 1));
	}
	const auto sources = ServerListSources::Read(file.path);
	ASSERT_EQUALS(size_t(1), sources.size());
	ASSERT_EQUALS(wxString("https://example.com/list"), sources[0]);
}

TEST(ServerListSources, ReadsUTF8BOM)
{
	SourceFile file;
	{
		wxFFile raw(file.path, "wb");
		const char contents[] = "\xef\xbb\xbfhttps://example.com/list\n";
		ASSERT_EQUALS(sizeof(contents) - 1, raw.Write(contents, sizeof(contents) - 1));
	}
	const auto sources = ServerListSources::Read(file.path);
	ASSERT_EQUALS(size_t(1), sources.size());
	ASSERT_EQUALS(wxString("https://example.com/list"), sources[0]);
}

TEST(ServerListSources, ReadStatusDistinguishesEmptyFromUnreadable)
{
	SourceFile file;
	wxArrayString sources;
	ASSERT_TRUE(ServerListSources::Read(file.path, sources));
	ASSERT_TRUE(sources.empty());
	ASSERT_TRUE(wxRemoveFile(file.path));
	ASSERT_FALSE(ServerListSources::Read(file.path, sources));
	file.Write("");
}

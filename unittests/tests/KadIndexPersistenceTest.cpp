// This file is part of the aMule Project.
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

#include <muleunit/test.h>
#include "kademlia/utils/IndexPersistence.h"
#include <wx/filefn.h>
#include <wx/filename.h>
using namespace muleunit;
DECLARE_SIMPLE(KadIndexPersistence)
namespace
{
struct Fixture
{
	CPath path;
	Fixture()
	: path(wxFileName::CreateTempFileName("amule-kad-index"))
	{
		CFile file(path, CFile::write);
		file.WriteUInt32(42);
		file.Close();
	}
	~Fixture()
	{
		wxRemoveFile(path.GetRaw());
		wxRemoveFile(path.AppendExt(".new").GetRaw());
	}
	uint32_t Read()
	{
		CFile file(path, CFile::read);
		return file.ReadUInt32();
	}
};
} // namespace
TEST(KadIndexPersistence, CompletedImageReplacesOldIndex)
{
	Fixture f;
	ASSERT_TRUE(Kademlia::SaveIndexFile(f.path, [](CFile &file) { file.WriteUInt32(100); }));
	ASSERT_EQUALS(uint32_t(100), f.Read());
	ASSERT_FALSE(f.path.AppendExt(".new").FileExists());
}
TEST(KadIndexPersistence, InterruptedSerializationPreservesOldIndex)
{
	Fixture f;
	ASSERT_RAISES(CIOFailureException, Kademlia::SaveIndexFile(f.path, [](CFile &file) {
		file.WriteUInt32(100);
		throw CIOFailureException("Injected serialization failure");
	}));
	ASSERT_EQUALS(uint32_t(42), f.Read());
	// A later successful save recovers, even with the abandoned candidate present.
	ASSERT_TRUE(Kademlia::SaveIndexFile(f.path, [](CFile &file) { file.WriteUInt32(200); }));
	ASSERT_EQUALS(uint32_t(200), f.Read());
}
TEST(KadIndexPersistence, CandidateOpenFailurePreservesOldIndex)
{
	Fixture f;
	const wxString candidate = f.path.AppendExt(".new").GetRaw();
	ASSERT_TRUE(wxMkdir(candidate));
	ASSERT_FALSE(Kademlia::SaveIndexFile(f.path, [](CFile &file) { file.WriteUInt32(100); }));
	ASSERT_EQUALS(uint32_t(42), f.Read());
	wxRmdir(candidate);
}

#ifdef __UNIX__
TEST(KadIndexPersistence, PromotionFailurePreservesOldIndex)
{
	Fixture f;
	ASSERT_FALSE(Kademlia::SaveIndexFile(f.path, [&](CFile &file) {
		file.WriteUInt32(100);
		// The fd remains open, but there is no candidate pathname to promote.
		ASSERT_TRUE(wxRemoveFile(f.path.AppendExt(".new").GetRaw()));
	}));
	ASSERT_EQUALS(uint32_t(42), f.Read());
}
#endif

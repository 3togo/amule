// This file is part of the aMule Project.
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

#include <muleunit/test.h>
#include "kademlia/utils/IndexPersistence.h"
#include <wx/filefn.h>
#include <wx/filename.h>
#ifdef __linux__
#include <sys/stat.h>
#include <unistd.h>
#endif
using namespace muleunit;
DECLARE_SIMPLE(KadIndexPersistence)
namespace
{
struct Fixture
{
	CPath path;
	Fixture(const wxString &prefix = "amule-kad-index")
	: path(wxFileName::CreateTempFileName(prefix))
	{
		CFile file(path, CFile::write);
		file.WriteUInt32(42);
		file.Close();
	}
	~Fixture()
	{
		if (path.FileExists())
			wxRemoveFile(path.GetRaw());
		if (path.AppendExt(".new").FileExists())
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

#ifdef __linux__
TEST(KadIndexPersistence, CrossFilesystemReplacementDoesNotCopyOverOldIndex)
{
	// Linux CI normally provides tmpfs here. The native rename must fail across
	// filesystems instead of copying and truncating the previous index.
	if (!wxDirExists("/dev/shm"))
		return;
	Fixture oldIndex;
	Fixture candidate("/dev/shm/amule-kad-index");
	struct stat oldStat, candidateStat;
	ASSERT_EQUALS(0, ::stat(oldIndex.path.GetRaw().fn_str(), &oldStat));
	ASSERT_EQUALS(0, ::stat(candidate.path.GetRaw().fn_str(), &candidateStat));
	if (oldStat.st_dev == candidateStat.st_dev)
		return;
	{
		CFile file(candidate.path, CFile::write);
		file.WriteUInt32(100);
		file.Close();
	}
	ASSERT_FALSE(CPath::ReplaceFileAtomically(candidate.path, oldIndex.path));
	ASSERT_EQUALS(uint32_t(42), oldIndex.Read());
	ASSERT_EQUALS(uint32_t(100), candidate.Read());
}
#endif

#ifdef __linux__
TEST(KadIndexPersistence, BufferedWriteFailureDoesNotPromote)
{
	if (::access("/dev/full", W_OK) != 0)
		return;
	Fixture f;
	const CPath candidate = f.path.AppendExt(".new");
	ASSERT_EQUALS(0, ::symlink("/dev/full", candidate.GetRaw().fn_str()));
	ASSERT_RAISES(CIOFailureException,
		Kademlia::SaveIndexFile(f.path, [](CFile &file) { file.WriteUInt32(100); }));
	ASSERT_EQUALS(uint32_t(42), f.Read());
	ASSERT_TRUE(wxRemoveFile(candidate.GetRaw()));
}
#endif

// This file is part of the aMule Project.
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

#ifndef KADINDEXPERSISTENCE_H
#define KADINDEXPERSISTENCE_H
#include "CFile.h"
namespace Kademlia
{
// Keep the old index until the complete new image has been flushed and closed.
// On an exception CFile's destructor closes the candidate without promotion.
// The on-disk format and each index's independent lifetime remain unchanged.
template <typename Writer> bool SaveIndexFile(const CPath &path, Writer writer)
{
	CFile file;
	if (!file.Open(path, CFile::write_safe))
		return false;
	writer(file);
	if (!file.Flush())
		return false;
	return file.Close();
}
} // namespace Kademlia
#endif

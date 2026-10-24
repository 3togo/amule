// Copyright (c) 2026 aMule Team
// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef PARTFILEBUFFERING_H
#define PARTFILEBUFFERING_H

#include <algorithm>
#include <list>

// Keep pending writes in ascending file-offset order. Equal ends retain arrival order.
template <typename Entry> void InsertPartFileBufferedWrite(std::list<Entry *> &buffers, Entry *item)
{
	const auto position = std::find_if(buffers.begin(), buffers.end(), [item](const Entry *queued) {
		return item->end < queued->end;
	});
	buffers.insert(position, item);
}
#endif

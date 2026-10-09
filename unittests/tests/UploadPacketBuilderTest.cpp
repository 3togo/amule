
//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#include "UploadPacketBuilder.h"
#include "Packet.h"
#include "MemFile.h"
#include "MD4Hash.h"
#include "ScopedPtr.h"
#include <protocol/Protocols.h>
#include <protocol/ed2k/Client2Client/TCP.h>
#include <protocol/ed2k/Constants.h>
#include <algorithm>
#include <cstring>
#include <vector>
#include <zlib.h>

using namespace muleunit;

// Frozen pre-refactor builders: the oracle compares complete wire packets, rather than
// reproducing the new assembler's header and splitting decisions in assertions.
namespace Legacy
{
// eMule 0.70b ref: CUploadDiskIOThread::CreateStandardPackets()
uint32 Standard(const uint8_t *buffer,
	uint64 startOffset,
	uint64 endOffset,
	CPacketList &packetList,
	const uint8_t *fileHash,
	uint32 uploadDatarate)
{
	uint32 overhead = 0;
	uint32 togo = (uint32)(endOffset - startOffset);

	CMemFile memfile(buffer, togo);
	// Adaptive chunk size: scale with per-slot speed, floor 10 KiB, ceil EMBLOCKSIZE. /8 is
	// ~125 ms of data per chunk, enough to saturate a TCP segment burst without making per-
	// packet latency awful on slow peers, and the floor keeps it sane while uploadDatarate is
	// still 0. Going past EMBLOCKSIZE buys nothing, since the receiver requests blocks of
	// exactly that size.
	const uint32 chunkSize = std::min(std::max(uploadDatarate / 8u, 10240u), (uint32)EMBLOCKSIZE);
	uint32 nPacketSize = (togo <= chunkSize + 2600u) ? togo : chunkSize;

	while (togo) {
		if (togo < nPacketSize * 2) {
			nPacketSize = togo;
		}

		wxASSERT(nPacketSize);
		togo -= nPacketSize;

		uint64 endpos = (endOffset - togo);
		uint64 startpos = endpos - nPacketSize;

		bool bLargeBlocks = (startpos > 0xFFFFFFFF) || (endpos > 0xFFFFFFFF);

		CMemFile data(nPacketSize + 16 + 2 * (bLargeBlocks ? 8 : 4));
		data.WriteHash(CMD4Hash(fileHash));
		if (bLargeBlocks) {
			data.WriteUInt64(startpos);
			data.WriteUInt64(endpos);
		} else {
			data.WriteUInt32(startpos);
			data.WriteUInt32(endpos);
		}
		char *tempbuf = new char[nPacketSize];
		memfile.Read(tempbuf, nPacketSize);
		data.Write(tempbuf, nPacketSize);
		delete[] tempbuf;
		CPacket *packet = new CPacket(data,
			(bLargeBlocks ? OP_EMULEPROT : OP_EDONKEYPROT),
			(bLargeBlocks ? (uint8)OP_SENDINGPART_I64 : (uint8)OP_SENDINGPART));
		overhead += 16 + 2 * (bLargeBlocks ? 8 : 4);
		packetList.push_back(std::make_pair(packet, nPacketSize));
	}
	return overhead;
}

// eMule 0.70b ref: CUploadDiskIOThread::CreatePackedPackets()
uint32 Packed(const uint8_t *buffer,
	uint64 startOffset,
	uint64 endOffset,
	CPacketList &packetList,
	const uint8_t *fileHash,
	uint32 uploadDatarate)
{
	uint32 overhead = 0;
	uint32 togo = (uint32)(endOffset - startOffset);
	uLongf newsize = togo + 300;
	CScopedArray<uint8_t> output(newsize);
	// eMule 0.70b: use compression level 1 instead of 9 -- for typical 10240-byte
	// blocks the size difference is small (~4-12%) but level 1 is 1.5-2.5x faster.
	uint16 result = compress2(output.get(), &newsize, buffer, togo, 1);
	if (result != Z_OK || togo <= newsize) {
		return Standard(buffer, startOffset, endOffset, packetList, fileHash, uploadDatarate);
	}

	CMemFile memfile(output.get(), newsize);

	uint32 totalPayloadSize = 0;
	uint32 oldSize = togo;
	togo = newsize;
	// Adaptive chunk size -- see CreateStandardPackets for rationale.
	const uint32 chunkSize = std::min(std::max(uploadDatarate / 8u, 10240u), (uint32)EMBLOCKSIZE);
	uint32 nPacketSize = (togo <= chunkSize + 2600u) ? togo : chunkSize;

	while (togo) {
		if (togo < nPacketSize * 2) {
			nPacketSize = togo;
		}
		togo -= nPacketSize;

		bool isLargeBlock = (startOffset > 0xFFFFFFFF) || (endOffset > 0xFFFFFFFF);

		CMemFile data(nPacketSize + 16 + (isLargeBlock ? 12 : 8));
		data.WriteHash(CMD4Hash(fileHash));
		if (isLargeBlock) {
			data.WriteUInt64(startOffset);
		} else {
			data.WriteUInt32(startOffset);
		}
		data.WriteUInt32(newsize);
		char *tempbuf = new char[nPacketSize];
		memfile.Read(tempbuf, nPacketSize);
		data.Write(tempbuf, nPacketSize);
		delete[] tempbuf;
		CPacket *packet = new CPacket(
			data, OP_EMULEPROT, (isLargeBlock ? OP_COMPRESSEDPART_I64 : OP_COMPRESSEDPART));

		uint32 payloadSize =
			static_cast<uint32>((static_cast<uint64>(nPacketSize) * oldSize) / newsize);

		if (togo == 0 && totalPayloadSize + payloadSize < oldSize) {
			payloadSize = oldSize - totalPayloadSize;
		}

		totalPayloadSize += payloadSize;

		overhead += 24;
		packetList.push_back(std::make_pair(packet, payloadSize));
	}
	return overhead;
}
} // namespace Legacy

namespace
{
struct Packets
{
	CPacketList list;
	~Packets()
	{
		for (auto &entry : list) {
			delete entry.first;
		}
	}
};

void Compare(const std::vector<uint8_t> &input, uint64 start, uint32 rate, bool packed)
{
	const uint8_t hash[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
	auto source = input;
	Packets expected, actual;
	const uint64 end = start + input.size();
	const uint8_t empty = 0;
	const uint8_t *original = input.empty() ? &empty : input.data();
	const uint8_t *buffer = source.empty() ? &empty : source.data();
	const uint32 oldOverhead = packed ? Legacy::Packed(original, start, end, expected.list, hash, rate)
					  : Legacy::Standard(original, start, end, expected.list, hash, rate);
	const uint32 newOverhead =
		packed ? UploadPacketBuilder::Packed(buffer, start, end, actual.list, hash, rate)
		       : UploadPacketBuilder::Standard(buffer, start, end, actual.list, hash, rate);
	ASSERT_EQUALS(oldOverhead, newOverhead);
	ASSERT_TRUE(source == input);
	ASSERT_EQUALS(expected.list.size(), actual.list.size());
	auto reference = expected.list.begin();
	uint32 total = 0;
	for (auto &entry : actual.list) {
		CPacket &oldPacket = *reference->first;
		CPacket &newPacket = *entry.first;
		ASSERT_EQUALS(reference->second, entry.second);
		ASSERT_EQUALS(oldPacket.GetRealPacketSize(), newPacket.GetRealPacketSize());
		ASSERT_EQUALS(oldPacket.IsFromPF(), newPacket.IsFromPF());
		ASSERT_TRUE(std::memcmp(oldPacket.GetPacket(),
				    newPacket.GetPacket(),
				    oldPacket.GetRealPacketSize()) == 0);
		// Packet-owned storage must survive changes to the input and be detachable.
		std::fill(source.begin(), source.end(), 0);
		uint8_t *detached = newPacket.DetachPacket();
		ASSERT_TRUE(std::memcmp(oldPacket.GetPacket(), detached, oldPacket.GetRealPacketSize()) == 0);
		delete[] detached;
		total += entry.second;
		++reference;
	}
	ASSERT_EQUALS(static_cast<uint32>(input.size()), total);
}

std::vector<uint8_t> Noise(size_t size)
{
	std::vector<uint8_t> data(size);
	uint32 state = 0x12345678;
	for (auto &byte : data) {
		state ^= state << 13;
		state ^= state >> 17;
		state ^= state << 5;
		byte = static_cast<uint8_t>(state);
	}
	return data;
}
} // namespace

DECLARE_SIMPLE(UploadPacketBuilder)

TEST(UploadPacketBuilder, StandardWireCompatibility)
{
	for (size_t size : { 0u,
		     1u,
		     10239u,
		     10240u,
		     12840u,
		     12841u,
		     20479u,
		     20480u,
		     20481u,
		     EMBLOCKSIZE,
		     3u * EMBLOCKSIZE }) {
		const auto data = Noise(size);
		for (uint64 start : { uint64(0),
			     uint64(0xFFFFFFFF) - data.size(),
			     uint64(0xFFFFFFFF) - 20000,
			     uint64(0xFFFFFFFF),
			     uint64(0x100000000) }) {
			for (uint32 rate : { 0u, 81920u, 200000u, 1474560u, 0xFFFFFFFFu }) {
				Compare(data, start, rate, false);
			}
		}
	}
}

TEST(UploadPacketBuilder, CompressedWireCompatibility)
{
	auto pattern = Noise(24000);
	std::vector<uint8_t> data(EMBLOCKSIZE);
	for (size_t i = 0; i < data.size(); ++i) {
		data[i] = pattern[i % pattern.size()];
	}
	// Repeated noise compresses to several packets, exercising proportional rounding.
	for (const auto &input : { std::vector<uint8_t>(EMBLOCKSIZE, 'A'), data }) {
		for (uint64 start : { uint64(0),
			     uint64(0xFFFFFFFF) - EMBLOCKSIZE + 1,
			     uint64(0xFFFFFFFF),
			     uint64(0x100000000) }) {
			for (uint32 rate : { 0u, 81920u, 200000u, 1474560u, 0xFFFFFFFFu }) {
				Compare(input, start, rate, true);
			}
		}
	}
}

TEST(UploadPacketBuilder, IncompressibleFallback)
{
	for (size_t size : { 0u, 1u, 12841u, EMBLOCKSIZE }) {
		const auto data = Noise(size);
		for (uint64 start : { uint64(0), uint64(0xFFFFFFFF) - 10000, uint64(0x100000000) }) {
			for (uint32 rate : { 0u, 200000u, 0xFFFFFFFFu }) {
				Compare(data, start, rate, true);
			}
		}
	}
}

TEST(UploadPacketBuilder, CompressedAccountingAndAppend)
{
	const auto pattern = Noise(24000);
	std::vector<uint8_t> data(EMBLOCKSIZE);
	for (size_t i = 0; i < data.size(); ++i) {
		data[i] = pattern[i % pattern.size()];
	}
	const uint8_t hash[16] = {};
	Packets packets;
	UploadPacketBuilder::Packed(data.data(), 0, data.size(), packets.list, hash, 0);
	ASSERT_TRUE(packets.list.size() > 1);
	uint32 total = 0;
	for (const auto &entry : packets.list) {
		ASSERT_EQUALS(static_cast<uint8>(OP_COMPRESSEDPART), entry.first->GetOpCode());
		total += entry.second;
	}
	ASSERT_EQUALS(static_cast<uint32>(data.size()), total);
	CPacket *first = packets.list.front().first;
	const auto count = packets.list.size();
	UploadPacketBuilder::Standard(data.data(), 0, 1, packets.list, hash, 0);
	ASSERT_EQUALS(count + 1, packets.list.size());
	ASSERT_TRUE(first == packets.list.front().first);
}

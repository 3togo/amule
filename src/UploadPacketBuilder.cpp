
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

#include "UploadPacketBuilder.h"
#include "Packet.h"
#include "MemFile.h"
#include "MD4Hash.h"
#include "ScopedPtr.h"
#include <protocol/Protocols.h>
#include <protocol/ed2k/Constants.h>
#include <protocol/ed2k/Client2Client/TCP.h>
#include <algorithm>
#include <memory>
#include <zlib.h>

namespace
{
uint32 Assemble(const uint8_t *buffer,
	uint32 size,
	uint64 startOffset,
	uint64 endOffset,
	uint32 originalSize,
	bool compressed,
	CPacketList &packets,
	const uint8_t *fileHash,
	uint32 uploadDatarate)
{
	uint32 remaining = size;
	uint32 accounted = 0;
	uint32 overhead = 0;
	// Preserve the adaptive chunk size and tail merging of the original builders.
	const uint32 chunkSize =
		std::min(std::max(uploadDatarate / 8u, 10240u), static_cast<uint32>(EMBLOCKSIZE));
	uint32 packetSize = (remaining <= chunkSize + 2600u) ? remaining : chunkSize;
	while (remaining) {
		if (remaining < packetSize * 2) {
			packetSize = remaining;
		}
		const uint32 position = size - remaining;
		remaining -= packetSize;
		const uint64 start = compressed ? startOffset : endOffset - remaining - packetSize;
		const uint64 end = compressed ? endOffset : endOffset - remaining;
		const bool large = start > 0xFFFFFFFF || end > 0xFFFFFFFF;
		const uint32 headerSize = 16 + (large ? 8 : 4) + (compressed ? 4 : (large ? 8 : 4));
		const uint8 protocol = (compressed || large) ? OP_EMULEPROT : OP_EDONKEYPROT;
		const uint8 opcode = compressed ? (large ? OP_COMPRESSEDPART_I64 : OP_COMPRESSEDPART)
						: (large ? static_cast<uint8>(OP_SENDINGPART_I64)
							 : static_cast<uint8>(OP_SENDINGPART));
		// Only the small header uses a stream, attached to stack storage without allocations.
		uint8 header[32];
		CMemFile data(header, headerSize);
		data.WriteHash(CMD4Hash(fileHash));
		if (large) {
			data.WriteUInt64(start);
		} else {
			data.WriteUInt32(start);
		}
		if (compressed) {
			data.WriteUInt32(size);
		} else if (large) {
			data.WriteUInt64(end);
		} else {
			data.WriteUInt32(end);
		}
		auto packet = std::make_unique<CPacket>(opcode, headerSize + packetSize, protocol, false);
		packet->CopyToDataBuffer(0, header, headerSize);
		packet->CopyToDataBuffer(headerSize, buffer + position, packetSize);
		uint32 payloadSize =
			compressed
				? static_cast<uint32>((static_cast<uint64>(packetSize) * originalSize) / size)
				: packetSize;
		if (compressed && remaining == 0 && accounted + payloadSize < originalSize) {
			payloadSize = originalSize - accounted;
		}
		accounted += payloadSize;
		// The old compressed builder counts 24 even for the 28-byte I64 header.
		overhead += compressed ? 24 : headerSize;
		packets.emplace_back(packet.get(), payloadSize);
		packet.release();
	}
	return overhead;
}
} // namespace

uint32 UploadPacketBuilder::Standard(const uint8_t *buffer,
	uint64 startOffset,
	uint64 endOffset,
	CPacketList &packets,
	const uint8_t *fileHash,
	uint32 uploadDatarate)
{
	const uint32 size = static_cast<uint32>(endOffset - startOffset);
	return Assemble(buffer, size, startOffset, endOffset, size, false, packets, fileHash, uploadDatarate);
}

uint32 UploadPacketBuilder::Packed(const uint8_t *buffer,
	uint64 startOffset,
	uint64 endOffset,
	CPacketList &packets,
	const uint8_t *fileHash,
	uint32 uploadDatarate)
{
	const uint32 size = static_cast<uint32>(endOffset - startOffset);
	uLongf packedSize = size + 300;
	CScopedArray<uint8_t> output(packedSize);
	// Retain level 1 and the standard-packet fallback when compression does not help.
	if (compress2(output.get(), &packedSize, buffer, size, 1) != Z_OK || size <= packedSize) {
		return Standard(buffer, startOffset, endOffset, packets, fileHash, uploadDatarate);
	}
	return Assemble(output.get(),
		static_cast<uint32>(packedSize),
		startOffset,
		endOffset,
		size,
		true,
		packets,
		fileHash,
		uploadDatarate);
}

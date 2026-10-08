// This file is part of the aMule Project.
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

#ifndef UDPQUEUEPOLICY_H
#define UDPQUEUEPOLICY_H
#include <cstddef>
#include <cstdint>
namespace UdpQueuePolicy
{
constexpr size_t kClientPackets = 4096;
constexpr size_t kClientBytes = 4 * 1024 * 1024;
constexpr size_t kServerPackets = 1024;
constexpr size_t kServerBytes = 1024 * 1024;
// Include the UDP framing and maximum client-obfuscation overhead.
constexpr size_t kPacketOverhead = 32;
inline bool CanAccept(size_t packets, size_t bytes, size_t payload, size_t packetLimit, size_t byteLimit)
{
	return packets < packetLimit && bytes <= byteLimit && byteLimit - bytes >= kPacketOverhead &&
	       payload <= byteLimit - bytes - kPacketOverhead;
}
inline bool Expired(uint64_t queuedAt, uint64_t now, uint64_t lifetime)
{
	return now >= queuedAt && now - queuedAt >= lifetime;
}
} // namespace UdpQueuePolicy
#endif

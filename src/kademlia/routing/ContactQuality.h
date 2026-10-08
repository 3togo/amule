// This file is part of the aMule Project.
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

#ifndef KADCONTACTQUALITY_H
#define KADCONTACTQUALITY_H
#include <cstdint>
#include <ctime>
namespace Kademlia
{
struct ContactQuality
{
	bool verified;
	bool helloReceived;
	bool hasKey;
	uint8_t type;
	time_t lastResponse;
};
inline unsigned LocalContactQuality(const ContactQuality &contact, time_t now)
{
	if (contact.type >= 4)
		return 0;
	unsigned score =
		(contact.verified ? 400 : 0) + (contact.helloReceived ? 240 : 0) + (contact.hasKey ? 160 : 0);
	const unsigned typeScores[] = { 120, 90, 60, 20 };
	score += typeScores[contact.type];
	// Only an observed response earns freshness credit. GetLastSeen() is derived
	// from expiry and can be distorted by probes. A clock regression earns none.
	if (contact.lastResponse > 0 && now >= contact.lastResponse) {
		const time_t age = now - contact.lastResponse;
		if (age <= 15 * 60)
			score += 90;
		else if (age <= 60 * 60)
			score += 70;
		else if (age <= 4 * 60 * 60)
			score += 50;
		else if (age <= 12 * 60 * 60)
			score += 25;
	}
	return score;
}
constexpr time_t kMaximumProbeDelay = 30 * 60;
inline bool ContactProbeDue(uint8_t type, time_t expiry, time_t now)
{
	return type < 4 && expiry < now;
}
inline bool BetterContactProbe(
	unsigned score, time_t expiry, unsigned bestScore, time_t bestExpiry, time_t now)
{
	const time_t age = now >= expiry ? now - expiry : 0;
	const time_t bestAge = now >= bestExpiry ? now - bestExpiry : 0;
	const bool overdue = age >= kMaximumProbeDelay;
	const bool bestOverdue = bestAge >= kMaximumProbeDelay;
	if (overdue != bestOverdue)
		return overdue;
	if (overdue && age != bestAge)
		return age > bestAge;
	if (score != bestScore)
		return score < bestScore;
	return expiry < bestExpiry;
}
} // namespace Kademlia
#endif

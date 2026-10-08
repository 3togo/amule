# Endgame scheduling

The existing `eMule/Endgame` setting controls the new policy. At 90% completion,
peers at least five times slower than another active source for the same part
receive approximately ten seconds of work per block, with a 16 KiB minimum.
Tiny trailing fragments remain attached to the reservation.

At 99.9% completion, or an estimated 30 seconds remaining, faster sources get a
15-second preference window. Slower sources retain their slot during this bounded
wait and receive capped fallback work afterwards. Waiting peers retry from the
core tick once per second even when no data response is pending. Existing rarity selection stays
in place. Endgame reclamation cannot cancel a pipeline with received payload and
has a 60-second per-file cooldown.

The policy tests cover large-file arithmetic, thresholds, caps, partial payload,
and cooldowns. The daemon builds successfully. Throughput, completion-time, and
peer-fairness measurements on live transfers remain necessary before release.

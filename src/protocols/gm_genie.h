/**
 * @file gm_genie.h
 * @brief The Genie Intellicode Sub-GHz protocol, which the official firmware
 *        does not ship.
 *
 * Registering this with a SubGhzEnvironment gives GarageMate a "Genie" protocol
 * it can both decode (to learn codes off the air from a remote you own) and
 * encode (to replay a stored code). The stock firmware exposes no Genie support
 * at all, so this is what makes Genie doors possible on an unmodified Flipper.
 *
 * The decode state machine and the OOK waveform are ported from Derek Jamison's
 * Genie Recorder app (MIT licensed, https://github.com/jamisonderek/
 * flipper-zero-tutorials) -- the part that reads real hardware and cannot
 * safely be re-derived by guessing. Everything tying that app to its own .gne
 * files, thread names and next-code arithmetic is deliberately left out:
 * GarageMate captures and replays exact codes and owns sequencing itself, so
 * this protocol only ever moves a raw 64-bit value on and off the air.
 */
#pragma once

#include <lib/subghz/protocols/base.h>
#include <lib/subghz/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GM_GENIE_PROTOCOL_NAME "Genie"

/** Payload width Genie remotes send, and the only width we accept. */
#define GM_GENIE_BIT_COUNT 64

extern const SubGhzProtocol gm_protocol_genie;

#ifdef __cplusplus
}
#endif

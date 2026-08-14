/**
 * @file gm_capture.h
 * @brief Learning Genie codes off the air from a remote you own.
 *
 * The capture context stands up its own Sub-GHz receive chain -- a worker that
 * turns the raw radio edges into level/duration pairs, a receiver that runs the
 * Genie decoder over them, and an environment carrying the Genie protocol the
 * firmware itself does not provide. Whenever the decoder recognises a full
 * Genie frame, the new code is appended to the caller's sequence.
 *
 * Usage: alloc, start on a frequency, let it run while the user presses their
 * remote nearby, then stop and free. The captured count updates live and can be
 * read from the sequence at any time.
 */
#pragma once

#include "../model/gm_genie_seq.h"

#include <lib/subghz/devices/devices.h>
#include <lib/subghz/environment.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GmCapture GmCapture;

/** Allocate a capture context bound to @p device. */
GmCapture* gm_capture_alloc(const SubGhzDevice* device);

/** Free a capture context. Stops the radio first if still running. */
void gm_capture_free(GmCapture* capture);

/**
 * Begin listening on @p frequency, appending new codes to @p seq.
 *
 * @p seq must outlive the capture. Its @c frequency is set to @p frequency.
 * @return false when the frequency is not transmittable/receivable here.
 */
bool gm_capture_start(GmCapture* capture, uint32_t frequency, GmGenieSeq* seq);

/** Stop listening and release the radio. Safe to call when not started. */
void gm_capture_stop(GmCapture* capture);

/** @return codes appended since gm_capture_start(), across the whole run. */
uint16_t gm_capture_total(const GmCapture* capture);

#ifdef __cplusplus
}
#endif

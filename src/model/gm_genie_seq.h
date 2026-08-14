/**
 * @file gm_genie_seq.h
 * @brief A captured run of Genie codes for one door, and its position in that
 *        run.
 *
 * Genie's rolling code cannot be computed -- the algorithm lives in the remote.
 * So a Genie door does not generate anything; it stores a short run of real
 * codes captured over the air from a remote the user owns, and plays them back
 * one at a time. @c next is the index of the code that the next OPEN will send.
 *
 * Kept in its own file, /ext/apps_data/garagemate/genie/<door_id>.genie, so a
 * long capture never bloats the .door record and can be inspected or backed up
 * on its own.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <storage/storage.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Most codes held for one door.
 *
 * Each OPEN spends one, and re-capturing tops the run back up, so this only
 * bounds how many presses you get between learns -- not the size of Genie's
 * 65,536-code space. Sized to stay comfortably within app RAM.
 */
#define GM_GENIE_MAX_CODES 64

/** A captured run of codes plus the playback cursor. */
typedef struct {
    uint32_t frequency;
    uint16_t next;
    uint16_t count;
    uint64_t codes[GM_GENIE_MAX_CODES];
} GmGenieSeq;

/** Reset @p seq to empty at @p frequency. */
void gm_genie_seq_init(GmGenieSeq* seq, uint32_t frequency);

/** @return codes captured but not yet sent (count - next). */
uint16_t gm_genie_seq_remaining(const GmGenieSeq* seq);

/**
 * Append @p code unless it duplicates the last one captured.
 *
 * A held remote button repeats the same code many times before advancing, so
 * consecutive duplicates are the norm and are dropped here.
 *
 * @return true when a genuinely new code was stored; false on a duplicate or
 *         when the run is already full.
 */
bool gm_genie_seq_append(GmGenieSeq* seq, uint64_t code);

/** Fill @p out with the next code to send. @return false when none remain. */
bool gm_genie_seq_peek(const GmGenieSeq* seq, uint64_t* out);

/** Advance past the code that was just sent. */
void gm_genie_seq_advance(GmGenieSeq* seq);

/** Load the run for @p door_id, or initialise an empty one. */
void gm_genie_seq_load(Storage* storage, const char* door_id, GmGenieSeq* seq);

/** Persist the run for @p door_id. @return true on success. */
bool gm_genie_seq_save(Storage* storage, const char* door_id, const GmGenieSeq* seq);

/** Delete the run file for @p door_id. */
void gm_genie_seq_delete(Storage* storage, const char* door_id);

#ifdef __cplusplus
}
#endif

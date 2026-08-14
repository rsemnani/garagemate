#include "gm_genie_seq.h"
#include "gm_door.h"
#include "gm_paths.h"

#include <flipper_format/flipper_format.h>
#include <furi.h>

#define TAG "GarageMate"

#define GM_GENIE_DIR      GM_DATA_DIR "/genie"
#define GM_GENIE_FILETYPE "GarageMate Genie Sequence"
#define GM_GENIE_VERSION  1

void gm_genie_seq_init(GmGenieSeq* seq, uint32_t frequency) {
    furi_assert(seq);
    memset(seq, 0, sizeof(GmGenieSeq));
    seq->frequency = frequency;
}

uint16_t gm_genie_seq_remaining(const GmGenieSeq* seq) {
    furi_assert(seq);
    return (seq->next < seq->count) ? (seq->count - seq->next) : 0;
}

bool gm_genie_seq_append(GmGenieSeq* seq, uint64_t code) {
    furi_assert(seq);
    if(seq->count >= GM_GENIE_MAX_CODES) return false;
    if(seq->count > 0 && seq->codes[seq->count - 1] == code) return false;
    seq->codes[seq->count++] = code;
    return true;
}

bool gm_genie_seq_peek(const GmGenieSeq* seq, uint64_t* out) {
    furi_assert(seq);
    furi_assert(out);
    if(seq->next >= seq->count) return false;
    *out = seq->codes[seq->next];
    return true;
}

void gm_genie_seq_advance(GmGenieSeq* seq) {
    furi_assert(seq);
    if(seq->next < seq->count) seq->next++;
}

static void gm_genie_seq_path(const char* door_id, char* out, size_t out_size) {
    snprintf(out, out_size, "%s/%s.genie", GM_GENIE_DIR, door_id);
}

void gm_genie_seq_load(Storage* storage, const char* door_id, GmGenieSeq* seq) {
    furi_assert(storage);
    gm_genie_seq_init(seq, 0);

    char path[GM_PATH_MAX];
    gm_genie_seq_path(door_id, path, sizeof(path));

    FlipperFormat* ff = flipper_format_file_alloc(storage);
    FuriString* buffer = furi_string_alloc();

    do {
        if(!flipper_format_file_open_existing(ff, path)) break;

        uint32_t version = 0;
        if(!flipper_format_read_header(ff, buffer, &version)) break;
        if(furi_string_cmp_str(buffer, GM_GENIE_FILETYPE) != 0 || version != GM_GENIE_VERSION)
            break;

        flipper_format_rewind(ff);
        flipper_format_read_uint32(ff, "Frequency", &seq->frequency, 1);

        uint32_t next = 0, count = 0;
        flipper_format_rewind(ff);
        flipper_format_read_uint32(ff, "Next", &next, 1);
        flipper_format_rewind(ff);
        flipper_format_read_uint32(ff, "Count", &count, 1);
        if(count > GM_GENIE_MAX_CODES) count = GM_GENIE_MAX_CODES;

        // Codes are stored big-endian, 8 bytes each, in one hex blob.
        uint8_t bytes[GM_GENIE_MAX_CODES * sizeof(uint64_t)];
        flipper_format_rewind(ff);
        if(count > 0 &&
           flipper_format_read_hex(ff, "Codes", bytes, count * sizeof(uint64_t))) {
            for(uint32_t i = 0; i < count; i++) {
                uint64_t code = 0;
                for(size_t b = 0; b < sizeof(uint64_t); b++) {
                    code = (code << 8) | bytes[i * sizeof(uint64_t) + b];
                }
                seq->codes[i] = code;
            }
            seq->count = (uint16_t)count;
            seq->next = (next > count) ? (uint16_t)count : (uint16_t)next;
        }
    } while(false);

    furi_string_free(buffer);
    flipper_format_free(ff);
}

bool gm_genie_seq_save(Storage* storage, const char* door_id, const GmGenieSeq* seq) {
    furi_assert(storage);
    furi_assert(seq);

    storage_simply_mkdir(storage, GM_GENIE_DIR);

    char path[GM_PATH_MAX];
    gm_genie_seq_path(door_id, path, sizeof(path));

    FlipperFormat* ff = flipper_format_file_alloc(storage);
    bool ok = false;

    do {
        if(!flipper_format_file_open_always(ff, path)) break;
        if(!flipper_format_write_header_cstr(ff, GM_GENIE_FILETYPE, GM_GENIE_VERSION)) break;

        uint32_t frequency = seq->frequency;
        if(!flipper_format_write_uint32(ff, "Frequency", &frequency, 1)) break;

        uint32_t next = seq->next;
        if(!flipper_format_write_uint32(ff, "Next", &next, 1)) break;

        uint32_t count = seq->count;
        if(!flipper_format_write_uint32(ff, "Count", &count, 1)) break;

        if(count > 0) {
            uint8_t bytes[GM_GENIE_MAX_CODES * sizeof(uint64_t)];
            for(uint32_t i = 0; i < count; i++) {
                for(size_t b = 0; b < sizeof(uint64_t); b++) {
                    bytes[i * sizeof(uint64_t) + b] =
                        (uint8_t)(seq->codes[i] >> (56 - 8 * b));
                }
            }
            if(!flipper_format_write_hex(ff, "Codes", bytes, count * sizeof(uint64_t))) break;
        }

        ok = true;
    } while(false);

    flipper_format_free(ff);
    if(!ok) FURI_LOG_E(TAG, "Failed to save genie sequence: %s", path);
    return ok;
}

void gm_genie_seq_delete(Storage* storage, const char* door_id) {
    furi_assert(storage);
    char path[GM_PATH_MAX];
    gm_genie_seq_path(door_id, path, sizeof(path));
    storage_simply_remove(storage, path);
}

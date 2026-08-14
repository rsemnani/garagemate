/*
 * Genie Intellicode protocol for GarageMate.
 *
 * The decode state machine (gm_genie_decoder_feed) and the transmit waveform
 * (gm_genie_encoder_get_upload) are ported from Derek Jamison's Genie Recorder
 * (MIT, https://github.com/jamisonderek/flipper-zero-tutorials). Those two
 * pieces touch real hardware timing and are reproduced faithfully. The
 * serialize/deserialize glue is written for GarageMate: it moves one raw 64-bit
 * code with no next-code arithmetic, because GarageMate replays exact captured
 * codes and manages the rolling sequence in its own store.
 *
 * MIT License. Original portions Copyright (c) 2023 Derek Jamison.
 */
#include "gm_genie.h"

#include <lib/subghz/blocks/const.h>
#include <lib/subghz/blocks/decoder.h>
#include <lib/subghz/blocks/encoder.h>
#include <lib/subghz/blocks/generic.h>
#include <lib/subghz/blocks/math.h>

#include <furi.h>
#include <lib/toolbox/level_duration.h>

#define TAG "GmGenie"

/*
 * Genie timing. OOK, symbols are 200 us short / 400 us long, with a 70 us
 * matching window. A bit is a short/long pair; the order distinguishes 0 from
 * 1. Codes are 64 bits, though some remotes append a few extra status bits.
 */
static const SubGhzBlockConst gm_genie_const = {
    .te_short = 200,
    .te_long = 400,
    .te_delta = 70,
    .min_count_bit_for_found = GM_GENIE_BIT_COUNT,
};

typedef struct {
    SubGhzProtocolDecoderBase base;
    SubGhzBlockDecoder decoder;
    SubGhzBlockGeneric generic;
    uint16_t header_count;
} GmGenieDecoder;

typedef struct {
    SubGhzProtocolEncoderBase base;
    SubGhzProtocolBlockEncoder encoder;
    SubGhzBlockGeneric generic;
} GmGenieEncoder;

typedef enum {
    GenieDecoderStepReset = 0,
    GenieDecoderStepCheckPreambula,
    GenieDecoderStepSaveDuration,
    GenieDecoderStepCheckDuration,
} GmGenieDecoderStep;

/* Every frame the encoder emits is repeated this many times, as a real remote
 * does; the receiver acts on the first and ignores the rest as duplicates. */
#define GM_GENIE_TX_REPEAT 20

/* upload buffer: 11 preamble pairs + sync + 64 data pairs + status + end. */
#define GM_GENIE_UPLOAD_SIZE 256

/* ---- decoder ------------------------------------------------------------ */

static void* gm_genie_decoder_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    GmGenieDecoder* instance = malloc(sizeof(GmGenieDecoder));
    memset(instance, 0, sizeof(GmGenieDecoder));
    instance->base.protocol = &gm_protocol_genie;
    instance->generic.protocol_name = instance->base.protocol->name;
    return instance;
}

static void gm_genie_decoder_free(void* context) {
    furi_assert(context);
    free(context);
}

static void gm_genie_decoder_reset(void* context) {
    furi_assert(context);
    GmGenieDecoder* instance = context;
    instance->decoder.parser_step = GenieDecoderStepReset;
}

static void gm_genie_decoder_feed(void* context, bool level, uint32_t duration) {
    furi_assert(context);
    GmGenieDecoder* instance = context;

    switch(instance->decoder.parser_step) {
    case GenieDecoderStepReset:
        if((level) &&
           DURATION_DIFF(duration, gm_genie_const.te_short) < gm_genie_const.te_delta) {
            instance->decoder.parser_step = GenieDecoderStepCheckPreambula;
            instance->header_count++;
        }
        break;

    case GenieDecoderStepCheckPreambula:
        if((!level) &&
           (DURATION_DIFF(duration, gm_genie_const.te_short) < gm_genie_const.te_delta)) {
            instance->decoder.parser_step = GenieDecoderStepReset;
            break;
        }
        if((instance->header_count > 2) &&
           (DURATION_DIFF(duration, gm_genie_const.te_short * 10) < gm_genie_const.te_delta * 10)) {
            // Long low after the preamble train is the sync gap.
            instance->decoder.parser_step = GenieDecoderStepSaveDuration;
            instance->decoder.decode_data = 0;
            instance->decoder.decode_count_bit = 0;
        } else {
            instance->decoder.parser_step = GenieDecoderStepReset;
            instance->header_count = 0;
        }
        break;

    case GenieDecoderStepSaveDuration:
        if(level) {
            instance->decoder.te_last = duration;
            instance->decoder.parser_step = GenieDecoderStepCheckDuration;
        }
        break;

    case GenieDecoderStepCheckDuration:
        if(!level) {
            if(duration >= ((uint32_t)gm_genie_const.te_short * 2 + gm_genie_const.te_delta)) {
                // End-of-transmission gap.
                instance->decoder.parser_step = GenieDecoderStepReset;

                // A valid frame is 64 bits, tolerating up to 5 trailing status
                // bits some Intellicode-compatible remotes append.
                if((instance->decoder.decode_count_bit >=
                    gm_genie_const.min_count_bit_for_found) &&
                   (instance->decoder.decode_count_bit <=
                    gm_genie_const.min_count_bit_for_found + 5)) {
                    if(instance->generic.data != instance->decoder.decode_data) {
                        instance->generic.data = instance->decoder.decode_data;
                        instance->generic.data_count_bit =
                            gm_genie_const.min_count_bit_for_found;
                        if(instance->base.callback)
                            instance->base.callback(&instance->base, instance->base.context);
                    }
                    instance->decoder.decode_data = 0;
                    instance->decoder.decode_count_bit = 0;
                    instance->header_count = 0;
                }
                break;
            } else if(
                (DURATION_DIFF(instance->decoder.te_last, gm_genie_const.te_short) <
                 gm_genie_const.te_delta) &&
                (DURATION_DIFF(duration, gm_genie_const.te_long) < gm_genie_const.te_delta * 2)) {
                // short high then long low -> 1
                if(instance->decoder.decode_count_bit < gm_genie_const.min_count_bit_for_found) {
                    subghz_protocol_blocks_add_bit(&instance->decoder, 1);
                } else {
                    instance->decoder.decode_count_bit++;
                }
                instance->decoder.parser_step = GenieDecoderStepSaveDuration;
            } else if(
                (DURATION_DIFF(instance->decoder.te_last, gm_genie_const.te_long) <
                 gm_genie_const.te_delta * 2) &&
                (DURATION_DIFF(duration, gm_genie_const.te_short) < gm_genie_const.te_delta)) {
                // long high then short low -> 0
                if(instance->decoder.decode_count_bit < gm_genie_const.min_count_bit_for_found) {
                    subghz_protocol_blocks_add_bit(&instance->decoder, 0);
                } else {
                    instance->decoder.decode_count_bit++;
                }
                instance->decoder.parser_step = GenieDecoderStepSaveDuration;
            } else {
                instance->decoder.parser_step = GenieDecoderStepReset;
                instance->header_count = 0;
            }
        } else {
            instance->decoder.parser_step = GenieDecoderStepReset;
            instance->header_count = 0;
        }
        break;
    }
}

static uint8_t gm_genie_decoder_get_hash_data(void* context) {
    furi_assert(context);
    GmGenieDecoder* instance = context;
    return subghz_protocol_blocks_get_hash_data(
        &instance->decoder, (instance->decoder.decode_count_bit / 8) + 1);
}

static SubGhzProtocolStatus gm_genie_decoder_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset) {
    furi_assert(context);
    GmGenieDecoder* instance = context;
    return subghz_block_generic_serialize(&instance->generic, flipper_format, preset);
}

static SubGhzProtocolStatus
    gm_genie_decoder_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    GmGenieDecoder* instance = context;
    return subghz_block_generic_deserialize_check_count_bit(
        &instance->generic, flipper_format, gm_genie_const.min_count_bit_for_found);
}

static void gm_genie_decoder_get_string(void* context, FuriString* output) {
    furi_assert(context);
    GmGenieDecoder* instance = context;

    furi_string_cat_printf(
        output,
        "%s %dbit\r\n"
        "Key:%08lX%08lX\r\n",
        instance->generic.protocol_name,
        instance->generic.data_count_bit,
        (uint32_t)(instance->generic.data >> 32),
        (uint32_t)(instance->generic.data & 0xFFFFFFFF));
}

/* ---- encoder ------------------------------------------------------------ */

static void* gm_genie_encoder_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    GmGenieEncoder* instance = malloc(sizeof(GmGenieEncoder));
    memset(instance, 0, sizeof(GmGenieEncoder));
    instance->base.protocol = &gm_protocol_genie;
    instance->generic.protocol_name = instance->base.protocol->name;
    instance->encoder.repeat = GM_GENIE_TX_REPEAT;
    instance->encoder.size_upload = GM_GENIE_UPLOAD_SIZE;
    instance->encoder.upload = malloc(instance->encoder.size_upload * sizeof(LevelDuration));
    instance->encoder.is_running = false;
    return instance;
}

static void gm_genie_encoder_free(void* context) {
    furi_assert(context);
    GmGenieEncoder* instance = context;
    free(instance->encoder.upload);
    free(instance);
}

/** Lay down the OOK waveform for exactly the code held in generic.data. */
static bool gm_genie_encoder_get_upload(GmGenieEncoder* instance) {
    furi_assert(instance);

    size_t index = 0;
    size_t size_upload = 11 * 2 + 2 + (instance->generic.data_count_bit * 2) + 4;
    if(size_upload > instance->encoder.size_upload) {
        FURI_LOG_E(TAG, "Upload exceeds buffer");
        return false;
    }
    instance->encoder.size_upload = size_upload;

    // Preamble: 11 short high/low pairs.
    for(uint8_t i = 11; i > 0; i--) {
        instance->encoder.upload[index++] = level_duration_make(true, gm_genie_const.te_short);
        instance->encoder.upload[index++] = level_duration_make(false, gm_genie_const.te_short);
    }
    // Sync: short high, then a 10x short low gap.
    instance->encoder.upload[index++] = level_duration_make(true, gm_genie_const.te_short);
    instance->encoder.upload[index++] = level_duration_make(false, gm_genie_const.te_short * 10);

    // Data, MSB first: 1 = short/long, 0 = long/short.
    for(uint8_t i = instance->generic.data_count_bit; i > 0; i--) {
        if(bit_read(instance->generic.data, i - 1)) {
            instance->encoder.upload[index++] = level_duration_make(true, gm_genie_const.te_short);
            instance->encoder.upload[index++] = level_duration_make(false, gm_genie_const.te_long);
        } else {
            instance->encoder.upload[index++] = level_duration_make(true, gm_genie_const.te_long);
            instance->encoder.upload[index++] = level_duration_make(false, gm_genie_const.te_short);
        }
    }
    // Trailing status bit, then the end gap.
    instance->encoder.upload[index++] = level_duration_make(true, gm_genie_const.te_short);
    instance->encoder.upload[index++] = level_duration_make(false, gm_genie_const.te_long);
    instance->encoder.upload[index++] = level_duration_make(true, gm_genie_const.te_short);
    instance->encoder.upload[index++] = level_duration_make(false, gm_genie_const.te_short * 40);

    return true;
}

static SubGhzProtocolStatus
    gm_genie_encoder_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    GmGenieEncoder* instance = context;
    SubGhzProtocolStatus ret = SubGhzProtocolStatusError;

    do {
        ret = subghz_block_generic_deserialize_check_count_bit(
            &instance->generic, flipper_format, gm_genie_const.min_count_bit_for_found);
        if(ret != SubGhzProtocolStatusOk) break;

        if(!gm_genie_encoder_get_upload(instance)) {
            ret = SubGhzProtocolStatusErrorEncoderGetUpload;
            break;
        }
        instance->encoder.front = 0;
        instance->encoder.repeat = GM_GENIE_TX_REPEAT;
        instance->encoder.is_running = true;
        ret = SubGhzProtocolStatusOk;
    } while(false);

    return ret;
}

static void gm_genie_encoder_stop(void* context) {
    furi_assert(context);
    GmGenieEncoder* instance = context;
    instance->encoder.is_running = false;
}

static LevelDuration gm_genie_encoder_yield(void* context) {
    GmGenieEncoder* instance = context;

    if(instance->encoder.repeat == 0 || !instance->encoder.is_running) {
        instance->encoder.is_running = false;
        return level_duration_reset();
    }

    LevelDuration ret = instance->encoder.upload[instance->encoder.front];

    if(++instance->encoder.front == instance->encoder.size_upload) {
        instance->encoder.repeat--;
        instance->encoder.front = 0;
    }
    return ret;
}

/* ---- registration ------------------------------------------------------- */

const SubGhzProtocolDecoder gm_genie_decoder = {
    .alloc = gm_genie_decoder_alloc,
    .free = gm_genie_decoder_free,
    .feed = gm_genie_decoder_feed,
    .reset = gm_genie_decoder_reset,
    .get_hash_data = gm_genie_decoder_get_hash_data,
    .serialize = gm_genie_decoder_serialize,
    .deserialize = gm_genie_decoder_deserialize,
    .get_string = gm_genie_decoder_get_string,
};

const SubGhzProtocolEncoder gm_genie_encoder = {
    .alloc = gm_genie_encoder_alloc,
    .free = gm_genie_encoder_free,
    .deserialize = gm_genie_encoder_deserialize,
    .stop = gm_genie_encoder_stop,
    .yield = gm_genie_encoder_yield,
};

const SubGhzProtocol gm_protocol_genie = {
    .name = GM_GENIE_PROTOCOL_NAME,
    .type = SubGhzProtocolTypeDynamic,
    .flag = SubGhzProtocolFlag_315 | SubGhzProtocolFlag_433 | SubGhzProtocolFlag_868 |
            SubGhzProtocolFlag_AM | SubGhzProtocolFlag_Decodable | SubGhzProtocolFlag_Load |
            SubGhzProtocolFlag_Save | SubGhzProtocolFlag_Send,
    .decoder = &gm_genie_decoder,
    .encoder = &gm_genie_encoder,
};

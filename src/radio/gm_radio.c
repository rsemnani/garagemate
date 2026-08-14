#include "gm_radio.h"
#include "gm_generator.h"

#include <furi.h>
#include <furi_hal.h>
#include <lib/subghz/devices/cc1101_int/cc1101_int_interconnect.h>
#include <lib/subghz/subghz_protocol_registry.h>

#define TAG "GarageMate"

/** Give up on a stuck transmission rather than blocking the UI forever. */
#define GM_TX_TIMEOUT_MS 3000

/** Gap between presses, roughly matching how fast a thumb can double-tap. */
#define GM_PRESS_GAP_MS 250

void gm_radio_alloc(GmRadio* radio) {
    furi_assert(radio);

    radio->environment = subghz_environment_alloc();
    subghz_environment_set_protocol_registry(
        radio->environment, (void*)&subghz_protocol_registry);

    // KeeLoq needs manufacturer keys, and the rainbow tables let the same
    // environment decode signals if a future version adds a read mode.
    subghz_environment_load_keystore(radio->environment, EXT_PATH("subghz/assets/keeloq_mfcodes"));
    subghz_environment_load_keystore(
        radio->environment, EXT_PATH("subghz/assets/keeloq_mfcodes_user"));
    subghz_environment_set_came_atomo_rainbow_table_file_name(
        radio->environment, EXT_PATH("subghz/assets/came_atomo"));
    subghz_environment_set_alutech_at_4n_rainbow_table_file_name(
        radio->environment, EXT_PATH("subghz/assets/alutech_at_4n"));
    subghz_environment_set_nice_flor_s_rainbow_table_file_name(
        radio->environment, EXT_PATH("subghz/assets/nice_flor_s"));

    subghz_devices_init();
    radio->device = subghz_devices_get_by_name(SUBGHZ_DEVICE_CC1101_INT_NAME);
}

void gm_radio_free(GmRadio* radio) {
    furi_assert(radio);

    subghz_devices_deinit();
    radio->device = NULL;

    subghz_environment_free(radio->environment);
    radio->environment = NULL;
}

bool gm_radio_frequency_allowed(const GmRadio* radio, uint32_t frequency) {
    if(radio->device == NULL) return false;

    // The radio's own band support is the baseline check.
    if(!subghz_devices_is_frequency_valid(radio->device, frequency)) return false;

    // Region limits only exist once a Flipper has been provisioned with region
    // data. Asking furi_hal_region on a device without it reports every
    // frequency as forbidden, so fall back to the band check above instead.
    if(furi_hal_region_get() == NULL) return true;

    return furi_hal_region_is_frequency_allowed(frequency);
}

/**
 * How far the tuned frequency may sit from the requested one, in Hz.
 *
 * The CC1101 synthesiser lands on discrete steps of roughly 400 Hz, so an exact
 * match is never returned; anything within a few kHz is the same channel as far
 * as an OOK garage receiver is concerned.
 */
#define GM_FREQ_TOLERANCE_HZ 10000

/** Configure the CC1101 for @p frequency. @return false when it is unusable. */
static bool gm_radio_begin(GmRadio* radio, uint32_t frequency) {
    subghz_devices_begin(radio->device);
    subghz_devices_reset(radio->device);
    subghz_devices_load_preset(radio->device, FuriHalSubGhzPresetOok650Async, NULL);

    uint32_t actual = subghz_devices_set_frequency(radio->device, frequency);
    uint32_t delta = (actual > frequency) ? (actual - frequency) : (frequency - actual);
    if(delta > GM_FREQ_TOLERANCE_HZ) {
        FURI_LOG_E(TAG, "Asked for %lu Hz, radio tuned %lu Hz", frequency, actual);
        return false;
    }
    return true;
}

static void gm_radio_end(GmRadio* radio) {
    subghz_devices_idle(radio->device);
    subghz_devices_sleep(radio->device);
    subghz_devices_end(radio->device);
}

/** Push one prepared transmitter out over the air and wait for it to drain. */
static GmTxStatus gm_radio_send_transmitter(GmRadio* radio, SubGhzTransmitter* transmitter) {
    if(!subghz_devices_set_tx(radio->device)) {
        FURI_LOG_E(TAG, "Radio refused to enter TX");
        return GmTxErrRadio;
    }

    if(!subghz_devices_start_async_tx(radio->device, subghz_transmitter_yield, transmitter)) {
        FURI_LOG_E(TAG, "Failed to start async TX");
        return GmTxErrRadio;
    }

    GmTxStatus status = GmTxOk;
    uint32_t waited = 0;
    while(!subghz_devices_is_async_complete_tx(radio->device)) {
        if(waited >= GM_TX_TIMEOUT_MS) {
            FURI_LOG_E(TAG, "TX did not complete within %d ms", GM_TX_TIMEOUT_MS);
            status = GmTxErrRadio;
            break;
        }
        furi_delay_ms(5);
        waited += 5;
    }

    subghz_devices_stop_async_tx(radio->device);
    return status;
}

/**
 * Build the payload for one press and hand it to the radio.
 *
 * A fresh transmitter per press is deliberate: deserialising is what builds the
 * upload buffer, so reusing one would replay the previous press byte for byte.
 */
static GmTxStatus gm_radio_send_press(
    GmRadio* radio,
    const GmDoor* door,
    const GmBrand* brand,
    uint32_t counter,
    uint32_t frequency) {
    SubGhzTransmitter* transmitter = NULL;
    if(brand->protocol != NULL) {
        transmitter = subghz_transmitter_alloc_init(radio->environment, brand->protocol);
        if(transmitter == NULL) {
            FURI_LOG_E(TAG, "Unknown protocol: %s", brand->protocol);
            return GmTxErrPayload;
        }
    }

    FlipperFormat* ff = flipper_format_string_alloc();
    GmTxStatus status = GmTxErrPayload;

    do {
        if(!gm_generator_build(transmitter, ff, door, brand, counter, frequency)) break;

        // Generators leave the cursor at the end; the parser reads forwards.
        flipper_format_rewind(ff);
        if(subghz_transmitter_deserialize(transmitter, ff) != SubGhzProtocolStatusOk) {
            FURI_LOG_E(TAG, "Transmitter rejected generated payload");
            break;
        }

        status = gm_radio_send_transmitter(radio, transmitter);
    } while(false);

    flipper_format_free(ff);
    if(transmitter != NULL) subghz_transmitter_free(transmitter);
    return status;
}

/**
 * Collect the frequencies one press should go out on.
 *
 * Bands the current region forbids are dropped rather than failing the whole
 * press, so a tri-band door still works on the two bands that are permitted.
 *
 * @return how many entries were written to @p out.
 */
static size_t gm_radio_band_plan(
    const GmRadio* radio,
    const GmDoor* door,
    const GmBrand* brand,
    uint32_t* out,
    size_t out_max) {
    size_t count = 0;

    if(door->all_bands) {
        for(uint8_t i = 0; i < brand->freq_count && count < out_max; i++) {
            if(gm_radio_frequency_allowed(radio, brand->freqs[i])) out[count++] = brand->freqs[i];
        }
    } else if(gm_radio_frequency_allowed(radio, door->frequency)) {
        out[count++] = door->frequency;
    }

    return count;
}

GmTxStatus gm_radio_press(GmRadio* radio, GmDoor* door, const GmBrand* brand, uint8_t presses) {
    furi_assert(radio);
    furi_assert(door);
    furi_assert(brand);

    if(!gm_generator_supports(brand)) return GmTxErrUnsupported;
    if(presses == 0) presses = 1;

    uint32_t bands[GM_FREQ_MAX];
    size_t band_count = gm_radio_band_plan(radio, door, brand, bands, COUNT_OF(bands));
    if(band_count == 0) return GmTxErrFrequency;

    furi_hal_power_suppress_charge_enter();

    GmTxStatus status = GmTxOk;
    for(uint8_t i = 0; i < presses && status == GmTxOk; i++) {
        // Rolling codes burn one counter value per press, exactly like a real
        // remote; fixed codes ignore it and send the same number every time.
        uint32_t counter = door->counter;
        if(brand->rolling) counter = ++door->counter;

        // A tri-band remote sends the *same* code on every band, so the counter
        // is chosen once per press and reused across the loop below.
        for(size_t band = 0; band < band_count && status == GmTxOk; band++) {
            if(!gm_radio_begin(radio, bands[band])) {
                status = GmTxErrRadio;
            } else {
                status = gm_radio_send_press(radio, door, brand, counter, bands[band]);
            }
            gm_radio_end(radio);
        }

        if(i + 1 < presses) furi_delay_ms(GM_PRESS_GAP_MS);
    }

    furi_hal_power_suppress_charge_exit();
    return status;
}

GmTxStatus gm_radio_export(
    GmRadio* radio,
    Storage* storage,
    const GmDoor* door,
    const GmBrand* brand,
    const char* path) {
    furi_assert(radio);
    furi_assert(storage);

    if(!gm_generator_supports(brand)) return GmTxErrUnsupported;

    SubGhzTransmitter* transmitter = NULL;
    if(brand->protocol != NULL) {
        transmitter = subghz_transmitter_alloc_init(radio->environment, brand->protocol);
        if(transmitter == NULL) return GmTxErrPayload;
    }

    FlipperFormat* ff = flipper_format_file_alloc(storage);
    GmTxStatus status = GmTxErrPayload;

    do {
        if(!flipper_format_file_open_always(ff, path)) {
            status = GmTxErrFile;
            break;
        }
        // Export the next code the door will send, so the file stays usable in
        // the stock app without desynchronising this door's counter.
        uint32_t counter = brand->rolling ? door->counter + 1 : door->counter;
        if(!gm_generator_build(transmitter, ff, door, brand, counter, door->frequency)) break;

        status = GmTxOk;
    } while(false);

    flipper_format_free(ff);
    if(transmitter != NULL) subghz_transmitter_free(transmitter);
    return status;
}

GmTxStatus gm_radio_send_file(GmRadio* radio, Storage* storage, const char* sub_path) {
    furi_assert(radio);
    furi_assert(storage);

    FlipperFormat* ff = flipper_format_file_alloc(storage);
    FuriString* buffer = furi_string_alloc();
    SubGhzTransmitter* transmitter = NULL;
    GmTxStatus status = GmTxErrFile;
    uint32_t frequency = 0;
    bool radio_open = false;

    do {
        if(!flipper_format_file_open_existing(ff, sub_path)) break;

        uint32_t version = 0;
        if(!flipper_format_read_header(ff, buffer, &version)) break;
        if(furi_string_cmp_str(buffer, SUBGHZ_KEY_FILE_TYPE) != 0) break;

        if(!flipper_format_read_uint32(ff, "Frequency", &frequency, 1)) break;

        status = GmTxErrPayload;
        if(!flipper_format_read_string(ff, "Preset", buffer)) break;
        if(!flipper_format_read_string(ff, "Protocol", buffer)) break;

        // RAW captures are sample streams, not protocol payloads, and need the
        // file-encoder worker rather than a transmitter.
        if(furi_string_cmp_str(buffer, "RAW") == 0) {
            FURI_LOG_E(TAG, "RAW captures are not supported: %s", sub_path);
            status = GmTxErrUnsupported;
            break;
        }

        transmitter =
            subghz_transmitter_alloc_init(radio->environment, furi_string_get_cstr(buffer));
        if(transmitter == NULL) break;

        flipper_format_rewind(ff);
        if(subghz_transmitter_deserialize(transmitter, ff) != SubGhzProtocolStatusOk) break;

        if(!gm_radio_frequency_allowed(radio, frequency)) {
            status = GmTxErrFrequency;
            break;
        }

        if(!gm_radio_begin(radio, frequency)) {
            status = GmTxErrFrequency;
            radio_open = true;
            break;
        }
        radio_open = true;

        furi_hal_power_suppress_charge_enter();
        status = gm_radio_send_transmitter(radio, transmitter);
        furi_hal_power_suppress_charge_exit();
    } while(false);

    if(radio_open) gm_radio_end(radio);
    if(transmitter != NULL) subghz_transmitter_free(transmitter);
    furi_string_free(buffer);
    flipper_format_free(ff);
    return status;
}

const char* gm_tx_status_text(GmTxStatus status) {
    switch(status) {
    case GmTxOk:
        return "Sent";
    case GmTxErrUnsupported:
        return "Brand not supported";
    case GmTxErrFrequency:
        return "Frequency not allowed here";
    case GmTxErrPayload:
        return "Could not build the code";
    case GmTxErrRadio:
        return "Radio error";
    case GmTxErrFile:
        return "File missing or unreadable";
    default:
        return "Unknown error";
    }
}

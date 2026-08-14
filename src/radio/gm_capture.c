#include "gm_capture.h"
#include "gm_radio.h"
#include "../protocols/gm_genie.h"
#include "../protocols/gm_registry.h"

#include <furi.h>
#include <furi_hal.h>
#include <lib/subghz/receiver.h>
#include <lib/subghz/subghz_worker.h>

#define TAG "GarageMate"

struct GmCapture {
    const SubGhzDevice* device;
    SubGhzEnvironment* environment;
    SubGhzReceiver* receiver;
    SubGhzWorker* worker;

    // Written on the worker thread, read on the UI thread, so both go through
    // this mutex.
    FuriMutex* mutex;
    GmGenieSeq* seq;
    uint16_t total;

    bool running;
};

GmCapture* gm_capture_alloc(const SubGhzDevice* device) {
    GmCapture* capture = malloc(sizeof(GmCapture));
    memset(capture, 0, sizeof(GmCapture));

    capture->device = device;
    capture->mutex = furi_mutex_alloc(FuriMutexTypeNormal);

    capture->environment = subghz_environment_alloc();
    subghz_environment_set_protocol_registry(
        capture->environment, (void*)gm_registry_get());

    capture->receiver = subghz_receiver_alloc_init(capture->environment);
    subghz_receiver_set_filter(capture->receiver, SubGhzProtocolFlag_Decodable);

    capture->worker = subghz_worker_alloc();
    subghz_worker_set_overrun_callback(
        capture->worker, (SubGhzWorkerOverrunCallback)subghz_receiver_reset);
    subghz_worker_set_pair_callback(
        capture->worker, (SubGhzWorkerPairCallback)subghz_receiver_decode);
    subghz_worker_set_context(capture->worker, capture->receiver);

    return capture;
}

void gm_capture_free(GmCapture* capture) {
    furi_assert(capture);
    gm_capture_stop(capture);

    subghz_receiver_free(capture->receiver);
    subghz_worker_free(capture->worker);
    subghz_environment_free(capture->environment);
    furi_mutex_free(capture->mutex);
    free(capture);
}

/** Fired on the worker thread each time a full Genie frame decodes. */
static void gm_capture_rx_callback(
    SubGhzReceiver* receiver,
    SubGhzProtocolDecoderBase* decoder,
    void* context) {
    UNUSED(receiver);
    GmCapture* capture = context;

    FuriString* text = furi_string_alloc();
    subghz_protocol_decoder_base_get_string(decoder, text);

    // get_string emits "Genie 64bit\r\nKey:XXXXXXXXXXXXXXXX\r\n"; pull the code
    // out of the Key line rather than reaching into the decoder's internals.
    uint64_t code = 0;
    size_t key_at = furi_string_search_str(text, "Key:", 0);
    if(key_at != FURI_STRING_FAILURE) {
        const char* cursor = furi_string_get_cstr(text) + key_at + 4;
        for(int i = 0; i < 16 && cursor[i]; i++) {
            char c = cursor[i];
            uint8_t nibble;
            if(c >= '0' && c <= '9')
                nibble = c - '0';
            else if(c >= 'A' && c <= 'F')
                nibble = c - 'A' + 10;
            else if(c >= 'a' && c <= 'f')
                nibble = c - 'a' + 10;
            else
                break;
            code = (code << 4) | nibble;
        }
    }
    furi_string_free(text);

    if(code == 0) return;

    furi_mutex_acquire(capture->mutex, FuriWaitForever);
    if(gm_genie_seq_append(capture->seq, code)) capture->total++;
    furi_mutex_release(capture->mutex);

    subghz_receiver_reset(receiver);
}

bool gm_capture_start(GmCapture* capture, uint32_t frequency, GmGenieSeq* seq) {
    furi_assert(capture);
    furi_assert(seq);
    if(capture->running) return true;
    if(!subghz_devices_is_frequency_valid(capture->device, frequency)) return false;

    capture->seq = seq;
    capture->seq->frequency = frequency;
    capture->total = 0;

    subghz_receiver_set_rx_callback(capture->receiver, gm_capture_rx_callback, capture);
    subghz_receiver_reset(capture->receiver);

    subghz_devices_begin(capture->device);
    subghz_devices_reset(capture->device);
    subghz_devices_load_preset(capture->device, FuriHalSubGhzPresetOok650Async, NULL);
    subghz_devices_set_frequency(capture->device, frequency);

    subghz_worker_start(capture->worker);
    subghz_devices_start_async_rx(
        capture->device, subghz_worker_rx_callback, capture->worker);

    capture->running = true;
    return true;
}

void gm_capture_stop(GmCapture* capture) {
    furi_assert(capture);
    if(!capture->running) return;

    subghz_devices_stop_async_rx(capture->device);
    subghz_worker_stop(capture->worker);

    subghz_devices_idle(capture->device);
    subghz_devices_sleep(capture->device);
    subghz_devices_end(capture->device);

    capture->running = false;
}

uint16_t gm_capture_total(const GmCapture* capture) {
    furi_assert(capture);
    GmCapture* mutable_capture = (GmCapture*)capture;
    furi_mutex_acquire(mutable_capture->mutex, FuriWaitForever);
    uint16_t total = capture->total;
    furi_mutex_release(mutable_capture->mutex);
    return total;
}

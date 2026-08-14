#include "gm_door_store.h"
#include "gm_paths.h"

#include <flipper_format/flipper_format.h>
#include <furi.h>

#define TAG "GarageMate"

#define GM_DOOR_FILETYPE "GarageMate Door"
#define GM_DOOR_VERSION  1

void gm_store_init(Storage* storage) {
    furi_assert(storage);
    storage_simply_mkdir(storage, GM_DATA_DIR);
    storage_simply_mkdir(storage, GM_DOOR_DIR);
    storage_simply_mkdir(storage, GM_EXPORT_DIR);
}

/** Read one door record. @return true when the file parsed completely. */
static bool gm_store_read_one(Storage* storage, const char* path, GmDoor* door) {
    FlipperFormat* ff = flipper_format_file_alloc(storage);
    FuriString* buffer = furi_string_alloc();
    bool ok = false;

    do {
        if(!flipper_format_file_open_existing(ff, path)) break;

        uint32_t version = 0;
        if(!flipper_format_read_header(ff, buffer, &version)) break;
        if(furi_string_cmp_str(buffer, GM_DOOR_FILETYPE) != 0) break;
        if(version != GM_DOOR_VERSION) break;

        memset(door, 0, sizeof(GmDoor));

        if(!flipper_format_read_string(ff, "Name", buffer)) break;
        strlcpy(door->name, furi_string_get_cstr(buffer), sizeof(door->name));

        if(!flipper_format_read_string(ff, "Brand", buffer)) break;
        strlcpy(door->brand_id, furi_string_get_cstr(buffer), sizeof(door->brand_id));

        if(!flipper_format_read_uint32(ff, "Frequency", &door->frequency, 1)) break;

        uint32_t scratch = 0;
        if(!flipper_format_read_uint32(ff, "Serial", &scratch, 1)) break;
        door->serial = scratch;

        if(!flipper_format_read_uint32(ff, "Button", &scratch, 1)) break;
        door->button = (uint8_t)scratch;

        if(!flipper_format_read_uint32(ff, "Counter", &door->counter, 1)) break;

        if(!flipper_format_read_bool(ff, "Managed", &door->managed, 1)) break;

        // Optional, so records written before multi-band support still load.
        flipper_format_read_bool(ff, "AllBands", &door->all_bands, 1);

        // Imported doors carry a path to the .sub we transmit verbatim. Managed
        // doors have no such field, so a missing key is not an error here.
        if(flipper_format_read_string(ff, "SubFile", buffer)) {
            strlcpy(door->sub_path, furi_string_get_cstr(buffer), sizeof(door->sub_path));
        }

        ok = true;
    } while(false);

    furi_string_free(buffer);
    flipper_format_free(ff);
    return ok;
}

void gm_store_load_all(Storage* storage, GmDoorList* list) {
    furi_assert(storage);
    furi_assert(list);
    list->count = 0;

    File* dir = storage_file_alloc(storage);
    FileInfo info;
    // Records we write are named "door_xxxxxxxx.door"; the cap keeps the
    // concatenated path comfortably inside GM_PATH_MAX.
    char filename[64];
    char path[GM_PATH_MAX];

    if(storage_dir_open(dir, GM_DOOR_DIR)) {
        while(list->count < GM_DOORS_MAX &&
              storage_dir_read(dir, &info, filename, sizeof(filename))) {
            if(file_info_is_dir(&info)) continue;

            size_t len = strlen(filename);
            size_t ext_len = strlen(GM_DOOR_EXT);
            if(len <= ext_len || strcmp(filename + len - ext_len, GM_DOOR_EXT) != 0) continue;

            snprintf(path, sizeof(path), "%s/%s", GM_DOOR_DIR, filename);

            GmDoor door;
            if(!gm_store_read_one(storage, path, &door)) {
                FURI_LOG_W(TAG, "Skipping unreadable door record: %s", path);
                continue;
            }

            // The filename is the source of truth for the id, so a record stays
            // addressable even if it was copied over from another Flipper.
            strlcpy(door.id, filename, sizeof(door.id));
            door.id[len - ext_len] = '\0';

            list->items[list->count++] = door;
        }
    }

    storage_dir_close(dir);
    storage_file_free(dir);
}

bool gm_store_save(Storage* storage, const GmDoor* door) {
    furi_assert(storage);
    furi_assert(door);

    char path[GM_PATH_MAX];
    gm_door_path(door, path, sizeof(path));

    FlipperFormat* ff = flipper_format_file_alloc(storage);
    bool ok = false;

    do {
        if(!flipper_format_file_open_always(ff, path)) break;
        if(!flipper_format_write_header_cstr(ff, GM_DOOR_FILETYPE, GM_DOOR_VERSION)) break;
        if(!flipper_format_write_string_cstr(ff, "Name", door->name)) break;
        if(!flipper_format_write_string_cstr(ff, "Brand", door->brand_id)) break;
        if(!flipper_format_write_uint32(ff, "Frequency", &door->frequency, 1)) break;
        if(!flipper_format_write_uint32(ff, "Serial", &door->serial, 1)) break;

        uint32_t button = door->button;
        if(!flipper_format_write_uint32(ff, "Button", &button, 1)) break;
        if(!flipper_format_write_uint32(ff, "Counter", &door->counter, 1)) break;

        bool managed = door->managed;
        if(!flipper_format_write_bool(ff, "Managed", &managed, 1)) break;

        bool all_bands = door->all_bands;
        if(!flipper_format_write_bool(ff, "AllBands", &all_bands, 1)) break;

        if(!managed && door->sub_path[0] != '\0') {
            if(!flipper_format_write_string_cstr(ff, "SubFile", door->sub_path)) break;
        }

        ok = true;
    } while(false);

    flipper_format_free(ff);

    if(!ok) FURI_LOG_E(TAG, "Failed to save door: %s", path);
    return ok;
}

bool gm_store_delete(Storage* storage, const GmDoor* door) {
    furi_assert(storage);
    furi_assert(door);

    char path[GM_PATH_MAX];

    gm_door_sub_export_path(door, path, sizeof(path));
    // The export is a convenience copy; its absence must not fail the delete.
    storage_simply_remove(storage, path);

    gm_door_path(door, path, sizeof(path));
    return storage_simply_remove(storage, path);
}

void gm_store_list_remove(GmDoorList* list, size_t index) {
    furi_assert(list);
    if(index >= list->count) return;

    for(size_t i = index; i + 1 < list->count; i++) {
        list->items[i] = list->items[i + 1];
    }
    list->count--;
}

bool gm_store_list_add(GmDoorList* list, const GmDoor* door) {
    furi_assert(list);
    if(list->count >= GM_DOORS_MAX) return false;
    list->items[list->count++] = *door;
    return true;
}

void gm_store_list_upsert(GmDoorList* list, const GmDoor* door) {
    furi_assert(list);
    for(size_t i = 0; i < list->count; i++) {
        if(strcmp(list->items[i].id, door->id) == 0) {
            list->items[i] = *door;
            return;
        }
    }
    gm_store_list_add(list, door);
}

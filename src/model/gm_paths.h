/**
 * @file gm_paths.h
 * @brief Well-known locations GarageMate reads and writes on the SD card.
 *
 * Exported .sub files deliberately live under /ext/subghz so the stock Sub-GHz
 * app can open them too -- GarageMate never takes ownership of your signals.
 */
#pragma once

#include <storage/storage.h>

/** App data root: settings and door records. */
#define GM_DATA_DIR EXT_PATH("apps_data/garagemate")

/** One .door file per saved door. */
#define GM_DOOR_DIR GM_DATA_DIR "/doors"

/** Generated .sub files, readable by the stock Sub-GHz app. */
#define GM_EXPORT_DIR EXT_PATH("subghz/garagemate")

/** Where the stock Sub-GHz app keeps saved signals, used by the importer. */
#define GM_SUBGHZ_DIR EXT_PATH("subghz")

/** Application settings file. */
#define GM_SETTINGS_PATH GM_DATA_DIR "/settings.conf"

/** Filename extension for door records. */
#define GM_DOOR_EXT ".door"

/** Filename extension for Sub-GHz signals. */
#define GM_SUB_EXT ".sub"

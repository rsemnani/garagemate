/**
 * @file gm_doorlist.h
 * @brief The home-screen list, with a per-row icon.
 *
 * The stock Submenu cannot draw an icon beside each row, and the whole point of
 * the icons is to make the door list scannable, so the list is a small custom
 * view instead. Door rows carry an icon; action rows ("Add a door", "Settings")
 * do not. Selecting a row reports its id to the callback.
 */
#pragma once

#include "../model/gm_icon.h"

#include <gui/view.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GM_DOORLIST_LABEL_MAX 28
#define GM_DOORLIST_MAX_ITEMS 32

typedef void (*GmDoorListCallback)(void* context, uint32_t id);

typedef struct GmDoorListView GmDoorListView;

GmDoorListView* gm_doorlist_alloc(void);
void gm_doorlist_free(GmDoorListView* list);
View* gm_doorlist_get_view(GmDoorListView* list);

/** Remove all rows. */
void gm_doorlist_reset(GmDoorListView* list);

/** Append a door row: @p label with icon @p icon, reporting @p id when chosen. */
void gm_doorlist_add_door(GmDoorListView* list, const char* label, GmIconId icon, uint32_t id);

/** Append an action row (no icon), reporting @p id when chosen. */
void gm_doorlist_add_action(GmDoorListView* list, const char* label, uint32_t id);

/** Set the OK callback. */
void gm_doorlist_set_callback(GmDoorListView* list, GmDoorListCallback callback, void* context);

/** Select the row whose id is @p id, if present. */
void gm_doorlist_set_selected(GmDoorListView* list, uint32_t id);

#ifdef __cplusplus
}
#endif

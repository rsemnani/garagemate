/**
 * @file garagemate_i.h
 * @brief Shared application state.
 *
 * One GarageMate instance owns every GUI module, the radio, and the loaded
 * door list. Scenes receive it as their context and mutate it directly; the
 * scene manager decides what is on screen.
 */
#pragma once

#include "catalog/gm_brands.h"
#include "model/gm_door.h"
#include "model/gm_door_store.h"
#include "model/gm_paths.h"
#include "model/gm_settings.h"
#include "radio/gm_generator.h"
#include "radio/gm_radio.h"
#include "scenes/gm_scene.h"

#include <dialogs/dialogs.h>
#include <furi.h>
#include <gui/gui.h>
#include <gui/modules/popup.h>
#include <gui/modules/submenu.h>
#include <gui/modules/text_input.h>
#include <gui/modules/variable_item_list.h>
#include <gui/modules/widget.h>
#include <gui/scene_manager.h>
#include <gui/view_dispatcher.h>
#include <notification/notification_messages.h>
#include <storage/storage.h>

#define GM_APP_NAME "GarageMate"

/** Views registered with the dispatcher. */
typedef enum {
    GmViewSubmenu,
    GmViewWidget,
    GmViewPopup,
    GmViewTextInput,
    GmViewVarItemList,
} GmViewId;

/**
 * Custom scene events.
 *
 * Submenu items report their own index as the event value, so control events
 * start above any index the UI can produce.
 */
typedef enum {
    GmCustomEventTxRun = 0x100,
    GmCustomEventPairNext,
    GmCustomEventPairBack,
    GmCustomEventPairSend,
    GmCustomEventNameDone,
    GmCustomEventPopupDone,
} GmCustomEvent;

/** Application state. */
typedef struct {
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;
    Gui* gui;
    NotificationApp* notifications;
    Storage* storage;
    DialogsApp* dialogs;

    Submenu* submenu;
    Widget* widget;
    Popup* popup;
    TextInput* text_input;
    VariableItemList* var_item_list;

    GmRadio radio;
    GmSettings settings;
    GmDoorList doors;

    /** Door being created or edited by the wizard. */
    GmDoor draft;
    /** Brand chosen in the wizard; also drives the pairing walkthrough. */
    const GmBrand* draft_brand;
    /** True while the wizard is building a door that is not yet saved. */
    bool draft_is_new;

    /** Index into GarageMate::doors of the door being viewed. */
    size_t door_index;
    /** Position within the current brand's walkthrough. */
    size_t pair_step;

    /** Scratch buffer backing the name text input. */
    char name_buf[GM_NAME_MAX];
    /** Result of the most recent transmission, shown by the popup scene. */
    GmTxStatus last_status;
} GarageMate;

/** @return the door currently being viewed, or NULL when the list is empty. */
GmDoor* garagemate_current_door(GarageMate* app);

/** Persist @p door and refresh its entry in the in-memory list. */
bool garagemate_save_door(GarageMate* app, const GmDoor* door);

/** Play the configured success or failure feedback. */
void garagemate_notify(GarageMate* app, bool success);

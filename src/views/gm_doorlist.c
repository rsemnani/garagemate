#include "gm_doorlist.h"

#include <furi.h>
#include <gui/elements.h>

/** Row height in pixels; fits a 13px icon with a little breathing room. */
#define GM_ROW_HEIGHT 16
#define GM_VISIBLE_ROWS 4

typedef struct {
    char label[GM_DOORLIST_LABEL_MAX];
    GmIconId icon;
    bool has_icon;
    uint32_t id;
} GmDoorListItem;

typedef struct {
    GmDoorListItem items[GM_DOORLIST_MAX_ITEMS];
    size_t count;
    size_t selected;
    size_t offset; // index of the first visible row
} GmDoorListModel;

struct GmDoorListView {
    View* view;
    GmDoorListCallback callback;
    void* context;
};

static void gm_doorlist_draw(Canvas* canvas, void* model) {
    GmDoorListModel* m = model;
    canvas_clear(canvas);
    canvas_set_font(canvas, FontSecondary);

    for(size_t row = 0; row < GM_VISIBLE_ROWS; row++) {
        size_t index = m->offset + row;
        if(index >= m->count) break;

        GmDoorListItem* item = &m->items[index];
        uint8_t y = row * GM_ROW_HEIGHT;
        bool selected = (index == m->selected);

        if(selected) {
            canvas_draw_box(canvas, 0, y, canvas_width(canvas), GM_ROW_HEIGHT);
            canvas_set_color(canvas, ColorWhite);
        } else {
            canvas_set_color(canvas, ColorBlack);
        }

        uint8_t text_x = 4;
        if(item->has_icon) {
            gm_icon_draw(canvas, item->icon, 3, y + 2);
            text_x = 20;
        }
        canvas_draw_str(canvas, text_x, y + 11, item->label);

        canvas_set_color(canvas, ColorBlack);
    }

    // Scrollbar when the list overflows the visible window.
    if(m->count > GM_VISIBLE_ROWS) {
        elements_scrollbar(canvas, m->selected, m->count);
    }
}

/** Keep the selected row inside the visible window. */
static void gm_doorlist_reveal(GmDoorListModel* m) {
    if(m->selected < m->offset) {
        m->offset = m->selected;
    } else if(m->selected >= m->offset + GM_VISIBLE_ROWS) {
        m->offset = m->selected - (GM_VISIBLE_ROWS - 1);
    }
}

static bool gm_doorlist_input(InputEvent* event, void* context) {
    GmDoorListView* list = context;
    if(event->type != InputTypeShort && event->type != InputTypeRepeat) return false;

    bool consumed = false;

    if(event->key == InputKeyUp || event->key == InputKeyDown) {
        with_view_model(
            list->view,
            GmDoorListModel * m,
            {
                if(m->count > 0) {
                    if(event->key == InputKeyUp) {
                        m->selected = (m->selected == 0) ? m->count - 1 : m->selected - 1;
                    } else {
                        m->selected = (m->selected + 1) % m->count;
                    }
                    gm_doorlist_reveal(m);
                }
            },
            true);
        consumed = true;
    } else if(event->key == InputKeyOk && event->type == InputTypeShort) {
        uint32_t id = 0;
        bool have = false;
        with_view_model(
            list->view,
            GmDoorListModel * m,
            {
                if(m->selected < m->count) {
                    id = m->items[m->selected].id;
                    have = true;
                }
            },
            false);
        if(have && list->callback) list->callback(list->context, id);
        consumed = true;
    }

    return consumed;
}

GmDoorListView* gm_doorlist_alloc(void) {
    GmDoorListView* list = malloc(sizeof(GmDoorListView));
    list->callback = NULL;
    list->context = NULL;

    list->view = view_alloc();
    view_set_context(list->view, list);
    view_allocate_model(list->view, ViewModelTypeLocking, sizeof(GmDoorListModel));
    view_set_draw_callback(list->view, gm_doorlist_draw);
    view_set_input_callback(list->view, gm_doorlist_input);

    return list;
}

void gm_doorlist_free(GmDoorListView* list) {
    furi_assert(list);
    view_free(list->view);
    free(list);
}

View* gm_doorlist_get_view(GmDoorListView* list) {
    furi_assert(list);
    return list->view;
}

void gm_doorlist_reset(GmDoorListView* list) {
    furi_assert(list);
    with_view_model(
        list->view,
        GmDoorListModel * m,
        {
            m->count = 0;
            m->selected = 0;
            m->offset = 0;
        },
        true);
}

static void
    gm_doorlist_add(GmDoorListView* list, const char* label, bool has_icon, GmIconId icon, uint32_t id) {
    with_view_model(
        list->view,
        GmDoorListModel * m,
        {
            if(m->count < GM_DOORLIST_MAX_ITEMS) {
                GmDoorListItem* item = &m->items[m->count];
                strlcpy(item->label, label, sizeof(item->label));
                item->has_icon = has_icon;
                item->icon = icon;
                item->id = id;
                m->count++;
            }
        },
        true);
}

void gm_doorlist_add_door(GmDoorListView* list, const char* label, GmIconId icon, uint32_t id) {
    gm_doorlist_add(list, label, true, icon, id);
}

void gm_doorlist_add_action(GmDoorListView* list, const char* label, uint32_t id) {
    gm_doorlist_add(list, label, false, GmIconGarage, id);
}

void gm_doorlist_set_callback(GmDoorListView* list, GmDoorListCallback callback, void* context) {
    furi_assert(list);
    list->callback = callback;
    list->context = context;
}

void gm_doorlist_set_selected(GmDoorListView* list, uint32_t id) {
    furi_assert(list);
    with_view_model(
        list->view,
        GmDoorListModel * m,
        {
            for(size_t i = 0; i < m->count; i++) {
                if(m->items[i].id == id) {
                    m->selected = i;
                    gm_doorlist_reveal(m);
                    break;
                }
            }
        },
        true);
}

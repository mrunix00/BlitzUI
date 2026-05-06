/*
 * SPDX-License-Identifier: GPL-3.0
 */

#include <libui/libui.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    char **items;
    size_t count;
    size_t capacity;
} list_t;

list_t list_new(size_t capacity)
{
    list_t list;
    list.items = malloc(capacity * sizeof(char *));
    if (list.items == NULL) {
        return (list_t){0};
    }
    list.count = 0;
    list.capacity = capacity;
    return list;
}

void list_free(list_t *list)
{
    free(list->items);
}

void list_push(list_t *list, const char *item)
{
    if (list->count == list->capacity)
        return;
    size_t len = strlen(item);
    list->items[list->count++] = malloc(len + 1);
    memcpy(list->items[list->count - 1], item, len + 1);
}

int main()
{
    bui_ctx_t ctx = {0};
    if (!bui_init_context(&ctx))
        return 1;

    bui_wctx_t *window = bui_new_window(&ctx, "UIDemo", 300, 200, (bui_window_flags_t){.resizable = true});
    if (window == NULL) {
        bui_deinit_context(&ctx);
        return 1;
    }

    char textbox_buffer[256] = {0};
    bui_textbox_state_t textbox_state = bui_new_textbox_state(textbox_buffer, sizeof(textbox_buffer));
    list_t list = list_new(10);
    while (bui_pump_events(&ctx)) {
        if (!bui_begin_window(window))
            continue;

        BUI_COLUMN(window, ((bui_rtlb_t){.r = 5, .t = 5, .l = 5, .b = 5}), {
            bui_label(window, "Hello, World!");
            bui_label(window, "This is libui on SDL3");

            BUI_ROW(window, (bui_rtlb_t){0}, {
                if (bui_button(window, "Submit")) {
                    list_push(&list, textbox_buffer);
                    bui_reset_textbox(&textbox_state);
                }
                if (bui_textbox(window, &textbox_state, -1)) {
                    list_push(&list, textbox_buffer);
                    bui_reset_textbox(&textbox_state);
                }
            });

            BUI_CONTAINER(window, -1, -1, (bui_rtlb_t){0}, ((bui_rtlb_t){7, 7, 7, 2}), {
                for (size_t i = list.count; i > 0; i--)
                    bui_label(window, list.items[i - 1]);
            });
        });

        bui_end_window(window);
    }

    bui_deinit_context(&ctx);
    return 0;
}

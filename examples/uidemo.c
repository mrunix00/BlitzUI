/*
 * SPDX-License-Identifier: GPL-3.0
 */

#include <libui/libui.h>
#include <libui/widgets/button.h>
#include <libui/widgets/column.h>
#include <libui/widgets/container.h>
#include <libui/widgets/row.h>
#include <libui/widgets/text.h>
#include <libui/widgets/textbox.h>
#include <string.h>

int main()
{
    bui_ctx_t ctx = {0};
    if (!bui_init_context(&ctx))
        return 1;

    static char list[256][256] = {0};
    size_t list_count = 0;

    bui_wctx_t *w
        = bui_new_window(&ctx, "UIDemo", 300, 200, (bui_window_flags_t){.resizable = true});
    if (w == NULL) {
        bui_deinit_context(&ctx);
        return 1;
    }

    char textbox_buffer[256] = {0};
    while (bui_pump_events(&ctx)) {
        BUI_WINDOW(w, {
            BUI_COLUMN(w, -1, -1, {
                bui_text(w, "Hello, World!");
                bui_text(w, "This is libui on SDL3");

                BUI_CONTAINER(w, -1, -1, {
                    for (size_t i = 0; i < list_count; i++)
                        bui_text(w, list[i]);
                });

                bool submit;
                bui_widget_event_t button_state;
                BUI_ROW(w, -1, 0, {
                    submit = bui_textbox(w, "Text Input", textbox_buffer, sizeof(textbox_buffer));
                    button_state = bui_button(w, "Submit");
                });

                if (submit || (button_state & BUI_WIDGET_EVENT_LCLICKED)) {
                    strncpy(list[list_count], textbox_buffer, sizeof(list[list_count]) - 1);
                    list_count++;
                    textbox_buffer[0] = '\0';
                }
            });
        });
    }

    bui_deinit_context(&ctx);
    return 0;
}

/*
 * Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <libui/types.h>

#define BUI_ROW(window, width, height, body) \
    bui_begin_row(window, width, height); \
    {body}; \
    bui_end_row(window)

void bui_begin_row(bui_wctx_t *wctx, uint32_t width, uint32_t height);
void bui_end_row(bui_wctx_t *wctx);

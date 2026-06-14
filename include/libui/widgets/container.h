/*
 * Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <libui/types.h>
#include <stdint.h>

#define BUI_CONTAINER(window, width, height, body) \
    bui_begin_container(window, width, height); \
    {body}; \
    bui_end_container(window)

void bui_begin_container(bui_wctx_t *wctx, uint32_t width, uint32_t height);
void bui_end_container(bui_wctx_t *wctx);

/*
 * Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <libui/libui.h>
#include <stdbool.h>

#define BUI_FONT_FIRST_CHAR 32
#define BUI_FONT_LAST_CHAR 126
#define BUI_FONT_ATLAS_WIDTH 512
#define BUI_FONT_ATLAS_HEIGHT 512
#define BUI_DEFAULT_FONT_SIZE 20.0f
#define BUI_DEFAULT_FONT_PATH "assets/IBMPlexSans_Condensed-Medium.ttf"

bool bui_font_init_default(void);

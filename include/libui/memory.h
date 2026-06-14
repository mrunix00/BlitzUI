/*
 * Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <stddef.h>

typedef struct _bui_arena_node bui_arena_node_t;
struct _bui_arena_node
{
    void *data;
    size_t size;
    size_t capacity;
    bui_arena_node_t *next;
};

typedef struct
{
    bui_arena_node_t *head;
    bui_arena_node_t *tail;
    size_t total_capacity;
} bui_arena_t;

void bui_arena_init(bui_arena_t *arena);
void bui_arena_free(bui_arena_t *arena);
void *bui_arena_alloc(bui_arena_t *arena, size_t size);
void bui_arena_reset(bui_arena_t *arena);

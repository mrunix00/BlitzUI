# SPDX-License-Identifier: GPL-3.0

CC ?= cc
AR ?= ar
PKG_CONFIG ?= pkg-config
PREFIX ?= /usr/local
BUILD_DIR ?= build

LIB_NAME := libui.a
DEMO_NAME := uidemo

SRC := \
	src/libui.c \
	src/backends/sdl3.c
OBJ := $(SRC:%.c=$(BUILD_DIR)/%.o)
DEP := $(OBJ:.o=.d)

DEMO_SRC := examples/uidemo.c
DEMO_OBJ := $(DEMO_SRC:%.c=$(BUILD_DIR)/%.o)

SDL3_CFLAGS := $(shell $(PKG_CONFIG) --cflags sdl3 2>/dev/null)
SDL3_LIBS := $(shell $(PKG_CONFIG) --libs sdl3 2>/dev/null)

CPPFLAGS += -Iinclude $(SDL3_CFLAGS)
CFLAGS ?= -std=c11 -O2 -g
CFLAGS += -Wall -Wextra -Wpedantic
LDLIBS += $(SDL3_LIBS)

.PHONY: all lib demo clean install uninstall check-sdl3

all: lib demo

lib: $(BUILD_DIR)/$(LIB_NAME)

demo: $(BUILD_DIR)/$(DEMO_NAME)

check-sdl3:
	@$(PKG_CONFIG) --exists sdl3 || { \
		echo "error: SDL3 development files not found by pkg-config (missing sdl3.pc)" >&2; \
		echo "install SDL3 dev files or set PKG_CONFIG_PATH to the directory containing sdl3.pc" >&2; \
		exit 1; \
	}

$(BUILD_DIR)/$(LIB_NAME): $(OBJ)
	@mkdir -p $(@D)
	$(AR) rcs $@ $^

$(BUILD_DIR)/$(DEMO_NAME): $(DEMO_OBJ) $(BUILD_DIR)/$(LIB_NAME) | check-sdl3
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(DEMO_OBJ) $(BUILD_DIR)/$(LIB_NAME) $(LDLIBS)

$(BUILD_DIR)/%.o: %.c | check-sdl3
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

install: $(BUILD_DIR)/$(LIB_NAME)
	install -d $(DESTDIR)$(PREFIX)/lib $(DESTDIR)$(PREFIX)/include
	install -m 0644 $(BUILD_DIR)/$(LIB_NAME) $(DESTDIR)$(PREFIX)/lib/$(LIB_NAME)
	cp -R include/* $(DESTDIR)$(PREFIX)/include/

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/lib/$(LIB_NAME)
	rm -rf $(DESTDIR)$(PREFIX)/include/libui $(DESTDIR)$(PREFIX)/include/libgfx $(DESTDIR)$(PREFIX)/include/libgfx.h

clean:
	rm -rf $(BUILD_DIR)

-include $(DEP) $(DEMO_OBJ:.o=.d)

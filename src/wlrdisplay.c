// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2024 Bardia Moshiri <fakeshell@bardia.tech>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "wlrdisplay.h"

struct randr_state;
struct randr_head;

struct randr_mode {
	struct randr_head *head;
	struct zwlr_output_mode_v1 *wlr_mode;
	struct wl_list link;

	bool preferred;
};

struct randr_head {
	struct randr_state *state;
	struct zwlr_output_head_v1 *wlr_head;
	struct wl_list link;

	char *name, *description;
	int32_t phys_width, phys_height; // mm
	struct wl_list modes;

	bool enabled;
	struct randr_mode *mode;
};

struct randr_state {
	struct zwlr_output_manager_v1 *output_manager;

	struct wl_list heads;
	uint32_t serial;
	bool running;
};

#ifndef __has_attribute
# define __has_attribute(x) 0  /* Compatibility with non-clang compilers. */
#endif

#if (__has_attribute(visibility) || defined(__GNUC__) && __GNUC__ >= 4)
#define WL_PRIVATE __attribute__ ((visibility("hidden")))
#else
#define WL_PRIVATE
#endif

extern const struct wl_interface zwlr_output_configuration_head_v1_interface;
extern const struct wl_interface zwlr_output_configuration_v1_interface;
extern const struct wl_interface zwlr_output_head_v1_interface;
extern const struct wl_interface zwlr_output_mode_v1_interface;

static const struct wl_interface *wlr_output_management_unstable_v1_types[] = {
	NULL,
	NULL,
	NULL,
	&zwlr_output_configuration_v1_interface,
	NULL,
	&zwlr_output_head_v1_interface,
	&zwlr_output_mode_v1_interface,
	&zwlr_output_mode_v1_interface,
	&zwlr_output_configuration_head_v1_interface,
	&zwlr_output_head_v1_interface,
	&zwlr_output_head_v1_interface,
	&zwlr_output_mode_v1_interface,
};

static const struct wl_message zwlr_output_manager_v1_requests[] = {
	{ "create_configuration", "nu", wlr_output_management_unstable_v1_types + 3 },
	{ "stop", "", wlr_output_management_unstable_v1_types + 0 },
};

static const struct wl_message zwlr_output_manager_v1_events[] = {
	{ "head", "n", wlr_output_management_unstable_v1_types + 5 },
	{ "done", "u", wlr_output_management_unstable_v1_types + 0 },
	{ "finished", "", wlr_output_management_unstable_v1_types + 0 },
};

WL_PRIVATE const struct wl_interface zwlr_output_manager_v1_interface = {
	"zwlr_output_manager_v1", 1,
	2, zwlr_output_manager_v1_requests,
	3, zwlr_output_manager_v1_events,
};

static const struct wl_message zwlr_output_head_v1_events[] = {
	{ "name", "s", wlr_output_management_unstable_v1_types + 0 },
	{ "description", "s", wlr_output_management_unstable_v1_types + 0 },
	{ "physical_size", "ii", wlr_output_management_unstable_v1_types + 0 },
	{ "mode", "n", wlr_output_management_unstable_v1_types + 6 },
	{ "enabled", "i", wlr_output_management_unstable_v1_types + 0 },
	{ "current_mode", "o", wlr_output_management_unstable_v1_types + 7 },
	{ "position", "ii", wlr_output_management_unstable_v1_types + 0 },
	{ "transform", "i", wlr_output_management_unstable_v1_types + 0 },
	{ "scale", "f", wlr_output_management_unstable_v1_types + 0 },
	{ "finished", "", wlr_output_management_unstable_v1_types + 0 },
};

WL_PRIVATE const struct wl_interface zwlr_output_head_v1_interface = {
	"zwlr_output_head_v1", 1,
	0, NULL,
	10, zwlr_output_head_v1_events,
};

static const struct wl_message zwlr_output_mode_v1_events[] = {
	{ "size", "ii", wlr_output_management_unstable_v1_types + 0 },
	{ "refresh", "i", wlr_output_management_unstable_v1_types + 0 },
	{ "preferred", "", wlr_output_management_unstable_v1_types + 0 },
	{ "finished", "", wlr_output_management_unstable_v1_types + 0 },
};

WL_PRIVATE const struct wl_interface zwlr_output_mode_v1_interface = {
	"zwlr_output_mode_v1", 1,
	0, NULL,
	4, zwlr_output_mode_v1_events,
};

static const struct wl_message zwlr_output_configuration_v1_requests[] = {
	{ "enable_head", "no", wlr_output_management_unstable_v1_types + 8 },
	{ "disable_head", "o", wlr_output_management_unstable_v1_types + 10 },
	{ "apply", "", wlr_output_management_unstable_v1_types + 0 },
	{ "test", "", wlr_output_management_unstable_v1_types + 0 },
	{ "destroy", "", wlr_output_management_unstable_v1_types + 0 },
};

static const struct wl_message zwlr_output_configuration_v1_events[] = {
	{ "succeeded", "", wlr_output_management_unstable_v1_types + 0 },
	{ "failed", "", wlr_output_management_unstable_v1_types + 0 },
	{ "cancelled", "", wlr_output_management_unstable_v1_types + 0 },
};

WL_PRIVATE const struct wl_interface zwlr_output_configuration_v1_interface = {
	"zwlr_output_configuration_v1", 1,
	5, zwlr_output_configuration_v1_requests,
	3, zwlr_output_configuration_v1_events,
};

static const struct wl_message zwlr_output_configuration_head_v1_requests[] = {
	{ "set_mode", "o", wlr_output_management_unstable_v1_types + 11 },
	{ "set_custom_mode", "iii", wlr_output_management_unstable_v1_types + 0 },
	{ "set_position", "ii", wlr_output_management_unstable_v1_types + 0 },
	{ "set_transform", "i", wlr_output_management_unstable_v1_types + 0 },
	{ "set_scale", "f", wlr_output_management_unstable_v1_types + 0 },
};

WL_PRIVATE const struct wl_interface zwlr_output_configuration_head_v1_interface = {
	"zwlr_output_configuration_head_v1", 1,
	5, zwlr_output_configuration_head_v1_requests,
	0, NULL,
};

int print_state(struct randr_state *state) {
    int result = 1;
    struct randr_head *head;
    wl_list_for_each(head, &state->heads, link) {
        if (head->enabled) {
            result = 0;
            break;
        }
    }

    state->running = false;
    return result;
}

static void mode_handle_size(void *data, struct zwlr_output_mode_v1 *wlr_mode,
		int32_t width, int32_t height) {
        // This space is intentionally left blank
}

static void mode_handle_refresh(void *data,
		struct zwlr_output_mode_v1 *wlr_mode, int32_t refresh) {
        // This space is intentionally left blank
}

static void mode_handle_preferred(void *data,
		struct zwlr_output_mode_v1 *wlr_mode) {
        // This space is intentionally left blank
}

static void mode_handle_finished(void *data,
		struct zwlr_output_mode_v1 *wlr_mode) {
        // This space is intentionally left blank
}

static const struct zwlr_output_mode_v1_listener mode_listener = {
	.size = mode_handle_size,
	.refresh = mode_handle_refresh,
	.preferred = mode_handle_preferred,
	.finished = mode_handle_finished,
};

static void head_handle_name(void *data,
		struct zwlr_output_head_v1 *wlr_head, const char *name) {
        // This space is intentionally left blank
}

static void head_handle_description(void *data,
		struct zwlr_output_head_v1 *wlr_head, const char *description) {
        // This space is intentionally left blank
}

static void head_handle_physical_size(void *data,
		struct zwlr_output_head_v1 *wlr_head, int32_t width, int32_t height) {
        // This space is intentionally left blank
}

static void head_handle_mode(void *data,
		struct zwlr_output_head_v1 *wlr_head,
		struct zwlr_output_mode_v1 *wlr_mode) {
	struct randr_head *head = data;

	struct randr_mode *mode = calloc(1, sizeof(*mode));
	mode->head = head;
	mode->wlr_mode = wlr_mode;
	wl_list_insert(&head->modes, &mode->link);

	zwlr_output_mode_v1_add_listener(wlr_mode, &mode_listener, mode);
}

static void head_handle_enabled(void *data,
		struct zwlr_output_head_v1 *wlr_head, int32_t enabled) {
	struct randr_head *head = data;
	head->enabled = !!enabled;
	if (!enabled) {
		head->mode = NULL;
	}
}

static void head_handle_current_mode(void *data,
		struct zwlr_output_head_v1 *wlr_head,
		struct zwlr_output_mode_v1 *wlr_mode) {
        // This space is intentionally left blank
}

static void head_handle_position(void *data,
		struct zwlr_output_head_v1 *wlr_head, int32_t x, int32_t y) {
        // This space is intentionally left blank
}

static void head_handle_transform(void *data,
		struct zwlr_output_head_v1 *wlr_head, int32_t transform) {
        // This space is intentionally left blank
}

static void head_handle_scale(void *data,
		struct zwlr_output_head_v1 *wlr_head, wl_fixed_t scale) {
        // This space is intentionally left blank
}

static void head_handle_finished(void *data,
		struct zwlr_output_head_v1 *wlr_head) {
        // This space is intentionally left blank
}

static const struct zwlr_output_head_v1_listener head_listener = {
	.name = head_handle_name,
	.description = head_handle_description,
	.physical_size = head_handle_physical_size,
	.mode = head_handle_mode,
	.enabled = head_handle_enabled,
	.current_mode = head_handle_current_mode,
	.position = head_handle_position,
	.transform = head_handle_transform,
	.scale = head_handle_scale,
	.finished = head_handle_finished,
};

static void output_manager_handle_head(void *data,
		struct zwlr_output_manager_v1 *manager,
		struct zwlr_output_head_v1 *wlr_head) {
	struct randr_state *state = data;

	struct randr_head *head = calloc(1, sizeof(*head));
	head->state = state;
	head->wlr_head = wlr_head;
	wl_list_init(&head->modes);
	wl_list_insert(&state->heads, &head->link);

	zwlr_output_head_v1_add_listener(wlr_head, &head_listener, head);
}

static void output_manager_handle_done(void *data,
		struct zwlr_output_manager_v1 *manager, uint32_t serial) {
	struct randr_state *state = data;
	state->serial = serial;
}

static void output_manager_handle_finished(void *data,
		struct zwlr_output_manager_v1 *manager) {
	// This space is intentionally left blank
}

static const struct zwlr_output_manager_v1_listener output_manager_listener = {
	.head = output_manager_handle_head,
	.done = output_manager_handle_done,
	.finished = output_manager_handle_finished,
};

static void registry_handle_global(void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version) {
	struct randr_state *state = data;

	if (strcmp(interface, zwlr_output_manager_v1_interface.name) == 0) {
		state->output_manager = wl_registry_bind(registry, name,
			&zwlr_output_manager_v1_interface, 1);
		zwlr_output_manager_v1_add_listener(state->output_manager,
			&output_manager_listener, state);
	}
}

static void registry_handle_global_remove(void *data,
		struct wl_registry *registry, uint32_t name) {
	// This space is intentionally left blank
}

static const struct wl_registry_listener registry_listener = {
	.global = registry_handle_global,
	.global_remove = registry_handle_global_remove,
};

int wlrdisplay(int argc, char *argv[]) {
    struct randr_state state = { .running = true };
    wl_list_init(&state.heads);

    struct wl_display *display = wl_display_connect(NULL);
    if (display == NULL) {
        fprintf(stderr, "failed to connect to display\n");
        return EXIT_FAILURE;
    }

    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, &state);
    wl_display_dispatch(display);
    wl_display_roundtrip(display);

    if (state.output_manager == NULL) {
        fprintf(stderr, "compositor doesn't support "
                "wlr-output-management-unstable-v1\n");
        return EXIT_FAILURE;
    }

    while (state.serial == 0) {
        if (wl_display_dispatch(display) < 0) {
            fprintf(stderr, "wl_display_dispatch failed\n");
            return EXIT_FAILURE;
        }
    }

    int result = print_state(&state);

    while (state.running && wl_display_dispatch(display) != -1) {
        // This space is intentionally left blank
    }

    zwlr_output_manager_v1_destroy(state.output_manager);
    wl_registry_destroy(registry);
    wl_display_disconnect(display);

    return result;
}

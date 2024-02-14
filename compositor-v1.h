/* Generated by wayland-scanner 1.22.0 */

#include <stdlib.h>
#include <stdint.h>
#include "wayland-util.h"

#ifndef __has_attribute
# define __has_attribute(x) 0  /* Compatibility with non-clang compilers. */
#endif

#if (__has_attribute(visibility) || defined(__GNUC__) && __GNUC__ >= 4)
#define WL_PRIVATE __attribute__ ((visibility("hidden")))
#else
#define WL_PRIVATE
#endif

extern const struct wl_interface zcompositor_v1_interface;

static const struct wl_interface *compositor_unstable_v1_types[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	&zcompositor_v1_interface,
};

static const struct wl_message zcompositor_manager_v1_requests[] = {
	{ "get_window_info", "n", compositor_unstable_v1_types + 5 },
	{ "set_window_area", "uiiii", compositor_unstable_v1_types + 0 },
	{ "destroy", "", compositor_unstable_v1_types + 0 },
};

WL_PRIVATE const struct wl_interface zcompositor_manager_v1_interface = {
	"zcompositor_manager_v1", 1,
	3, zcompositor_manager_v1_requests,
	0, NULL,
};

static const struct wl_message zcompositor_v1_requests[] = {
	{ "destroy", "", compositor_unstable_v1_types + 0 },
};

static const struct wl_message zcompositor_v1_events[] = {
	{ "window_info", "uis", compositor_unstable_v1_types + 0 },
	{ "done", "", compositor_unstable_v1_types + 0 },
};

WL_PRIVATE const struct wl_interface zcompositor_v1_interface = {
	"zcompositor_v1", 1,
	1, zcompositor_v1_requests,
	2, zcompositor_v1_events,
};


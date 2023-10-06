#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>
#include <compositor-client-v1.h>

struct WindowInfo {
    uint32_t window_id;
    int pid;
    char* title;
};

struct WindowManager {
    struct wl_display* display;
    struct wl_registry* registry;
    struct zcompositor_manager_v1* manager;
};


// Callback to handle the window_info event
static void window_info_callback(void* data, struct zcompositor_v1* zcompositor_v1,
                                 uint32_t window_id, int32_t pid, const char* title) {
    struct WindowManager* window_manager = data;

    printf("Window ID: %u\n", window_id);
    printf("PID: %d\n", pid);
    printf("Title: %s\n", title);

    zcompositor_manager_v1_set_window_area(window_manager->manager, window_id, 0, 0, 720, 720);
}

// Callback to handle the done event
static void done_callback(void* data, struct zcompositor_v1* zcompositor_v1) {
    // The 'done' event indicates that all window information has been received.
    // You can choose to perform any cleanup or exit the program here.
    printf("Received 'done' event. All window information has been sent.\n");
}

static const struct zcompositor_v1_listener compositor_listener = {
    .window_info = window_info_callback,
    .done = done_callback,
};

static void registry_handle_global(void* data, struct wl_registry* registry, uint32_t id,
                                   const char* interface, uint32_t version) {
    struct WindowManager* window_manager = data;
    printf("Interface: %s\n", interface);

    if (strcmp(interface, "zcompositor_manager_v1") == 0) {
        printf("Found zcompositor_manager_v1 interface\n");

        window_manager->manager = wl_registry_bind(registry, id, &zcompositor_manager_v1_interface, 1);
        
        struct zcompositor_v1 *compositor = zcompositor_manager_v1_get_window_info(window_manager->manager);
        zcompositor_v1_add_listener(compositor, &compositor_listener, window_manager);
    }
}

static void registry_handle_global_remove(void* data, struct wl_registry* registry, uint32_t name) {
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_handle_global,
    .global_remove = registry_handle_global_remove,
};

int main() {
    struct WindowManager window_manager = {0};
    
    window_manager.display = wl_display_connect(NULL);
    if (!window_manager.display) {
        fprintf(stderr, "Failed to connect to the Wayland display\n");
        return 1;
    }

    window_manager.registry = wl_display_get_registry(window_manager.display);
    wl_registry_add_listener(window_manager.registry, &registry_listener, &window_manager);
    wl_display_dispatch(window_manager.display);
    wl_display_roundtrip(window_manager.display);

    // Main event loop
    while (wl_display_dispatch(window_manager.display) != -1) {
        wl_display_roundtrip(window_manager.display);
    }

    return 0;
}

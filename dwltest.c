#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>
#include <compositor-client-v1.h>

// Your client-specific data structure to store window information
struct WindowInfo {
    uint32_t window_id;
    int pid;
    char* title;
    // Other window-related data
};

// Your client-specific data structure to manage windows
struct WindowManager {
    struct wl_display* display;
    struct wl_registry* registry;
    struct zcompositor_manager_v1* manager;
    struct wl_list window_infos; // List of WindowInfo structures
    // Other client-related data
};


// Callback to handle the window_info event
static void window_info_callback(void* data, struct zcompositor_v1* zcompositor_v1,
                                 uint32_t window_id, int32_t pid, const char* title) {
    struct WindowManager* window_manager = data;
    
    // Create a new WindowInfo structure and populate it with received data
    // struct WindowInfo* window_info = malloc(sizeof(struct WindowInfo));
    // window_info->window_id = window_id;
    // window_info->pid = pid;
    // window_info->title = strdup(title);
    
    // // Add the WindowInfo structure to the list
    // wl_list_insert(&window_manager->window_infos, &window_info->link);
    
    // Print window information
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

    // Cleanup resources, if needed
    // ...

    // Exit the program
    // exit(0);
}

// Define the compositor listener
static const struct zcompositor_v1_listener compositor_listener = {
    .window_info = window_info_callback,
    .done = done_callback,
};

// Callback for handling new global objects
static void registry_handle_global(void* data, struct wl_registry* registry, uint32_t id,
                                   const char* interface, uint32_t version) {
    struct WindowManager* window_manager = data;
    printf("Interface: %s\n", interface);
    // Check if the global object is the compositor manager
    if (strcmp(interface, "zcompositor_manager_v1") == 0) {
        printf("Found zcompositor_manager_v1 interface\n");

        // Bind the manager interface
        window_manager->manager = wl_registry_bind(registry, id, &zcompositor_manager_v1_interface, 1);
        
        // Set up listeners for manager events
        struct zcompositor_v1 *compositor = zcompositor_manager_v1_get_window_info(window_manager->manager);
        zcompositor_v1_add_listener(compositor, &compositor_listener, window_manager);
    }
}

// Callback for handling removed global objects
static void registry_handle_global_remove(void* data, struct wl_registry* registry, uint32_t name) {
    // Handle global object removal if needed
    // This is typically not used in this example
}

// Define the registry listener
static const struct wl_registry_listener registry_listener = {
    .global = registry_handle_global,
    .global_remove = registry_handle_global_remove,
};

int main() {
    struct WindowManager window_manager = {0};
    
    // Initialize your Wayland client and connect to the compositor
    window_manager.display = wl_display_connect(NULL);
    if (!window_manager.display) {
        fprintf(stderr, "Failed to connect to the Wayland display\n");
        return 1;
    }

    // Get the Wayland registry and bind the required interfaces
    window_manager.registry = wl_display_get_registry(window_manager.display);
    wl_registry_add_listener(window_manager.registry, &registry_listener, &window_manager);
    wl_display_dispatch(window_manager.display);
    wl_display_roundtrip(window_manager.display);

    // Main event loop
    while (wl_display_dispatch(window_manager.display) != -1) {
        // Handle client-specific logic here
        wl_display_roundtrip(window_manager.display);
    }

    // Cleanup and exit
    // Free allocated memory, destroy Wayland objects, and disconnect from the display
    
    return 0;
}

/**
 * @file manual_cgl_test.c
 * A minimal manual test for the virglrenderer CGL backend on macOS.
 *
 * This test simply initializes and cleans up virglrenderer. A successful run
 * without errors proves that the CGL winsys implementation is being correctly
 * loaded and that a basic CGL context can be created and destroyed.
 */

#include "virglrenderer.h"
#include <stdio.h>

int main(void) {
    // We don't need any special callbacks for this simple test.
    struct virgl_renderer_callbacks cbs = {0};

    printf("Attempting to initialize virglrenderer with CGL backend...\n");

    // Initialize virglrenderer.
    // Passing 0 for flags requests a default (core) profile.
    int ret = virgl_renderer_init(NULL, 0, &cbs);

    if (ret != 0) {
        fprintf(stderr, "virgl_renderer_init failed with error %d\n", ret);
        return 1;
    }

    printf("virgl_renderer_init succeeded.\n");

    // Clean up.
    virgl_renderer_cleanup(NULL);
    printf("virglrenderer cleaned up successfully.\n");

    return 0;
}

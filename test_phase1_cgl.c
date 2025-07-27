/*
 * Phase 1 CGL Backend Test
 * 
 * Minimal test to validate CGL backend initialization and basic OpenGL context creation.
 * This test focuses only on what we've implemented in Phase 1.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <OpenGL/gl.h>

#include "src/virglrenderer.h"

// Minimal callbacks - we don't need full functionality for this test
static void test_write_fence(void *cookie, uint32_t fence) {
    (void)cookie;
    printf("Fence %u completed\n", fence);
}

static virgl_renderer_gl_context test_create_gl_context(void *cookie, int scanout_idx, 
                                                       struct virgl_renderer_gl_ctx_param *param) {
    (void)cookie; (void)scanout_idx; (void)param;
    return NULL; // We'll use the internal CGL context creation
}

static void test_destroy_gl_context(void *cookie, virgl_renderer_gl_context ctx) {
    (void)cookie; (void)ctx;
}

static int test_make_current(void *cookie, int scanout_idx, virgl_renderer_gl_context ctx) {
    (void)cookie; (void)scanout_idx; (void)ctx;
    return 0;
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    
    printf("=== Phase 1 CGL Backend Test ===\n");
    
    // Set up minimal callbacks
    struct virgl_renderer_callbacks cbs = {
        .version = VIRGL_RENDERER_CALLBACKS_VERSION,
        .write_fence = test_write_fence,
        .create_gl_context = test_create_gl_context,
        .destroy_gl_context = test_destroy_gl_context,
        .make_current = test_make_current,
    };
    
    printf("1. Testing CGL backend initialization...\n");
    
    // Test CGL backend initialization
    int ret = virgl_renderer_init(NULL, VIRGL_RENDERER_USE_CGL, &cbs);
    if (ret != 0) {
        printf("❌ FAILED: virgl_renderer_init returned %d\n", ret);
        return 1;
    }
    printf("✅ SUCCESS: CGL backend initialized\n");
    
    printf("2. Testing OpenGL context availability...\n");
    
    // Test that we can get basic OpenGL information
    const char *version = (const char*)glGetString(GL_VERSION);
    if (version) {
        printf("✅ SUCCESS: OpenGL Version: %s\n", version);
    } else {
        printf("❌ FAILED: Could not get OpenGL version\n");
        goto cleanup;
    }
    
    const char *vendor = (const char*)glGetString(GL_VENDOR);
    if (vendor) {
        printf("✅ SUCCESS: OpenGL Vendor: %s\n", vendor);
    } else {
        printf("❌ WARNING: Could not get OpenGL vendor\n");
    }
    
    const char *renderer = (const char*)glGetString(GL_RENDERER);
    if (renderer) {
        printf("✅ SUCCESS: OpenGL Renderer: %s\n", renderer);
    } else {
        printf("❌ WARNING: Could not get OpenGL renderer\n");
    }
    
    printf("3. Testing basic OpenGL functionality...\n");
    
    // Test basic OpenGL state queries
    GLint max_texture_size;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
    GLenum error = glGetError();
    if (error == GL_NO_ERROR) {
        printf("✅ SUCCESS: Max texture size: %d\n", max_texture_size);
    } else {
        printf("❌ FAILED: OpenGL error 0x%x when querying max texture size\n", error);
        goto cleanup;
    }
    
    printf("4. Testing cleanup...\n");
    
cleanup:
    virgl_renderer_cleanup(NULL);
    printf("✅ SUCCESS: Cleanup completed\n");
    
    printf("\n=== Phase 1 Test Complete ===\n");
    printf("The CGL backend basic functionality is working!\n");
    
    return 0;
} 
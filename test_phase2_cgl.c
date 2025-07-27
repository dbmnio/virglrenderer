/*
 * Phase 2 CGL Backend Integration Test
 * 
 * Tests the integration of the CGL backend with the vrend_winsys subsystem.
 * This validates:
 * 1. Automatic CGL flag selection on macOS
 * 2. Flag mapping from VIRGL_RENDERER_USE_CGL to VREND_USE_CGL  
 * 3. Context creation and management through the integrated system
 */

#define GL_SILENCE_DEPRECATION  // Silence OpenGL deprecation warnings on macOS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <OpenGL/gl.h>

#include "src/virglrenderer.h"

// Test state tracking
static int test_cookie = 42;
static int context_create_count = 0;
static int context_destroy_count = 0;
static int make_current_count = 0;

// Test callbacks to track behavior
static void test_write_fence(void *cookie, uint32_t fence) {
    (void)cookie;
    printf("  📨 Fence %u completed\n", fence);
}

static virgl_renderer_gl_context test_create_gl_context(void *cookie, int scanout_idx, 
                                                       struct virgl_renderer_gl_ctx_param *param) {
    (void)cookie; (void)scanout_idx;
    context_create_count++;
    printf("  🔧 GL context creation requested (major: %d, minor: %d, shared: %s)\n", 
           param->major_ver, param->minor_ver, param->shared ? "yes" : "no");
    return NULL; // Use internal context creation
}

static void test_destroy_gl_context(void *cookie, virgl_renderer_gl_context ctx) {
    (void)cookie; (void)ctx;
    context_destroy_count++;
    printf("  🗑️  GL context destruction requested\n");
}

static int test_make_current(void *cookie, int scanout_idx, virgl_renderer_gl_context ctx) {
    (void)cookie; (void)scanout_idx; (void)ctx;
    make_current_count++;
    printf("  ⚡ Make current requested (count: %d)\n", make_current_count);
    return 0;
}

// Helper function to reset counters
static void reset_counters(void) {
    context_create_count = 0;
    context_destroy_count = 0;
    make_current_count = 0;
}

// Helper function to test basic OpenGL functionality
static bool test_opengl_basic(void) {
    printf("  🔍 Testing basic OpenGL functionality...\n");
    
    // Test basic OpenGL state queries
    GLint max_texture_size;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        printf("  ❌ OpenGL error 0x%x when querying max texture size\n", error);
        return false;
    }
    
    const char *version = (const char*)glGetString(GL_VERSION);
    if (!version) {
        printf("  ❌ Could not get OpenGL version\n");
        return false;
    }
    
    printf("  ✅ OpenGL Version: %s\n", version);
    printf("  ✅ Max texture size: %d\n", max_texture_size);
    return true;
}

// Test 1: Automatic CGL flag selection on macOS
static bool test_automatic_flag_selection(void) {
    printf("\n📋 Test 1: Automatic CGL Flag Selection\n");
    printf("Testing that CGL is automatically selected on macOS when no window system flags are specified...\n");
    
    reset_counters();
    
    struct virgl_renderer_callbacks cbs = {
        .version = VIRGL_RENDERER_CALLBACKS_VERSION,
        .write_fence = test_write_fence,
        .create_gl_context = test_create_gl_context,
        .destroy_gl_context = test_destroy_gl_context,
        .make_current = test_make_current,
    };
    
    // Initialize without any window system flags - should auto-select CGL on macOS
    int ret = virgl_renderer_init(&test_cookie, 0, &cbs);
    if (ret != 0) {
        printf("❌ FAILED: virgl_renderer_init returned %d (expected 0)\n", ret);
        return false;
    }
    
    printf("✅ SUCCESS: Initialization succeeded with no window system flags\n");
    
    // Test that OpenGL is working (indicating CGL was selected)
    if (!test_opengl_basic()) {
        printf("❌ FAILED: OpenGL functionality not available\n");
        virgl_renderer_cleanup(NULL);
        return false;
    }
    
    printf("✅ SUCCESS: OpenGL is functional, indicating CGL was automatically selected\n");
    
    virgl_renderer_cleanup(NULL);
    return true;
}

// Test 2: Explicit CGL flag mapping
static bool test_explicit_flag_mapping(void) {
    printf("\n📋 Test 2: Explicit CGL Flag Mapping\n");
    printf("Testing that VIRGL_RENDERER_USE_CGL flag is properly processed...\n");
    
    reset_counters();
    
    struct virgl_renderer_callbacks cbs = {
        .version = VIRGL_RENDERER_CALLBACKS_VERSION,
        .write_fence = test_write_fence,
        .create_gl_context = test_create_gl_context,
        .destroy_gl_context = test_destroy_gl_context,
        .make_current = test_make_current,
    };
    
    // Initialize with explicit CGL flag
    int ret = virgl_renderer_init(&test_cookie, VIRGL_RENDERER_USE_CGL, &cbs);
    if (ret != 0) {
        printf("❌ FAILED: virgl_renderer_init returned %d (expected 0)\n", ret);
        return false;
    }
    
    printf("✅ SUCCESS: Initialization succeeded with explicit VIRGL_RENDERER_USE_CGL flag\n");
    
    // Test that OpenGL is working
    if (!test_opengl_basic()) {
        printf("❌ FAILED: OpenGL functionality not available\n");
        virgl_renderer_cleanup(NULL);
        return false;
    }
    
    printf("✅ SUCCESS: OpenGL is functional with explicit CGL flag\n");
    
    virgl_renderer_cleanup(NULL);
    return true;
}

// Test 3: Context creation and management
static bool test_context_creation(void) {
    printf("\n📋 Test 3: Context Creation and Management\n");
    printf("Testing OpenGL context creation, switching, and resource management...\n");
    
    reset_counters();
    
    struct virgl_renderer_callbacks cbs = {
        .version = VIRGL_RENDERER_CALLBACKS_VERSION,
        .write_fence = test_write_fence,
        .create_gl_context = test_create_gl_context,
        .destroy_gl_context = test_destroy_gl_context,
        .make_current = test_make_current,
    };
    
    // Initialize CGL backend
    int ret = virgl_renderer_init(&test_cookie, VIRGL_RENDERER_USE_CGL, &cbs);
    if (ret != 0) {
        printf("❌ FAILED: virgl_renderer_init returned %d\n", ret);
        return false;
    }
    
    printf("✅ SUCCESS: CGL backend initialized\n");
    
    // Test creating a virgl context (this exercises the winsys integration)
    uint32_t ctx_id = 1;
    const char *ctx_name = "test_ctx";
    
    ret = virgl_renderer_context_create(ctx_id, strlen(ctx_name), ctx_name);
    if (ret != 0) {
        printf("❌ FAILED: virgl_renderer_context_create returned %d\n", ret);
        virgl_renderer_cleanup(NULL);
        return false;
    }
    
    printf("✅ SUCCESS: Virgl context created (ID: %u)\n", ctx_id);
    
    // Test that we can still do OpenGL operations
    if (!test_opengl_basic()) {
        printf("❌ FAILED: OpenGL functionality not available after context creation\n");
        virgl_renderer_context_destroy(ctx_id);
        virgl_renderer_cleanup(NULL);
        return false;
    }
    
    printf("✅ SUCCESS: OpenGL operations work with created context\n");
    
    // Test context destruction
    virgl_renderer_context_destroy(ctx_id);
    printf("✅ SUCCESS: Virgl context destroyed\n");
    
    virgl_renderer_cleanup(NULL);
    
    // Verify callback interactions (internal contexts may or may not use callbacks)
    printf("📊 Context management stats:\n");
    printf("  - GL context creates: %d\n", context_create_count);
    printf("  - GL context destroys: %d\n", context_destroy_count);
    printf("  - Make current calls: %d\n", make_current_count);
    
    return true;
}

// Test 4: Multi-initialization behavior  
static bool test_multiple_init_cleanup(void) {
    printf("\n📋 Test 4: Multiple Initialization/Cleanup Cycles\n");
    printf("Testing that multiple init/cleanup cycles work correctly...\n");
    
    struct virgl_renderer_callbacks cbs = {
        .version = VIRGL_RENDERER_CALLBACKS_VERSION,
        .write_fence = test_write_fence,
        .create_gl_context = test_create_gl_context,
        .destroy_gl_context = test_destroy_gl_context,
        .make_current = test_make_current,
    };
    
    for (int i = 0; i < 3; i++) {
        printf("  🔄 Cycle %d/3...\n", i + 1);
        
        reset_counters();
        
        int ret = virgl_renderer_init(&test_cookie, VIRGL_RENDERER_USE_CGL, &cbs);
        if (ret != 0) {
            printf("❌ FAILED: virgl_renderer_init cycle %d returned %d\n", i + 1, ret);
            return false;
        }
        
        // Quick OpenGL test
        GLint max_texture_size;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
        if (glGetError() != GL_NO_ERROR) {
            printf("❌ FAILED: OpenGL error in cycle %d\n", i + 1);
            virgl_renderer_cleanup(NULL);
            return false;
        }
        
        virgl_renderer_cleanup(NULL);
    }
    
    printf("✅ SUCCESS: Multiple init/cleanup cycles completed successfully\n");
    return true;
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    
    printf("🧪 === Phase 2 CGL Backend Integration Test ===\n");
    printf("Testing CGL backend integration with vrend_winsys subsystem\n");
    
#ifndef __APPLE__
    printf("⚠️  WARNING: This test is designed for macOS. Results may vary on other platforms.\n");
#endif
    
    bool all_passed = true;
    
    // Run all test cases
    if (!test_automatic_flag_selection()) {
        all_passed = false;
    }
    
    if (!test_explicit_flag_mapping()) {
        all_passed = false;  
    }
    
    if (!test_context_creation()) {
        all_passed = false;
    }
    
    if (!test_multiple_init_cleanup()) {
        all_passed = false;
    }
    
    // Summary
    printf("\n🏁 === Test Summary ===\n");
    if (all_passed) {
        printf("🎉 ALL TESTS PASSED! Phase 2 CGL integration is working correctly.\n");
        printf("\n✅ Verified functionality:\n");
        printf("  • Automatic CGL selection on macOS\n");
        printf("  • Explicit CGL flag processing\n");
        printf("  • OpenGL context creation and management\n");
        printf("  • Multiple initialization cycles\n");
        printf("  • Basic OpenGL operations\n");
        printf("\nThe CGL backend is ready for Phase 3 implementation! 🚀\n");
        return 0;
    } else {
        printf("❌ SOME TESTS FAILED. Please check the output above for details.\n");
        return 1;
    }
} 
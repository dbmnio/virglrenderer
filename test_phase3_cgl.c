/*
 * test_phase3_cgl.c
 *
 * Comprehensive test suite for Phase 3 CGL backend implementation.
 * Tests enhanced context management, sub-context creation, resource sharing,
 * and memory management.
 */

#define GL_SILENCE_DEPRECATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>

// Include the CGL winsys headers
#include "vrend/vrend_winsys_cgl.h"

// Forward declarations for types we need
typedef void *virgl_renderer_gl_context;

struct virgl_gl_ctx_param {
    int major_ver;
    int minor_ver;
    bool shared;
    bool compat_ctx;
};

// Helper function to print errors (simple version of virgl_error)
static void test_error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

// Test result tracking
static int tests_run = 0;
static int tests_passed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        tests_run++; \
        if (condition) { \
            printf("  ✅ %s\n", message); \
            tests_passed++; \
        } else { \
            printf("  ❌ %s\n", message); \
        } \
    } while(0)

#define TEST_SECTION(name) \
    printf("\n📋 %s\n", name)

// Helper function to check OpenGL functionality
static int test_opengl_functionality(void)
{
    // Test basic OpenGL calls
    const char *version = (const char*)glGetString(GL_VERSION);
    const char *vendor = (const char*)glGetString(GL_VENDOR);
    const char *renderer = (const char*)glGetString(GL_RENDERER);
    
    if (!version || !vendor || !renderer) {
        return 0;
    }
    
    printf("    🔍 OpenGL Version: %s\n", version);
    printf("    🔍 Vendor: %s\n", vendor);
    printf("    🔍 Renderer: %s\n", renderer);
    
    // Test that we can query basic GL state
    GLint max_texture_size;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
    printf("    🔍 Max texture size: %d\n", max_texture_size);
    
    return (max_texture_size > 0) ? 1 : 0;
}

// Test basic CGL initialization
static void test_cgl_initialization(void)
{
    TEST_SECTION("Test 1: CGL Initialization");
    
    // Test virgl_cgl_init
    struct virgl_cgl *cgl = virgl_cgl_init();
    TEST_ASSERT(cgl != NULL, "virgl_cgl_init succeeds");
    
    if (cgl) {
        // Test that we can get proc addresses
        void *proc = virgl_cgl_get_proc_address("glGetString");
        TEST_ASSERT(proc != NULL, "virgl_cgl_get_proc_address works for glGetString");
        
        // Cleanup
        virgl_cgl_destroy(cgl);
        printf("  ✅ CGL cleanup completed\n");
    }
}

// Test enhanced context creation
static void test_enhanced_context_creation(void)
{
    TEST_SECTION("Test 2: Enhanced Context Creation");
    
    struct virgl_cgl *cgl = virgl_cgl_init();
    TEST_ASSERT(cgl != NULL, "CGL initialization for context tests");
    
    if (!cgl) return;
    
    // Test context creation with different parameters
    struct virgl_gl_ctx_param params = {
        .major_ver = 3,
        .minor_ver = 3,
        .shared = false,
        .compat_ctx = false
    };
    
    virgl_renderer_gl_context ctx = virgl_cgl_create_context(cgl, &params);
    TEST_ASSERT(ctx != NULL, "Context creation succeeds");
    
    if (ctx) {
        // Test making context current
        int result = virgl_cgl_make_context_current(cgl, ctx);
        TEST_ASSERT(result == 0, "Making context current succeeds");
        
        if (result == 0) {
            // Test that OpenGL works with this context
            int gl_works = test_opengl_functionality();
            TEST_ASSERT(gl_works, "OpenGL functionality works with created context");
            
            // Test context structure access
            struct vrend_cgl_context *vrend_ctx = (struct vrend_cgl_context *)ctx;
            TEST_ASSERT(vrend_ctx->ctx != NULL, "Native CGL context is valid");
            TEST_ASSERT(vrend_ctx->pixel_format != NULL, "Pixel format is valid");
            TEST_ASSERT(!vrend_ctx->owns_pixel_format, "Context doesn't claim to own pixel format (memory safety)");
            TEST_ASSERT(vrend_ctx->gl_major_version >= 3, "OpenGL major version is reasonable");
            TEST_ASSERT(vrend_ctx->gl_minor_version >= 0, "OpenGL minor version is reasonable");
            
            printf("    🔍 Context GL version: %d.%d\n", 
                   vrend_ctx->gl_major_version, vrend_ctx->gl_minor_version);
        }
        
        // Test context release
        result = virgl_cgl_make_context_current(cgl, NULL);
        TEST_ASSERT(result == 0, "Releasing context succeeds");
        
        // Cleanup context
        virgl_cgl_destroy_context(cgl, ctx);
        printf("  ✅ Context cleanup completed\n");
    }
    
    virgl_cgl_destroy(cgl);
}

// Test sub-context creation and resource sharing
static void test_sub_context_management(void)
{
    TEST_SECTION("Test 3: Sub-Context Management and Resource Sharing");
    
    struct virgl_cgl *cgl = virgl_cgl_init();
    TEST_ASSERT(cgl != NULL, "CGL initialization for sub-context tests");
    
    if (!cgl) return;
    
    // Create main context
    struct virgl_gl_ctx_param params = {
        .major_ver = 3,
        .minor_ver = 3,
        .shared = false,
        .compat_ctx = false
    };
    
    virgl_renderer_gl_context main_ctx = virgl_cgl_create_context(cgl, &params);
    TEST_ASSERT(main_ctx != NULL, "Main context creation succeeds");
    
    if (!main_ctx) {
        virgl_cgl_destroy(cgl);
        return;
    }
    
    struct vrend_cgl_context *main_vrend_ctx = (struct vrend_cgl_context *)main_ctx;
    
    // Test sub-context creation
    struct vrend_cgl_context *sub_ctx = virgl_cgl_create_sub_context(cgl, main_vrend_ctx);
    TEST_ASSERT(sub_ctx != NULL, "Sub-context creation succeeds");
    
    if (sub_ctx) {
        // Verify sub-context properties
        TEST_ASSERT(sub_ctx->ctx != NULL, "Sub-context has valid native CGL context");
        TEST_ASSERT(sub_ctx->pixel_format == main_vrend_ctx->pixel_format, 
                   "Sub-context shares pixel format with main context");
        TEST_ASSERT(!sub_ctx->owns_pixel_format, 
                   "Sub-context doesn't claim ownership of pixel format");
        TEST_ASSERT(sub_ctx->gl_major_version == main_vrend_ctx->gl_major_version,
                   "Sub-context inherits GL major version from main context");
        TEST_ASSERT(sub_ctx->gl_minor_version == main_vrend_ctx->gl_minor_version,
                   "Sub-context inherits GL minor version from main context");
        
        printf("    🔍 Sub-context GL version: %d.%d (inherited from main)\n", 
               sub_ctx->gl_major_version, sub_ctx->gl_minor_version);
        
        // Test making sub-context current
        int result = virgl_cgl_make_sub_context_current(sub_ctx);
        TEST_ASSERT(result == 0, "Making sub-context current succeeds");
        
        if (result == 0) {
            // Test OpenGL functionality with sub-context
            int gl_works = test_opengl_functionality();
            TEST_ASSERT(gl_works, "OpenGL functionality works with sub-context");
            
            // Test resource sharing by creating a texture in sub-context
            GLuint texture;
            glGenTextures(1, &texture);
            TEST_ASSERT(texture != 0, "Texture creation in sub-context succeeds");
            
            if (texture != 0) {
                glBindTexture(GL_TEXTURE_2D, texture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
                
                GLenum error = glGetError();
                TEST_ASSERT(error == GL_NO_ERROR, "Texture operations in sub-context work");
                
                // Switch back to main context and verify texture is accessible (resource sharing)
                result = virgl_cgl_make_context_current(cgl, main_ctx);
                if (result == 0) {
                    glBindTexture(GL_TEXTURE_2D, texture);
                    error = glGetError();
                    TEST_ASSERT(error == GL_NO_ERROR, "Texture created in sub-context is accessible from main context (resource sharing works)");
                    
                    glDeleteTextures(1, &texture);
                }
            }
        }
        
        // Test sub-context release
        result = virgl_cgl_make_sub_context_current(NULL);
        TEST_ASSERT(result == 0, "Releasing sub-context succeeds");
        
        // Cleanup sub-context
        virgl_cgl_destroy_sub_context(sub_ctx);
        printf("  ✅ Sub-context cleanup completed\n");
    }
    
    // Cleanup main context and CGL
    virgl_cgl_destroy_context(cgl, main_ctx);
    virgl_cgl_destroy(cgl);
    printf("  ✅ Main context and CGL cleanup completed\n");
}

// Test error handling and edge cases
static void test_error_handling(void)
{
    TEST_SECTION("Test 4: Error Handling and Edge Cases");
    
    // Test NULL parameter handling
    struct virgl_cgl *cgl = virgl_cgl_init();
    TEST_ASSERT(cgl != NULL, "CGL initialization for error tests");
    
    if (!cgl) return;
    
    // Test NULL parameter in context creation
    virgl_renderer_gl_context ctx = virgl_cgl_create_context(NULL, NULL);
    TEST_ASSERT(ctx == NULL, "Context creation with NULL parameters returns NULL");
    
    ctx = virgl_cgl_create_context(cgl, NULL);
    TEST_ASSERT(ctx == NULL, "Context creation with NULL params returns NULL");
    
    // Test NULL parameter in sub-context creation
    struct vrend_cgl_context *sub_ctx = virgl_cgl_create_sub_context(NULL, NULL);
    TEST_ASSERT(sub_ctx == NULL, "Sub-context creation with NULL parameters returns NULL");
    
    sub_ctx = virgl_cgl_create_sub_context(cgl, NULL);
    TEST_ASSERT(sub_ctx == NULL, "Sub-context creation with NULL main context returns NULL");
    
    // Test NULL parameter in cleanup functions (should not crash)
    virgl_cgl_destroy_context(cgl, NULL);
    virgl_cgl_destroy_sub_context(NULL);
    printf("  ✅ NULL parameter cleanup functions don't crash\n");
    
    // Test making NULL context current
    int result = virgl_cgl_make_context_current(cgl, NULL);
    TEST_ASSERT(result == 0, "Making NULL context current succeeds (context release)");
    
    result = virgl_cgl_make_sub_context_current(NULL);
    TEST_ASSERT(result == 0, "Making NULL sub-context current succeeds (context release)");
    
    virgl_cgl_destroy(cgl);
    printf("  ✅ Error handling tests completed\n");
}

// Test memory leak prevention
static void test_memory_management(void)
{
    TEST_SECTION("Test 5: Memory Management and Leak Prevention");
    
    printf("  🔍 Creating and destroying multiple contexts to test for memory leaks...\n");
    
    // Create and destroy multiple CGL instances
    for (int i = 0; i < 10; i++) {
        struct virgl_cgl *cgl = virgl_cgl_init();
        TEST_ASSERT(cgl != NULL, "Multiple CGL init/destroy cycles work");
        
        if (cgl) {
            struct virgl_gl_ctx_param params = {
                .major_ver = 3,
                .minor_ver = 3,
                .shared = false,
                .compat_ctx = false
            };
            
            // Create multiple contexts
            virgl_renderer_gl_context ctx1 = virgl_cgl_create_context(cgl, &params);
            virgl_renderer_gl_context ctx2 = virgl_cgl_create_context(cgl, &params);
            
            if (ctx1 && ctx2) {
                // Create sub-contexts
                struct vrend_cgl_context *main_ctx = (struct vrend_cgl_context *)ctx1;
                struct vrend_cgl_context *sub1 = virgl_cgl_create_sub_context(cgl, main_ctx);
                struct vrend_cgl_context *sub2 = virgl_cgl_create_sub_context(cgl, main_ctx);
                
                // Cleanup in reverse order
                if (sub2) virgl_cgl_destroy_sub_context(sub2);
                if (sub1) virgl_cgl_destroy_sub_context(sub1);
                virgl_cgl_destroy_context(cgl, ctx2);
                virgl_cgl_destroy_context(cgl, ctx1);
            }
            
            virgl_cgl_destroy(cgl);
        }
    }
    
    printf("  ✅ Multiple create/destroy cycles completed without crashes\n");
    printf("  🔍 If no heap corruption errors appeared, memory management is likely correct\n");
}

int main(void)
{
    printf("🧪 === Phase 3 CGL Backend Enhancement Test ===\n");
    printf("Testing enhanced context management, sub-contexts, and resource sharing\n");
    
    // Note about PBuffer removal
    printf("\n📝 Note: PBuffer support was removed due to macOS deprecation.\n");
    printf("   virglrenderer uses FBOs for off-screen rendering instead.\n");
    
    // Run all tests
    test_cgl_initialization();
    test_enhanced_context_creation();
    test_sub_context_management();
    test_error_handling();
    test_memory_management();
    
    // Print summary
    printf("\n🏁 === Test Summary ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    
    if (tests_passed == tests_run) {
        printf("🎉 All tests passed! Phase 3 CGL implementation is working correctly.\n");
        return 0;
    } else {
        printf("❌ Some tests failed. Please check the implementation.\n");
        return 1;
    }
} 
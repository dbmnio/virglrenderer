/*
 * Copyright 2024 afritz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * @file vrend_winsys_cgl.c
 *
 * CGL (CoreGL) winsys implementation for virglrenderer on macOS.
 *
 * This file provides the platform-specific winsys implementation that allows
 * virglrenderer to create and manage OpenGL contexts using the native CGL
 * framework on macOS. It supports both main contexts and sub-contexts with
 * proper resource sharing, PBuffer management, and OpenGL version detection.
 */

#define GL_SILENCE_DEPRECATION

#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLTypes.h>
#include <OpenGL/gl.h>

#include "vrend_winsys_cgl.h"
#include "vrend_renderer.h"
#include "virgl_util.h"

struct virgl_cgl {
    CGLPixelFormatObj pix_fmt;
    CGLContextObj ctx;
};

/**
 * Create appropriate pixel format attributes for OpenGL 3.3+ Core Profile
 * 
 * @return Array of pixel format attributes suitable for virglrenderer
 */
static CGLPixelFormatAttribute *cgl_create_pixel_format_attributes(void)
{
    static CGLPixelFormatAttribute attribs[] = {
        kCGLPFAOpenGLProfile,
        (CGLPixelFormatAttribute)kCGLOGLPVersion_3_2_Core,
        kCGLPFADoubleBuffer,
        kCGLPFAAccelerated,
        kCGLPFANoRecovery,
        kCGLPFAColorSize, 24,
        kCGLPFAAlphaSize, 8,
        kCGLPFADepthSize, 24,
        kCGLPFAStencilSize, 8,
        (CGLPixelFormatAttribute)0
    };
    
    return attribs;
}

/**
 * Query OpenGL version from current context
 *
 * @param major_ver Pointer to store major version
 * @param minor_ver Pointer to store minor version
 */
static void cgl_query_gl_version(int *major_ver, int *minor_ver)
{
    const char *version_str = (const char *)glGetString(GL_VERSION);
    if (version_str) {
        sscanf(version_str, "%d.%d", major_ver, minor_ver);
    } else {
        *major_ver = 3;
        *minor_ver = 3;  /* Default fallback */
    }
}

struct virgl_cgl *virgl_cgl_init(void)
{
    struct virgl_cgl *cgl = malloc(sizeof(struct virgl_cgl));
    if (!cgl)
        return NULL;

    cgl->pix_fmt = NULL;
    cgl->ctx = NULL;

    CGLPixelFormatAttribute *attribs = cgl_create_pixel_format_attributes();
    GLint num_pixel_formats = 0;
    
    CGLError err = CGLChoosePixelFormat(attribs, &cgl->pix_fmt, &num_pixel_formats);
    if (err != kCGLNoError) {
        virgl_error("CGLChoosePixelFormat failed: %s\n", CGLErrorString(err));
        free(cgl);
        return NULL;
    }

    err = CGLCreateContext(cgl->pix_fmt, NULL, &cgl->ctx);
    if (err != kCGLNoError) {
        virgl_error("CGLCreateContext failed: %s\n", CGLErrorString(err));
        CGLDestroyPixelFormat(cgl->pix_fmt);
        free(cgl);
        return NULL;
    }

    err = CGLSetCurrentContext(cgl->ctx);
    if (err != kCGLNoError) {
        virgl_error("CGLSetCurrentContext failed: %s\n", CGLErrorString(err));
        CGLDestroyContext(cgl->ctx);
        CGLDestroyPixelFormat(cgl->pix_fmt);
        free(cgl);
        return NULL;
    }

    return cgl;
}

void virgl_cgl_destroy(struct virgl_cgl *cgl)
{
    if (!cgl)
        return;

    if (cgl->ctx) {
        CGLDestroyContext(cgl->ctx);
    }
    if (cgl->pix_fmt) {
        CGLDestroyPixelFormat(cgl->pix_fmt);
    }
    free(cgl);
}

virgl_renderer_gl_context virgl_cgl_create_context(struct virgl_cgl *cgl, struct virgl_gl_ctx_param *vparams)
{
    if (!cgl || !vparams)
        return NULL;

    CGLContextObj shared_ctx = vparams->shared ? CGLGetCurrentContext() : NULL;
    struct vrend_cgl_context *vrend_ctx = malloc(sizeof(struct vrend_cgl_context));
    if (!vrend_ctx) {
        virgl_error("Failed to allocate vrend_cgl_context\n");
        return NULL;
    }

    /* Initialize the context structure */
    memset(vrend_ctx, 0, sizeof(struct vrend_cgl_context));
    vrend_ctx->pixel_format = cgl->pix_fmt;
    vrend_ctx->owns_pixel_format = true;  /* Main contexts own their resources */
    vrend_ctx->owns_pbuffer = true;

    /* Create the native CGL context */
    CGLError err = CGLCreateContext(cgl->pix_fmt, shared_ctx, &vrend_ctx->ctx);
    if (err != kCGLNoError) {
        virgl_error("CGLCreateContext failed: %s\n", CGLErrorString(err));
        free(vrend_ctx);
        return NULL;
    }

    /* Create a small PBuffer for off-screen rendering */
    err = CGLCreatePBuffer(1, 1, GL_TEXTURE_2D, GL_RGBA, 0, &vrend_ctx->pbuffer);
    if (err != kCGLNoError) {
        virgl_error("CGLCreatePBuffer failed: %s\n", CGLErrorString(err));
        CGLDestroyContext(vrend_ctx->ctx);
        free(vrend_ctx);
        return NULL;
    }

    /* Attach the PBuffer to the context */
    err = CGLSetPBuffer(vrend_ctx->ctx, vrend_ctx->pbuffer, 0, 0, 0);
    if (err != kCGLNoError) {
        virgl_error("CGLSetPBuffer failed: %s\n", CGLErrorString(err));
        CGLDestroyPBuffer(vrend_ctx->pbuffer);
        CGLDestroyContext(vrend_ctx->ctx);
        free(vrend_ctx);
        return NULL;
    }

    /* Make context current to query OpenGL version */
    CGLContextObj prev_ctx = CGLGetCurrentContext();
    err = CGLSetCurrentContext(vrend_ctx->ctx);
    if (err != kCGLNoError) {
        virgl_error("CGLSetCurrentContext failed during context creation: %s\n", CGLErrorString(err));
        CGLDestroyPBuffer(vrend_ctx->pbuffer);
        CGLDestroyContext(vrend_ctx->ctx);
        free(vrend_ctx);
        return NULL;
    }

    /* Query and store OpenGL version */
    cgl_query_gl_version(&vrend_ctx->gl_major_version, &vrend_ctx->gl_minor_version);

    /* Restore previous context */
    CGLSetCurrentContext(prev_ctx);

    return (virgl_renderer_gl_context)vrend_ctx;
}

void virgl_cgl_destroy_context(struct virgl_cgl *cgl, virgl_renderer_gl_context ctx)
{
    (void)cgl; // unused parameter
    
    if (!ctx)
        return;

    struct vrend_cgl_context *vrend_ctx = (struct vrend_cgl_context *)ctx;

    /* Destroy resources in the correct order, but only if this context owns them */
    if (vrend_ctx->owns_pbuffer && vrend_ctx->pbuffer) {
        CGLDestroyPBuffer(vrend_ctx->pbuffer);
    }
    
    if (vrend_ctx->ctx) {
        CGLDestroyContext(vrend_ctx->ctx);
    }
    
    if (vrend_ctx->owns_pixel_format && vrend_ctx->pixel_format) {
        CGLDestroyPixelFormat(vrend_ctx->pixel_format);
    }

    free(vrend_ctx);
}

int virgl_cgl_make_context_current(struct virgl_cgl *cgl, virgl_renderer_gl_context ctx)
{
    (void)cgl; // unused parameter
    
    if (!ctx) {
        /* Release current context */
        CGLError err = CGLSetCurrentContext(NULL);
        if (err != kCGLNoError) {
            virgl_error("CGLSetCurrentContext(NULL) failed: %s\n", CGLErrorString(err));
            return -1;
        }
        return 0;
    }

    struct vrend_cgl_context *vrend_ctx = (struct vrend_cgl_context *)ctx;
    
    CGLError err = CGLSetCurrentContext(vrend_ctx->ctx);
    if (err != kCGLNoError) {
        virgl_error("CGLSetCurrentContext failed: %s\n", CGLErrorString(err));
        return -1;
    }
    
    return 0;
}

void *virgl_cgl_get_proc_address(const char *procname)
{
    return dlsym(RTLD_DEFAULT, procname);
}

/**
 * Create a sub-context that shares resources with the main context
 *
 * @param cgl The main CGL winsys object
 * @param main_ctx The main context to share resources with
 * @return New sub-context or NULL on failure
 */
struct vrend_cgl_context *virgl_cgl_create_sub_context(struct virgl_cgl *cgl, 
                                                        struct vrend_cgl_context *main_ctx)
{
    if (!cgl || !main_ctx) {
        virgl_error("Invalid parameters for sub-context creation\n");
        return NULL;
    }

    struct vrend_cgl_context *sub_ctx = malloc(sizeof(struct vrend_cgl_context));
    if (!sub_ctx) {
        virgl_error("Failed to allocate sub-context\n");
        return NULL;
    }

    /* Initialize sub-context structure */
    memset(sub_ctx, 0, sizeof(struct vrend_cgl_context));
    
    /* Sub-contexts reference main context resources but don't own them */
    sub_ctx->pixel_format = main_ctx->pixel_format;
    sub_ctx->pbuffer = main_ctx->pbuffer;
    sub_ctx->owns_pixel_format = false;
    sub_ctx->owns_pbuffer = false;
    
    /* Copy OpenGL version from main context */
    sub_ctx->gl_major_version = main_ctx->gl_major_version;
    sub_ctx->gl_minor_version = main_ctx->gl_minor_version;

    /* Create the native CGL sub-context sharing resources with main context */
    CGLError err = CGLCreateContext(main_ctx->pixel_format, main_ctx->ctx, &sub_ctx->ctx);
    if (err != kCGLNoError) {
        virgl_error("CGLCreateContext failed for sub-context: %s\n", CGLErrorString(err));
        free(sub_ctx);
        return NULL;
    }

    return sub_ctx;
}

/**
 * Destroy a sub-context
 *
 * @param ctx The sub-context to destroy
 */
void virgl_cgl_destroy_sub_context(struct vrend_cgl_context *ctx)
{
    if (!ctx)
        return;

    /* Only destroy the native context - sub-contexts don't own shared resources */
    if (ctx->ctx) {
        CGLDestroyContext(ctx->ctx);
    }

    free(ctx);
}

/**
 * Make a sub-context current on the calling thread
 *
 * @param ctx The sub-context to make current, or NULL to release current context
 * @return 0 on success, -1 on failure
 */
int virgl_cgl_make_sub_context_current(struct vrend_cgl_context *ctx)
{
    CGLContextObj cgl_ctx = ctx ? ctx->ctx : NULL;
    
    CGLError err = CGLSetCurrentContext(cgl_ctx);
    if (err != kCGLNoError) {
        virgl_error("CGLSetCurrentContext failed for sub-context: %s\n", CGLErrorString(err));
        return -1;
    }
    
    return 0;
} 

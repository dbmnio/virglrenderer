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
 * framework on macOS. It is responsible for initializing the renderer,
 * creating surfaces, and handling context management.
 */

#include <dlfcn.h>
#include <stdlib.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLTypes.h>

#include "vrend_winsys_cgl.h"
#include "vrend_debug.h"

struct virgl_cgl {
    CGLPixelFormatObj pix_fmt;
    CGLContextObj ctx;
};

struct virgl_cgl *virgl_cgl_init(void)
{
    struct virgl_cgl *cgl = malloc(sizeof(struct virgl_cgl));
    if (!cgl)
        return NULL;

    cgl->pix_fmt = NULL;
    cgl->ctx = NULL;

    CGLPixelFormatAttribute attribs[] = {
        kCGLPFAOpenGLProfile,
        (CGLPixelFormatAttribute)kCGLOGLPVersion_3_3_Core,
        kCGLPFADoubleBuffer,
        kCGLPFAAccelerated,
        kCGLPFANoRecovery,
        kCGLPFAColorSize, 24,
        kCGLPFAAlphaSize, 8,
        kCGLPFADepthSize, 24,
        kCGLPFAStencilSize, 8,
        (CGLPixelFormatAttribute)0
    };

    GLint num_pixel_formats = 0;
    CGLError err = CGLChoosePixelFormat(attribs, &cgl->pix_fmt, &num_pixel_formats);
    if (err != kCGLNoError) {
        vrend_printf("CGLChoosePixelFormat failed: %s\n", CGLErrorString(err));
        free(cgl);
        return NULL;
    }

    err = CGLCreateContext(cgl->pix_fmt, NULL, &cgl->ctx);
    if (err != kCGLNoError) {
        vrend_printf("CGLCreateContext failed: %s\n", CGLErrorString(err));
        CGLDestroyPixelFormat(cgl->pix_fmt);
        free(cgl);
        return NULL;
    }

    err = CGLSetCurrentContext(cgl->ctx);
    if (err != kCGLNoError) {
        vrend_printf("CGLSetCurrentContext failed: %s\n", CGLErrorString(err));
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
    CGLContextObj new_ctx;

    CGLError err = CGLCreateContext(cgl->pix_fmt, shared_ctx, &new_ctx);
    if (err != kCGLNoError) {
        vrend_printf("CGLCreateContext failed: %s\n", CGLErrorString(err));
        return NULL;
    }

    return (virgl_renderer_gl_context)new_ctx;
}

void virgl_cgl_destroy_context(struct virgl_cgl *cgl, virgl_renderer_gl_context ctx)
{
    (void)cgl; // unused parameter
    if (ctx) {
        CGLContextObj cgl_ctx = (CGLContextObj)ctx;
        CGLDestroyContext(cgl_ctx);
    }
}

int virgl_cgl_make_context_current(struct virgl_cgl *cgl, virgl_renderer_gl_context ctx)
{
    (void)cgl; // unused parameter
    CGLContextObj cgl_ctx = (CGLContextObj)ctx;
    
    CGLError err = CGLSetCurrentContext(cgl_ctx);
    if (err != kCGLNoError) {
        vrend_printf("CGLSetCurrentContext failed: %s\n", CGLErrorString(err));
        return -1;
    }
    
    return 0;
}

void *virgl_cgl_get_proc_address(const char *procname)
{
    return dlsym(RTLD_DEFAULT, procname);
} 
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
#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLTypes.h>

#include "vrend_winsys.h"
#include "vrend_debug.h"

struct vrend_winsys_cgl {
    CGLPixelFormatObj pix_fmt;
    CGLContextObj ctx;
};

static struct vrend_winsys_cgl cgl_info;

static int vrend_winsys_cgl_init(void);
static void vrend_winsys_cgl_destroy(void);
static vrend_gl_proc_t vrend_winsys_cgl_get_proc_address(const char *procname);

const struct vrend_winsys_vtable vrend_winsys_cgl_vtable = {
    .init = vrend_winsys_cgl_init,
    .destroy = vrend_winsys_cgl_destroy,
    .get_proc_address = vrend_winsys_cgl_get_proc_address,
};

static vrend_gl_proc_t vrend_winsys_cgl_get_proc_address(const char *procname)
{
    return dlsym(RTLD_DEFAULT, procname);
}

static void vrend_winsys_cgl_destroy(void)
{
    if (cgl_info.ctx) {
        CGLDestroyContext(cgl_info.ctx);
        cgl_info.ctx = NULL;
    }
    if (cgl_info.pix_fmt) {
        CGLDestroyPixelFormat(cgl_info.pix_fmt);
        cgl_info.pix_fmt = NULL;
    }
}

static int cgl_init(void)
{
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
    CGLError err = CGLChoosePixelFormat(attribs, &cgl_info.pix_fmt, &num_pixel_formats);
    if (err != kCGLNoError) {
        vrend_printf( "CGLChoosePixelFormat failed: %s\n", CGLErrorString(err));
        return -1;
    }

    err = CGLCreateContext(cgl_info.pix_fmt, NULL, &cgl_info.ctx);
    if (err != kCGLNoError) {
        vrend_printf("CGLCreateContext failed: %s\n", CGLErrorString(err));
        vrend_winsys_cgl_destroy();
        return -1;
    }

    err = CGLSetCurrentContext(cgl_info.ctx);
    if (err != kCGLNoError) {
        vrend_printf("CGLSetCurrentContext failed: %s\n", CGLErrorString(err));
        vrend_winsys_cgl_destroy();
        return -1;
    }

    return 0;
}

static int vrend_winsys_cgl_init(void)
{
    if (cgl_init() != 0) {
        return -1;
    }
    return 0;
} 
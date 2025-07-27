/*
 * Copyright 2021 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef VREND_WINSYS_CGL_H
#define VREND_WINSYS_CGL_H

#include "vrend_winsys.h"

#ifdef HAVE_CGL_H
#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLTypes.h>

/**
 * @struct vrend_cgl_context
 *
 * Enhanced CGL context structure that holds additional state information
 * for proper sub-context management and resource sharing.
 *
 * This structure contains the native CGL objects and metadata needed
 * for managing contexts in the vrend system.
 */
struct vrend_cgl_context {
    CGLContextObj ctx;              /* Native CGL context */
    CGLPixelFormatObj pixel_format; /* Pixel format (owned by main context) */
    CGLPBufferObj pbuffer;          /* PBuffer for off-screen rendering */
    
    /* OpenGL version information */
    int gl_major_version;
    int gl_minor_version;
    
    /* Context ownership flags */
    bool owns_pixel_format;         /* True for main contexts, false for sub-contexts */
    bool owns_pbuffer;              /* True for main contexts, false for sub-contexts */
};

struct virgl_cgl;

struct virgl_cgl *virgl_cgl_init(void);

void virgl_cgl_destroy(struct virgl_cgl *cgl);

virgl_renderer_gl_context virgl_cgl_create_context(struct virgl_cgl *cgl, struct virgl_gl_ctx_param *param);

void virgl_cgl_destroy_context(struct virgl_cgl *cgl, virgl_renderer_gl_context ctx);

int virgl_cgl_make_context_current(struct virgl_cgl *cgl, virgl_renderer_gl_context ctx);

void *virgl_cgl_get_proc_address(const char *procname);

/* Enhanced context management functions for sub-context support */
struct vrend_cgl_context *virgl_cgl_create_sub_context(struct virgl_cgl *cgl, 
                                                        struct vrend_cgl_context *main_ctx);

void virgl_cgl_destroy_sub_context(struct vrend_cgl_context *ctx);

int virgl_cgl_make_sub_context_current(struct vrend_cgl_context *ctx);

#endif /* HAVE_CGL_H */

#endif

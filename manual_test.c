/*
 * A minimal manual test for the CGL winsys on macOS.
 *
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

#import <Cocoa/Cocoa.h>
#include "virglrenderer.h"

@interface VirglView : NSView
@end

@implementation VirglView
- (void)drawRect:(NSRect)dirtyRect {
    (void)dirtyRect;
    virgl_renderer_force_ctx_0();
    
    // Create a command buffer to clear the screen
    struct virgl_cmd_buf *buf = virgl_renderer_cmd_buf_new(1024);
    virgl_renderer_emit_clear(buf, VIRGL_RENDERER_CLEAR_COLOR, &(float[4]){0.2, 0.8, 0.2, 1.0}, 0, NULL);
    virgl_renderer_submit_cmd(buf, 0);
    
    virgl_renderer_swap_buffers();
}
@end

int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    id menubar = [[NSMenu new] autorelease];
    id appMenuItem = [[NSMenuItem new] autorelease];
    [menubar addItem:appMenuItem];
    [NSApp setMainMenu:menubar];

    id appMenu = [[NSMenu new] autorelease];
    id appName = [[NSProcessInfo processInfo] processName];
    id quitTitle = [@"Quit " stringByAppendingString:appName];
    id quitMenuItem = [[[NSMenuItem alloc] initWithTitle:quitTitle
                                                 action:@selector(terminate:)
                                          keyEquivalent:@"q"] autorelease];
    [appMenu addItem:quitMenuItem];
    [appMenuItem setSubmenu:appMenu];

    id window = [[[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 400, 400)
                                            styleMask:NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask
                                              backing:NSBackingStoreBuffered
                                                defer:NO] autorelease];
    [window cascadeTopLeftFromPoint:NSMakePoint(20,20)];
    [window setTitle:appName];
    [window makeKeyAndOrderFront:nil];
    
    VirglView *view = [[VirglView alloc] initWithFrame:[[window contentView] frame]];
    [window setContentView:view];

    virgl_renderer_init(view, VIRGL_RENDERER_USE_CGL, NULL);
    
    id ad = [[NSAutoreleasePool alloc] init];
    
    [NSApp activateIgnoringOtherApps:YES];
    [NSApp run];

    virgl_renderer_cleanup(NULL);
    [ad release];
    return 0;
}

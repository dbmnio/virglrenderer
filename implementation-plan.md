### **Feature Overview**

The goal is to create a new platform-specific implementation of `vrend_winsys.c` for macOS. This will allow `virglrenderer` to use the native CoreGL (CGL) framework to create and manage OpenGL contexts, enabling virgl-based rendering directly on the macOS display system. The implementation will reside in a new file, `src/vrend/vrend_winsys_cgl.c`, and will be conditionally compiled only for macOS builds.

---

### **Phase 1: Basic CGL Context and Project Scaffolding**

**Goal:** The primary objective of this phase is to establish the foundational code structure. We will create the new source file, integrate it into the Meson build system for macOS, and implement the minimal code required to create and manage a headless CGL (CoreGL) context. This validates that the project can link against the necessary macOS frameworks and that a basic OpenGL context can be successfully created.

**Implementation Steps:**

1.  **File Creation:** ✅ Create a new file: `src/vrend/vrend_winsys_cgl.c`.
2.  **Build System Integration:** ✅
    *   Modify `src/vrend/meson.build`.
    *   Add a conditional block to check if the host system is 'darwin' (macOS).
    *   Inside this block, add `vrend_winsys_cgl.c` to the list of sources for the `virglrenderer` library.
    *   Also in this block, add a dependency on the `OpenGL.framework` for linking.
3.  **Initial CGL Implementation (`vrend_winsys_cgl.c`):** ✅
    *   Implement the initial `vrend_winsys_init_cgl` function. This function will be responsible for populating the `vrend_winsys_vtable` with our CGL-specific function pointers.
    *   Implement a private `cgl_init` function that is called by `vrend_winsys_init_cgl`. This function will perform the one-time setup:
        *   Define `CGLPixelFormatAttribute` array to request an OpenGL 3.3+ Core Profile. This is crucial for modern OpenGL support.
        *   Use `CGLChoosePixelFormat` and `CGLCreateContext` to create a headless/pbuffer-based CGL context. A pbuffer (pixel buffer) is an off-screen rendering target, which is ideal for initial testing as it has no dependency on the windowing system.
    *   Implement the `vrend_winsys_cgl_get_proc_address` function. This will use `dlsym` to resolve OpenGL function pointers, which is standard practice on macOS.
    *   Implement a `vrend_winsys_cgl_destroy` function to clean up and release the CGL context and pixel format objects.

**Testing Strategy:**

*   **Standalone Test:** ✅ Create a minimal test program (`/tests/test_virgl_cgl.c`). This program will not be part of the final product but is essential for this phase.
*   **Test Logic:**
    1.  The test will link against `virglrenderer`.
    2.  It will call `vrend_renderer_init(NULL)`, which should trigger our new CGL initialization path on macOS.
    3.  After initialization, it will use `vrend_renderer_get_proc_address` to get a pointer to `glGetString`.
    4.  It will call `glGetString(GL_VERSION)` and print the result to the console.
    5.  Finally, it will call `vrend_renderer_cleanup()`.
*   **Success Criteria:** The test program compiles and runs successfully on a macOS machine, printing an OpenGL version string (e.g., "3.3 Core Profile" or higher). This confirms that the build system is correctly configured and that a valid CGL context is being created.

---

### **Phase 2: Surface Creation and On-Screen Rendering**

**Goal:** Extend the CGL implementation to support rendering to an on-screen window. This involves interfacing with the Cocoa framework to create a window and a view, and then connecting our CGL context to that view for visible output.

**Implementation Steps:**

1.  **Cocoa Integration:**
    *   The winsys implementation will now need to interact with Cocoa APIs. This means including `<Cocoa/Cocoa.h>`.
    *   Update `src/vrend/meson.build` to also link against the `Cocoa.framework` on macOS.
2.  **Surface Creation (`vrend_winsys_cgl_create_surface`):**
    *   Implement this function, which will be part of the `vrend_winsys_vtable`.
    *   It will take a native window handle (`void *`) as an argument. For macOS, this handle will be an `NSView*`.
    *   The function will associate the CGL context with the `NSView`. The modern approach is to create and attach a `CAOpenGLLayer` to the view and then set the CGL context on that layer.
3.  **Buffer Swapping (`vrend_winsys_cgl_swap_buffers`):**
    *   Implement this function. It is responsible for presenting the rendered back buffer to the screen.
    *   This will be achieved by calling `CGLFlushDrawable` on the CGL context.
4.  **Context Management:** Implement `vrend_winsys_cgl_surface_make_current` to activate the rendering context for the given surface.

**Testing Strategy:**

*   **Visual Test Application:** Create a new test application using Cocoa.
*   **Test Logic:**
    1.  The application will create a standard `NSWindow` and an `NSView`.
    2.  It will initialize `virglrenderer`.
    3.  It will call `vrend_renderer_create_surface` (a new function to be added in the renderer interface), passing in the `NSView` pointer.
    4.  The application will use the `testvirgl` library to get a `virgl_context` and send a simple command stream to clear the screen to a solid color (e.g., green).
    5.  In a simple loop, it will call `virgl_renderer_force_ctx_0` and `vrend_renderer_swap_buffers`.
*   **Success Criteria:** A window appears on the screen and is filled with the specified clear color. This visually confirms that the CGL context is correctly attached to the on-screen view and that rendering commands are being processed. Test resizing the window to ensure the viewport is updated correctly.

---

### **Phase 3: Full Integration with `vtest` and `virglrenderer`**

**Goal:** Fully integrate the CGL backend so it can be used by the existing `vtest` suite. This phase focuses on ensuring compatibility with the complete `virglrenderer` command stream and resource management.

**Implementation Steps:**

1.  **Renderer Integration:**
    *   In `vrend_renderer.c`, modify `vrend_renderer_init_procs` to call `vrend_winsys_init_cgl` when on macOS. This makes the CGL backend the default on this platform.
    *   Ensure all required functions in the `vrend_winsys_vtable` are implemented, even if they are just stubs for now (e.g., functions for features not supported on CGL).
2.  **Resource Management:**
    *   The most critical part of this phase is handling resource creation, specifically scanout-capable resources that can be displayed.
    *   Implement `vrend_winsys_cgl_create_scanout_res`. This function will likely need to create a texture that is backed by an `IOSurface`. `IOSurface` is macOS's mechanism for sharing texture data efficiently between processes and the window server.
    *   When a resource is bound as a framebuffer (`VIRGL_BIND_SCANOUT`), the implementation should associate this resource with the `CAOpenGLLayer` for display.

**Testing Strategy:**

*   **Run `vtest`:** The main goal is to get the standard `vtest` suite to run to completion using the CGL backend.
*   **Test Logic:**
    1.  Build `virglrenderer` and `vtest` on macOS.
    2.  Run `./vtest-vulkan` (or the GL equivalent). `vtest` will create its own window and pass it to `virglrenderer`.
    3.  The test suite will send thousands of commands, creating resources, shaders, and complex drawing operations.
*   **Success Criteria:**
    *   `vtest` runs and exits cleanly without any OpenGL errors or crashes.
    *   The `vtest` window displays the expected complex graphical output (e.g., rotating teapots, textured cubes). This is a comprehensive test that validates the entire pipeline from command submission to on-screen rendering.
    *   Use `VREND_DEBUG="trace"` to get a trace of OpenGL calls and verify that they are consistent with the commands being sent.

---

### **Phase 4: Synchronization and Final Touches**

**Goal:** Implement synchronization mechanisms and add final polish. Proper synchronization is critical for performance and for preventing rendering artifacts in a multithreaded renderer.

**Implementation Steps:**

1.  **Fence Support:**
    *   `virglrenderer` relies on fences for GPU-GPU synchronization.
    *   The CGL context must be created with a profile that supports `GL_ARB_sync` objects. This should already be covered by requesting a 3.3+ Core Profile.
    *   The `virgl_fence.c` implementation uses standard `glFenceSync` and `glClientWaitSync`, so no custom CGL implementation is needed *if* the context correctly exposes the extension. This step is about verification.
2.  **Performance Profiling:**
    *   Use macOS's native tools (like Instruments with the OpenGL template) to profile the renderer running `vtest` or another benchmark.
    *   Identify any major bottlenecks. Pay close attention to texture uploads/downloads (`vrend_transfer_queue`) and potential stalls between the CPU and GPU.
3.  **Error Handling and Logging:**
    *   Rigorously check the return values of all CGL and Cocoa API calls.
    *   Implement robust error reporting using `vrend_log` so that failures are descriptive.
    *   Integrate with the `VREND_DEBUG` environment variable to allow for conditional logging in the CGL backend.

**Testing Strategy:**

*   **Fence Test:** Run the `test_virgl_fence` test case to specifically validate the synchronization logic.
*   **Stress Testing:** Run a long, intensive `vtest` trace replay for an extended period. This can uncover rare race conditions or memory leaks.
*   **Failure Injection:** Manually introduce failures in the test app (e.g., pass an invalid `NSView*`) to ensure the error handling in the CGL backend is triggered correctly and does not crash the application.
*   **Success Criteria:** The implementation is stable, performs reasonably well, and provides clear, actionable error messages when things go wrong. All existing relevant tests pass reliably.
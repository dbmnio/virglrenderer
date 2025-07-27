# Phase 2 CGL Backend Testing

This document explains how to test the Phase 2 implementation of the CGL backend integration.

## What Phase 2 Implements

Phase 2 integrates the CGL backend with the `vrend_winsys` subsystem, providing:

- ✅ **Automatic CGL Selection**: CGL is automatically chosen on macOS when no window system is specified
- ✅ **Flag Mapping**: `VIRGL_RENDERER_USE_CGL` properly maps to `VREND_USE_CGL` internally
- ✅ **Integration**: Full integration with the renderer initialization system
- ✅ **Context Management**: Proper OpenGL context creation and management

## Testing

### Prerequisites

- macOS system (Darwin)
- Meson build system
- Clang compiler
- OpenGL framework

### Running the Tests

1. **Build and run the Phase 2 test:**
   ```bash
   ./build_phase2_test.sh
   ./test_phase2_cgl
   ```

2. **Expected output:**
   The test will run 4 main test cases:
   - **Test 1**: Automatic CGL flag selection
   - **Test 2**: Explicit CGL flag mapping  
   - **Test 3**: Context creation and management
   - **Test 4**: Multiple initialization cycles

   If all tests pass, you should see:
   ```
   🎉 ALL TESTS PASSED! Phase 2 CGL integration is working correctly.
   
   ✅ Verified functionality:
     • Automatic CGL selection on macOS
     • Explicit CGL flag processing  
     • OpenGL context creation and management
     • Multiple initialization cycles
     • Basic OpenGL operations
   
   The CGL backend is ready for Phase 3 implementation! 🚀
   ```

### What the Tests Validate

1. **Automatic Flag Selection Test**:
   - Calls `virgl_renderer_init()` with no window system flags
   - Verifies that initialization succeeds (indicating CGL was auto-selected)
   - Tests basic OpenGL functionality

2. **Explicit Flag Mapping Test**:
   - Calls `virgl_renderer_init()` with `VIRGL_RENDERER_USE_CGL` flag
   - Verifies proper flag processing and OpenGL functionality

3. **Context Creation Test**:
   - Tests creating and destroying virgl contexts
   - Validates OpenGL operations work after context creation
   - Tracks callback interactions

4. **Multiple Initialization Test**:
   - Runs multiple init/cleanup cycles
   - Ensures no resource leaks or state corruption

## Troubleshooting

### Build Issues

- **"CGL is not supported on this platform"**: You're not on macOS. This backend only works on macOS.
- **Missing OpenGL framework**: Install Xcode command line tools
- **Meson not found**: Install meson via Homebrew: `brew install meson`

### Runtime Issues

- **Initialization fails**: Check that you have OpenGL drivers installed
- **Context creation fails**: Verify your system supports OpenGL 3.3+
- **OpenGL errors**: Check system OpenGL implementation

### Manual Testing

You can also test the integration manually:

```c
#include "src/virglrenderer.h"

// Test automatic selection (no flags)
virgl_renderer_init(&cookie, 0, &callbacks);

// Test explicit selection  
virgl_renderer_init(&cookie, VIRGL_RENDERER_USE_CGL, &callbacks);
```

Both should work on macOS and create functional OpenGL contexts.

## Next Steps

With Phase 2 complete, you can proceed to Phase 3: Implementation of CoreGL Backend Functions, which will implement the detailed CGL-specific rendering functionality. 
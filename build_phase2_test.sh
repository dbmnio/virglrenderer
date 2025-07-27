#!/bin/bash

# Simple build script for Phase 2 CGL test
# This builds the test using the main project's build system

set -e

echo "🔨 Building Phase 2 CGL Test..."

# Check if we're on macOS
if [[ "$(uname)" != "Darwin" ]]; then
    echo "❌ Error: This test is designed for macOS only"
    exit 1
fi

# Build the main project if builddir doesn't exist
if [[ ! -d "builddir" ]]; then
    echo "📦 Setting up main build directory..."
    meson setup builddir -Dtests=enabled
fi

# Compile the main project to ensure all dependencies are built
echo "🔧 Building main project..."
meson compile -C builddir

# Build the Phase 2 test using the project's built libraries
echo "🧪 Building Phase 2 test..."
clang -std=c99 -Wall -Wextra \
    -I. -Ibuilddir \
    -framework OpenGL \
    -lepoxy \
    test_phase2_cgl.c \
    builddir/src/libvirglrenderer.a \
    -o test_phase2_cgl

echo "✅ Build complete! Run with: ./test_phase2_cgl" 
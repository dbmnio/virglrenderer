# This relies on normally "private" dependencies for testing.  I.e. this is
# white box testing.  So we need to link against some of the archive/object
# files directly for running this test.
#
# It also assumes the meson build for virglrenderer happened in the ./build 
# directory

clang -o test_phase3_cgl test_phase3_cgl.c -I./build/src -Isrc -I./build b
uild/src/libvirgl.a build/src/mesa/libmesa.a -framework OpenGL

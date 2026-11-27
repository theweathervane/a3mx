/* The GPU renderer's backend in use (ge_backend.h): OpenGL by default, SDL3 GPU when main.c asks. */
#include "ge_backend.h"

const GeBackend *ge_backend = &ge_backend_gl;

int ge_backend_init(void *(*gl_getproc)(const char *), int scale, int display_h) {
    GeBackendInit in = { gl_getproc, 0, 0, scale, display_h };
    return ge_backend->init(&in);
}
int ge_backend_init_sdlgpu(void *window, const char *driver, int scale, int display_h) {
    GeBackendInit in = { 0, window, driver, scale, display_h };
    ge_backend = &ge_backend_sdlgpu;
    if (ge_backend->init(&in)) return 1;
    ge_backend = &ge_backend_gl;
    return 0;
}

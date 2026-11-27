/* The GPU renderer's backend in use (ge_backend.h). OpenGL is the only one so far. */
#include "ge_backend.h"

const GeBackend *ge_backend = &ge_backend_gl;

int ge_backend_init(void *(*gl_getproc)(const char *), int scale, int display_h) {
    GeBackendInit in = { gl_getproc, scale, display_h };
    return ge_backend->init(&in);
}

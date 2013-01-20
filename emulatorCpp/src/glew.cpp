#include <GL/glew.h>
#include <cstdio>

void initGlew() {
    static bool glewInitialized = false;
    if (!glewInitialized) {
        glewInitialized = true;
        
        glewExperimental=true;
        GLenum err = glewInit();
        if (GLEW_OK != err) {
            /* Problem: glewInit failed, something is seriously wrong. */
            fprintf(stderr, "GLEW: ERROR: GLEW initialization failed! We're going to keep going anyways, but we will most likely crash.\n");
            fprintf(stderr, "GLEW: ERROR: %s\n", glewGetErrorString(err));
        }
        fprintf(stdout, "GLEW: Using GLEW %s\n", glewGetString(GLEW_VERSION));
    }
}

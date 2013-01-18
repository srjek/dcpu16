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
            fprintf(stderr, "ERROR: GLEW initialization failed! We're going to keep going anyways, but we will most likely crash.\n");
            fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        }
        fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
    }
}

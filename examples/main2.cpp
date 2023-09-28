#include <cstdio>
#include <cstdlib>
#include <X11/Xlib.h>
#include <GL/glx.h>

int main() {
    Display *display = XOpenDisplay(nullptr);
    int glxMajor, glxMinor;
    if (!glXQueryVersion(display, &glxMajor, &glxMinor)
    || (glxMajor == 1 && glxMinor < 3)
    || (glxMajor < 1)) {
        fprintf(stderr, "ERROR: invalid GLX version\n");
        exit(1);
    }
    printf("Major: %d\tMinor: %d\n", glxMajor, glxMinor);

    GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
    XVisualInfo *vis = glXChooseVisual(display, 0, att);
    printf("visual %p selected\n", (void *)vis->visualid);

    return(0);
}

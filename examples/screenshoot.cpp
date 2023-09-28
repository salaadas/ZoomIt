#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <GL/glx.h>
#include <GL/glu.h>
#include <cstring>

int main()
{
    Display *display;
    Window root;

    display = XOpenDisplay(nullptr);
    if (display == nullptr) {
        fprintf(stderr, "ERROR: could not open display\n");
        exit(1);
    }

    root = DefaultRootWindow(display);

    // now try to show the image back onto the screen
    XRRScreenConfiguration *screenConfig = XRRGetScreenInfo(display, root);
    uint16_t rate = XRRConfigCurrentRate(screenConfig);
    printf("Screen rate: %u\n", rate);

    int glxMajor, glxMinor;
    if (!glXQueryVersion(display, &glxMajor, &glxMinor) ||
    (glxMajor == 1 && glxMinor < 3) || (glxMajor < 1)) {
        fprintf(stderr, "ERROR: Invalid GLX version. Expected >= 1.3\n");
        exit(1);
    }

    int screen = DefaultScreen(display);

    printf("glxMajor: %d, glxMinor: %d\n", glxMajor, glxMinor);
    printf("GLX Extension: %s\n", glXQueryExtensionsString(display, screen));

    int attrs[] = {
        GLX_RGBA,
        GLX_DEPTH_SIZE, 24,
        GLX_DOUBLEBUFFER,
        None
    };

    XVisualInfo *vi = glXChooseVisual(display, 0, attrs);
    if (vi == nullptr) {
        fprintf(stderr, "ERROR: no appropriate visual found\n");
        exit(1);
    }

    printf("Visual %ld selected\n", vi->visualid);

    XSetWindowAttributes swa;
    swa.colormap = XCreateColormap(display, root, vi->visual, AllocNone);
    swa.event_mask = ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | ExposureMask | ClientMessage;

    swa.override_redirect = 1;
    swa.save_under = 1;

    XWindowAttributes attributes;
    XGetWindowAttributes(display, root, &attributes);

    Window appWindow = XCreateWindow(
    display, root, 0, 0, attributes.width, attributes.height, 0,
    vi->depth, InputOutput, vi->visual,
    CWColormap | CWEventMask | CWOverrideRedirect | CWSaveUnder,
    &swa
    );

    XMapWindow(display, appWindow);
    char wmName[] = "zoomit";
    char wmClass[] = "ZoomIt";
    XClassHint hints;
    hints.res_name = wmName;
    hints.res_class = wmClass;

    XStoreName(display, appWindow, wmName);
    XSetClassHint(display, appWindow, &hints);

    Atom wmDeleteMessage = XInternAtom(
    display, "WM_DELETE_WINDOW",
    0
    );
    XSetWMProtocols(display, appWindow, &wmDeleteMessage, 1);
    GLXContext glc = glXCreateContext(display, vi, nullptr, GL_TRUE);
    glXMakeCurrent(display, appWindow, glc);

    XImage *image;
    image = XGetImage(display, appWindow, 0, 0, attributes.width, attributes.height, AllPlanes, ZPixmap);
    std::vector<uint8_t> pixels;
    pixels.resize(attributes.width * attributes.height * 4); // 4 is because of RGBA
    memcpy(&pixels[0], image->data, pixels.size());

    int count = 1;
    for (auto c : pixels) {
        printf("%X ", c);
        if (count % 4 == 0) printf("\n");
        ++count;
    }
    printf("end...\n");

    double dt = 1.0 / static_cast<double>(rate);

    bool quit = false;
    while (!quit) {
        // set the focus always on the window
        XSetInputFocus(display, appWindow, RevertToParent, CurrentTime);
        XEvent xev;
        while (XPending(display) > 0) {
            XNextEvent(display, &xev);
            switch (xev.type) {
                case Expose: {
                } break;
                case MotionNotify: {
                    printf("scrolled mouse\n");
                } break;
                case ClientMessage: {
                    if ((Atom)xev.xclient.data.l[0] == wmDeleteMessage) {
                        quit = true;
                    }
                } break;
                case KeyPress: {
                    KeySym key = XLookupKeysym(&xev.xkey, 0);
                    switch (key) {
                        case XK_q:
                        case XK_Escape: {
                            quit = true;
                        } break;
                    }
                } break;
            }
        }
    }

    XSync(display, 0);

    // XFreeGC(display, gc);
    XDestroyImage(image);
    XDestroyWindow(display, appWindow);
    XCloseDisplay(display);

    return(0);
}

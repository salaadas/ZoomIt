#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

// GC gc;

constexpr int WIDTH = 300;
constexpr int HEIGHT = 300;
constexpr int BORDER = 10;

unsigned long int coral = 0;

struct Hello {
    Hello() = default;
    Display *display;
    Window win;
    int screen;
    bool running;
    GC gc;
    std::string message;
};

Hello init_x(int width, int height, const std::string &message)
{
    Hello result;
    uint32_t black, white;
    result.display = XOpenDisplay((char*)0);
    if (result.display == nullptr) {
        fprintf(stderr, "ERROR: could not open display\n");
        exit(1);
    }
    result.screen = DefaultScreen(result.display);
    black = BlackPixel(result.display, result.screen);
    white = WhitePixel(result.display, result.screen);

    // display, parent, x, y, width, height, border_width, border, background
    result.win = XCreateSimpleWindow(result.display,
    DefaultRootWindow(result.display), 0, 0, // notice how the parent of display is RootWindow
    width, height, 0, white, black);

    // display, window, window_name, icon_name, icon_pixmap, argv, argc, hints
    XSetStandardProperties(result.display, result.win,
    "My X11 application", "HI!",
    None, NULL, 0, NULL);

    // choose what input is allowed
    XSelectInput(result.display, result.win, ExposureMask | ButtonPressMask | KeyPressMask);
    result.gc = XCreateGC(result.display, result.win, 0, 0);

    XSetBackground(result.display, result.gc, white);
    XSetForeground(result.display, result.gc, black);

    // clear the window and bring it on top of the other windows
    XClearWindow(result.display, result.win);
    XMapRaised(result.display, result.win);

    return result;
}

void close_x(const Hello &h)
{
    XFreeGC(h.display, h.gc);
    XDestroyWindow(h.display, h.win);
    XCloseDisplay(h.display);
    exit(1);
}

void redraw(const Hello &h)
{
    // Draw a line
    XSetForeground(h.display, h.gc, coral);
    XDrawLine(h.display, h.win, h.gc, 10, 10, 100, 100); // currently, it is all black
    std::string text = "Hello world";
    XDrawString(h.display, h.win, h.gc, 50, 50, text.c_str(), text.length());
}

void get_colors(const Hello &h) {
    XColor tmp;
    XParseColor(h.display, DefaultColormap(h.display, h.screen), "coral", &tmp);
    XAllocColor(h.display, DefaultColormap(h.display, h.screen), &tmp);
    coral = tmp.pixel;
}

int main()
{
    Hello h = init_x(300, 300, "My app");
    char text[255];
    XEvent event;
    KeySym key;

    // while (true) {
    //     XNextEvent(dis, &event);

    //     if (event.type == Expose && event.xexpose.count == 0) {
    //         get_colors();
    //         redraw();
    //     }

    //     if (event.type == KeyPress &&
    //     XLookupString(&event.xkey, text, 255, &key, 0) == 1) {
    //         if (text[0] == 'q') close_x();
    //         printf("You pressed key: %c\n", text[0]);
    //     }

    //     if (event.type == ButtonPress) {
    //         printf("You pressed a button at (%i, %i)\n",
    //         event.xbutton.x, event.xbutton.y);
    //     }
    // }
    // close_x();
    return(0);
}

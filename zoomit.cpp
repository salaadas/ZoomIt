// https://tronche.com/gui/x/xlib/function-index.html
#include <cassert>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>
#include <iostream>
#include <cstdlib>

#include <SDL.h>
#include <SDL_opengl.h>

int main() {
    Display *display;

    display = XOpenDisplay(nullptr);
    if (display == nullptr) {
        std::cerr << "ERROR: could not open display" << std::endl;
        exit(1);
    }

    Window root = DefaultRootWindow(display);

    XWindowAttributes attributes;
    XGetWindowAttributes(display, root, &attributes);

    int imgWidth = attributes.width, imgHeight = attributes.height;
    XImage *image = XGetImage(display, root, 0, 0, imgWidth, imgHeight, AllPlanes, ZPixmap);

    printf("Width: %d\t\tHeight: %d\n", imgWidth, imgHeight);
    printf("Bits per pixel: %d\n", image->bits_per_pixel);

    assert(image->bits_per_pixel == 32);

    // // save to ppm
    // FILE *f = fopen("screenshoot.ppm", "wb");
    // if (!f) {
    //     fprintf(stderr, "ERROR: could not open file to save image to\n");
    //     exit(1);
    // }

    // fprintf(f,
    //         "P6\n"
    //         "%d %d\n"
    //         "255\n", imgWidth, imgHeight);
    // for (int i = 0; i < imgHeight * imgWidth; ++i) {
    //         putc(image->data[i * 4 + 2], f);
    //         putc(image->data[i * 4 + 1], f);
    //         putc(image->data[i * 4 + 0], f);
    // }

    // fflush(f);
    // fclose(f);

    // XDestroyImage(image);
    XCloseDisplay(display);

    // Rendering
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "ERROR: could not init SDL\n");
        exit(1);
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    constexpr int TARGET_WIDTH = 1920 * 3/4,
                  TARGET_HEIGHT = 1080 * 3/4;

    SDL_Window *appWindow = SDL_CreateWindow("ZoomIt", 0, 0, TARGET_WIDTH, TARGET_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (appWindow == nullptr) {
        fprintf(stderr, "ERROR: could not create SDL window\n");
        exit(1);
    }

    SDL_SetWindowBordered(appWindow, SDL_FALSE);

    // opengl context
    SDL_GLContext appContext = SDL_GL_CreateContext(appWindow);
    if (appContext == nullptr) {
        fprintf(stderr, "ERROR: could not create SDL GL context\n");
        exit(1);
    }

    if (SDL_GL_MakeCurrent(appWindow, appContext) < 0) {
        fprintf(stderr, "ERROR: could not create make context current\n");
        exit(1);
    }

    glViewport(0, 0, TARGET_WIDTH, TARGET_HEIGHT);

    if (SDL_GL_SetSwapInterval(1) < 0) {
        fprintf(stderr, "WARNING: unable to set VSync: %s\n", SDL_GetError());
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    GLuint textures = 0;
    glGenTextures(1, &textures);
    glBindTexture(GL_TEXTURE_2D, textures);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGB,
                 imgWidth,
                 imgHeight,
                 0,
                 GL_BGRA,
                 GL_UNSIGNED_BYTE,
                 image->data);
    glEnable(GL_TEXTURE_2D);

    glOrtho(static_cast<GLfloat>(imgWidth)  * -0.5,
            static_cast<GLfloat>(imgWidth)  *  0.5,
            static_cast<GLfloat>(imgHeight) *  0.5,
            static_cast<GLfloat>(imgHeight) * -0.5,
            -1.0, 1.0);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MIN_FILTER,
                    GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MAG_FILTER,
                    GL_NEAREST);

    XDestroyImage(image);

    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

    bool quit = false;
    SDL_StartTextInput();

    float scaleMagnitude = 1.0f;
    struct {
        double x=0.0f, y=0.0f;
    } mAnchor;
    struct {
        double x=0.0f, y=0.0f;
    } mTranslate;

    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT: {quit = true; break;}
                case SDL_MOUSEWHEEL: {
                    if (e.wheel.y > 0) {
                        scaleMagnitude += 0.1;
                    } else if (e.wheel.y < 0) {
                        scaleMagnitude -= 0.1;
                    }
                } break;
                case SDL_MOUSEMOTION: {
                    if (e.button.button == SDL_BUTTON(SDL_BUTTON_LEFT)) {
                        mAnchor.x = e.motion.x - mTranslate.x;
                        mAnchor.y = e.motion.y - mTranslate.y;
                    }
                    mTranslate.x = e.motion.x - mAnchor.x;
                    mTranslate.y = e.motion.y - mAnchor.y;
                } break;
                case SDL_TEXTINPUT: {
                    if (e.text.text[0] == 'q') quit = true;
                    if (e.text.text[0] == 'w') scaleMagnitude += 0.1;
                    if (e.text.text[0] == 's') scaleMagnitude -= 0.1;
                } break;
            }
        }

        glClear(GL_COLOR_BUFFER_BIT);

        glPushMatrix();

        glScalef(scaleMagnitude, scaleMagnitude, 1.0);
        glTranslatef(mAnchor.x, mAnchor.y, 0.0);

        glBegin(GL_QUADS);
        glTexCoord2i(0, 0);
        glVertex2f(imgWidth * -0.5, imgHeight * -0.5);

        glTexCoord2i(1, 0);
        glVertex2f(imgWidth * 0.5,  imgHeight * -0.5);

        glTexCoord2i(1, 1);
        glVertex2f(imgWidth * 0.5,  imgHeight * 0.5);

        glTexCoord2i(0, 1);
        glVertex2f(imgWidth * -0.5, imgHeight * 0.5);
        glEnd();

        glFlush();
        glPopMatrix();

        SDL_GL_SwapWindow(appWindow);
    }
    SDL_StopTextInput();
    SDL_DestroyWindow(appWindow);
    SDL_Quit();

    return(0);
}

// https://tronche.com/gui/x/xlib/function-index.html
#include <cmath>
#include <cassert>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>
#include <iostream>
#include <cstdlib>

#include <SDL.h>
#include <SDL_opengl.h>

struct Screenshoot {
    Display *display;
    Window   root;
    XImage  *image=nullptr;
    int   width, height;
    char *data=nullptr;

    Screenshoot() = delete;
    explicit Screenshoot(Display *dsp, Window r, int w, int h)
        : display(dsp), root(r), width(w), height(h)
    {}

    void capture() {
        if (image != nullptr) XDestroyImage(image);
        image = XGetImage(display, root, 0, 0, width, height, AllPlanes, ZPixmap);
        printf("Bits per pixel: %d\n", image->bits_per_pixel);
        assert((image->bits_per_pixel == 32) && "ASSERT: Image's bits per pixel should be 32");
        data = image->data;
    }

    void saveToPPM(const char *fp) {
        FILE *f = fopen(fp, "wb");
        if (!f) {
            fprintf(stderr, "ERROR: could not open file to save image to %s\n", fp);
            exit(1);
        }
        fprintf(f,
                "P6\n"
                "%d %d\n"
                "255\n", width, height);
        for (int i = 0; i < width * height; ++i) {
                putc(data[i * 4 + 2], f);
                putc(data[i * 4 + 1], f);
                putc(data[i * 4 + 0], f);
        }
        fflush(f);
        fclose(f);
    }
};

struct V2 {
    double x, y;
    explicit V2(double x_, double y_) : x(x_), y(y_)
    {}

    explicit V2(double v) : x(v), y(v)
    {}

    V2 operator+(V2 const &r) {return V2(x + r.x, y + r.y);}
    V2 operator-(V2 const &r) {return V2(x - r.x, y - r.y);}
    V2 operator*(V2 const &r) {return V2(x * r.x, y * r.y);}
    V2 operator/(V2 const &r) {return V2(x / r.x, y / r.y);}

    V2 &operator+=(V2 const &rval) {x += rval.x; y += rval.y; return *this;}
    V2 &operator-=(V2 const &rval) {x -= rval.x; y -= rval.y; return *this;}
    V2 &operator*=(V2 const &rval) {x *= rval.x; y *= rval.y; return *this;}
    V2 &operator/=(V2 const &rval) {x /= rval.x; y /= rval.y; return *this;}

    double len() {
        return sqrt(x * x + y * y);
    }

    V2 normalized() {
        if (fabs(len()) < 1e-9) {
            return V2(0.0);
        }
        return V2(x / len(), y / len());
    }
};

float scaleMagnitude = 1.0f;
float deltaScale     = 0.0f;
V2 mTranslate(0.0);
V2 mPrev(0.0);
V2 velocity(0.0);
V2 mouseCurrent(0.0);
V2 mousePrev(0.0);
bool isDragging = false;

V2 world(V2 vec)
{
    return (vec - mTranslate) / V2(scaleMagnitude);
}

template<typename T>
T sign(T i)
{
    return (i > 0.0) ? 1 : (i == 0) ? 0 : -1;
}

void update(double dt)
{
    if (fabs(deltaScale) > 0.5) {
        V2 worldPoint0 = world(mouseCurrent);
        scaleMagnitude += scaleMagnitude + deltaScale * dt; // should we put a constraint on the minimum scale???
        V2 worldPoint1 = world(mouseCurrent);
        mTranslate += worldPoint1 - worldPoint0;
        deltaScale -= sign(deltaScale) * 5.0 * dt;
    }

    if (!isDragging && velocity.len() > 20.0) {
        mTranslate += velocity * V2(dt);
        velocity   -= velocity.normalized() * V2(dt) * V2(100.0);
    }
}

int main()
{
    Display *display;

    display = XOpenDisplay(nullptr);
    if (display == nullptr) {
        std::cerr << "ERROR: could not open display" << std::endl;
        exit(1);
    }
    Window root = DefaultRootWindow(display);

    XWindowAttributes attributes;
    XGetWindowAttributes(display, root, &attributes);

    Screenshoot scroot(display, root, attributes.width, attributes.height);
    scroot.capture();

    XCloseDisplay(display);

    // Rendering
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "ERROR: could not init SDL\n");
        exit(1);
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    constexpr int TARGET_WIDTH  = 1920 * 3/4,
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
                 scroot.width,
                 scroot.height,
                 0,
                 GL_BGRA,
                 GL_UNSIGNED_BYTE,
                 scroot.data);
    glEnable(GL_TEXTURE_2D);

    glOrtho(0,
            static_cast<GLfloat>(scroot.width),
            static_cast<GLfloat>(scroot.height),
            0,
            -1.0, 1.0);

    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MIN_FILTER,
                    GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MAG_FILTER,
                    GL_NEAREST);

    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glViewport(0, 0, scroot.width, scroot.height);

    SDL_StartTextInput();
    bool quit = false;

    uint64_t lastTick = 0;
    uint64_t currentTick = SDL_GetPerformanceCounter();
    double deltaTime = 0;
    while (!quit) {
        glViewport(0, 0, TARGET_WIDTH, TARGET_HEIGHT);
        SDL_Event e;
        uint32_t startTick = SDL_GetTicks();
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT: {quit = true; break;}

                // Relating to zooming
                case SDL_MOUSEWHEEL: {
                    if (e.wheel.y > 0) {
                        deltaScale += 1.0;
                    } else if (e.wheel.y < 0) {
                        deltaScale -= 1.0;
                    }
                } break;

                // Relating to panning
                case SDL_MOUSEBUTTONDOWN: {
                    mousePrev = mouseCurrent;
                    isDragging = true;
                } break;
                case SDL_MOUSEBUTTONUP: {
                    isDragging = false;
                } break;
                case SDL_MOUSEMOTION: {
                    if (isDragging) {
                        mTranslate += world(mouseCurrent) - world(mousePrev);
                        velocity = (mouseCurrent - mousePrev) * V2(20.0);
                        mousePrev = mouseCurrent;
                    }
                    mouseCurrent.x = e.motion.x;
                    mouseCurrent.y = e.motion.y;
                } break;

                case SDL_TEXTINPUT: {
                    if (e.text.text[0] == 'q') quit = true;
                    if (e.text.text[0] == 'w') {scaleMagnitude += 0.1;}
                    if (e.text.text[0] == 's') {scaleMagnitude -= 0.1;}
                    if (e.text.text[0] == '0') {
                        mousePrev      = V2(0.0);
                        mTranslate     = V2(0.0);
                        mouseCurrent   = V2(0.0);
                        scaleMagnitude = 1.0;
                    }
                } break;
            }
        }

        // update
        lastTick = currentTick;
        currentTick = SDL_GetPerformanceCounter();
        deltaTime = (double)((currentTick - lastTick) / (double)SDL_GetPerformanceFrequency());
        // printf("%f\n", deltaTime);
        update(deltaTime);

        // rendering
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glPushMatrix();

        glScalef(scaleMagnitude, scaleMagnitude, 1.0);
        glTranslatef(mTranslate.x, mTranslate.y, 0.0);

        glBegin(GL_QUADS);
        glTexCoord2i(0, 0);
        glVertex2f(0, 0);

        glTexCoord2i(1, 0);
        glVertex2f(scroot.width,  0.0);

        glTexCoord2i(1, 1);
        glVertex2f(scroot.width,  scroot.height);

        glTexCoord2i(0, 1);
        glVertex2f(0.0, scroot.height);
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

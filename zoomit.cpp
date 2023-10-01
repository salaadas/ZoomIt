// NEXT GOAL -> refactor code

#include <X11/Xlib.h> // ----> https://tronche.com/gui/x/xlib/function-index.html
#include <X11/Xutil.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include <SDL.h>
#include <GL/glew.h>
#include <SDL_opengl.h>
#include <GL/glext.h>
#include <GL/glu.h>

namespace GloballyAvail
{
    float time = 0;
}

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

namespace Mouse
{
    V2 current(0.0), previous(0.0);
    bool isDragging      = false;
    float scaleMagnitude = 1.0f,
          deltaScale     = 0.0f;
}

namespace Camera
{
    V2 p(0.0); // position for the top left corner of the camera
    V2 velocity(0.0);
    V2 scalePivot(0.0);
}

#define INITIAL_RAD (60.0f)
namespace Lamp
{
    bool isEnabled = false;
    float radius   = INITIAL_RAD;
    float deltaRad = 0.0f;
    float shadow   = 0.0f;
}

template<typename T>
T sign(T i)
{
    return (i >= 0.0) ? 1 : -1;
}

V2 world(V2 vec)
{
    return (vec - Camera::p) / V2(Mouse::scaleMagnitude);
}

void update(double dt)
{
    if (fabs(Mouse::deltaScale) > 0.5) {
        V2 worldPoint0        = world(Mouse::current);
        // should we put a constraint on the minimum scale???
        Mouse::scaleMagnitude += Mouse::deltaScale * dt;
        if (Mouse::scaleMagnitude < 0.5) {
            Mouse::scaleMagnitude = 0.5;
            Mouse::deltaScale     = 0.0;
        } else {
            V2 worldPoint1     = world(Mouse::current);
            Camera::p         += worldPoint1 - worldPoint0;
            Mouse::deltaScale -= sign(Mouse::deltaScale) * 5.0 * dt;
        }
    }
    if (!Mouse::isDragging && Camera::velocity.len() > 20.0) {
        Camera::p        += Camera::velocity * V2(5 * dt);
        Camera::velocity -= Camera::velocity.normalized() * V2(dt) * V2(100.0);
    }
    // lamp
    if (abs(Lamp::deltaRad) > 1.0) {
        Lamp::radius    = std::max(0.0, Lamp::radius + Lamp::deltaRad * dt);
        Lamp::deltaRad -= Lamp::deltaRad * 10.0 * dt;
    }
    if (Lamp::isEnabled) {
        Lamp::shadow = std::min(Lamp::shadow + 6.0 * dt, 0.8);
    } else {
        Lamp::shadow = std::max(Lamp::shadow - 6.0 * dt, 0.0);
    }
}

namespace Gfx
{
    enum Program {
        P_SCENE = 0,
        P_DESKTOP,
        P_COUNT
    };

    enum Uniforms {
        U_IMG_SZ = 0,
        U_SCREEN_SCALE,
        U_CAMERA,
        U_TIME,
        U_LAMP_POS,
        U_LAMP_SHADOW,
        U_LAMP_RADIUS,
        U_COUNT
    };

    static_assert(U_COUNT == 7, "Update list of uniforms");
    static const char *uName[U_COUNT] = {
        [U_IMG_SZ]       = "imgSize",
        [U_SCREEN_SCALE] = "scale",
        [U_CAMERA]       = "camera",
        [U_TIME]         = "iTime",
        [U_LAMP_POS]     = "lampPos",
        [U_LAMP_SHADOW]  = "shadow",
        [U_LAMP_RADIUS]  = "radius",
    };
    enum VertexAttrib {
        VA_POS = 0,
        VA_COLOR,
        VA_TEXCOORD,
        VA_COUNT
    };
    enum Textures {
        TEX_DESKTOP = 0,
        TEX_COUNT
    };

    GLuint vao, vbo, ebo;
    GLuint texID;

    GLfloat vbData[] = {
        // positions         // colors          // texture coords
         1.0f,  1.0f, 0.0f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f, // top right
         1.0f, -1.0f, 0.0f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f, // bottom right
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f, // bottom left
        -1.0f,  1.0f, 0.0f,  1.0f, 1.0f, 0.0f,  0.0f, 0.0f  // top left
    };

    GLuint indices[] = {
        0, 1, 3,
        1, 2, 3
    };

    void initVertexAttrib(GLenum usage, std::function<void()> &&fn)
    {
        glGenVertexArrays(1, &vao);
        glGenBuffers     (1, &vbo);
        glGenBuffers     (1, &ebo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vbData), vbData, usage);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, usage);

        fn(); // let user set which data coreespond to which section

        printf("Init vertex attributes successfully\n");
    }

    void initTexture(void *data, V2 &&dimensions)
    {
        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_2D, texID);
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGB,
                     dimensions.x,
                     dimensions.y,
                     0,
                     GL_BGRA,
                     GL_UNSIGNED_BYTE,
                     data);
        glTexParameteri(GL_TEXTURE_2D,
                        GL_TEXTURE_MIN_FILTER,
                        GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,
                        GL_TEXTURE_MAG_FILTER,
                        GL_NEAREST);
        glEnable(GL_TEXTURE_2D);
        printf("Init texture successfully\n");
    }

    GLuint compileShader(const char *shaderSource, GLenum shaderType)
    {
        GLuint shader = glCreateShader(shaderType);
        if (shader == 0) {
            fprintf(stderr, "ERROR: Could not create shader\n");
            exit(1);
        }
        glShaderSource(shader, 1, &shaderSource, 0);
        glCompileShader(shader);

        GLint compileStatus; glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
        if (!compileStatus) {
            GLchar buffer[1024]; int length;
            glGetShaderInfoLog(shader, sizeof(buffer), &length, buffer);
            fprintf(stderr, "ERROR: Could not compile shader because of:\n%s\n", buffer);
            exit(1);
        }
        printf("Compiled shader successfully\n");
        return shader;
    }

    GLuint loadShaderFromFile(std::string fp, GLenum shaderType)
    {
        std::ifstream fs(fp, std::ifstream::in);
        std::string res("");
        for (std::string line; fs.good(); ) {
            std::getline(fs, line);
            res.insert(res.length(), line);
            res.insert(res.length(), "\n");
        }
        fs.close();
        printf("Load shader from %s successfully\n", fp.c_str());
        return compileShader(res.c_str(), shaderType);
    }

    GLuint linkProgram(GLuint &vertShader, GLuint &fragShader)
    {
        GLuint program = glCreateProgram();
        if (!program) {
            fprintf(stderr, "ERROR: Could not create shader program\n");
            exit(1);
        }
        glAttachShader(program, vertShader);
        glAttachShader(program, fragShader);
        glLinkProgram(program);
        GLint linkStatus;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (!linkStatus) {
            GLchar buffer[1024]; int length;
            glGetProgramInfoLog(program, sizeof(buffer), &length, buffer);
            fprintf(stderr, "ERROR: Could not link shader program because of:\n%s\n", buffer);
            exit(1);
        }
              printf("Link program successfully\n");
        return program;
    }

    GLuint createProgram(const char *vertPath, const char *fragPath)
    {
        GLuint vs = loadShaderFromFile(vertPath, GL_VERTEX_SHADER);
        GLuint fs = loadShaderFromFile(fragPath, GL_FRAGMENT_SHADER);
        return linkProgram(vs, fs);
              printf("Create program successfully\n");
    }

    void renderAll(std::vector<std::function<void()>> fns)
    {
        glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        std::for_each(fns.begin(), fns.end(), [](std::function<void()> f) {f();});
    }

    void cleanup()
    {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
        printf("Clean up successfully\n");
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

    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    const int TARGET_WIDTH  = attributes.width,
              TARGET_HEIGHT = attributes.height;

    SDL_Window *appWindow = SDL_CreateWindow("ZoomIt", 0, 0, TARGET_WIDTH, TARGET_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (appWindow == nullptr) {
        fprintf(stderr, "ERROR: could not create SDL window\n");
        exit(1);
    }

    SDL_SetWindowBordered(appWindow, SDL_FALSE);

    // opengl context
    SDL_GLContext appContext = SDL_GL_CreateContext(appWindow);
    if (appContext == nullptr) {
        fprintf(stderr, "ERROR: could not create SDL GL context: %s\n", SDL_GetError());
        exit(1);
    }

    if (SDL_GL_MakeCurrent(appWindow, appContext) < 0) {
        fprintf(stderr, "ERROR: could not create make context current: %s\n", SDL_GetError());
        exit(1);
    }

    if (SDL_GL_SetSwapInterval(1) < 0) {
        fprintf(stderr, "WARNING: unable to set VSync: %s\n", SDL_GetError());
        exit(1);
    }

    GLenum glewInitStatus = glewInit();
    if (glewInitStatus != GLEW_OK) {
        fprintf(stderr, "ERROR: could not init glew\n");
        exit(1);
    }

    if (!GLEW_VERSION_2_1) {
        fprintf(stderr, "ERROR: need glew to be >= 2.1\n");
        exit(1);
    }

    GLuint programs[Gfx::P_COUNT];
    programs[Gfx::P_SCENE]   = Gfx::createProgram("shaders/bg.vert", "shaders/bg.frag");
    programs[Gfx::P_DESKTOP] = Gfx::createProgram("shaders/screen.vert", "shaders/screen.frag");

    Gfx::initVertexAttrib(GL_STATIC_DRAW, []() {
        // position
        glVertexAttribPointer(Gfx::VA_POS, 3,
        GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(Gfx::VA_POS);
        // color attribute
        glVertexAttribPointer(Gfx::VA_COLOR, 3,
        GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(Gfx::VA_COLOR);
        // texture coord attribute
        glVertexAttribPointer(Gfx::VA_TEXCOORD, 2,
        GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(Gfx::VA_TEXCOORD);
    });

    Gfx::initTexture(scroot.data, V2(scroot.width, scroot.height));

    glViewport(0, 0, scroot.width, scroot.height);
    glUseProgram(programs[Gfx::P_DESKTOP]);

    SDL_StartTextInput();
    bool quit = false;
    uint64_t lastTick = 0;
    uint64_t currentTick = SDL_GetPerformanceCounter();
    double deltaTime = 0;

    while (!quit) {
        glViewport(0, 0, TARGET_WIDTH, TARGET_HEIGHT);
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT: {quit = true; break;}

                // Relating to zooming
                case SDL_MOUSEWHEEL: {
                    if (e.wheel.y > 0) {
                        if ((SDL_GetModState() & KMOD_CTRL) && Lamp::isEnabled) {
                            Lamp::deltaRad += INITIAL_RAD;
                        } else {
                            Mouse::deltaScale += 1.0;
                            Camera::scalePivot = Mouse::current;
                        }
                    } else if (e.wheel.y < 0) {
                        if ((SDL_GetModState() & KMOD_CTRL) && Lamp::isEnabled) {
                            Lamp::deltaRad -= INITIAL_RAD;
                        } else {
                            Mouse::deltaScale -= 1.0;
                            Camera::scalePivot = Mouse::current;
                        }
                    }
                } break;

                // Relating to panning
                case SDL_MOUSEBUTTONDOWN: {
                    Mouse::previous = Mouse::current;
                    Mouse::isDragging = true;
                } break;
                case SDL_MOUSEBUTTONUP: {
                    Mouse::isDragging = false;
                } break;
                case SDL_MOUSEMOTION: {
                    if (Mouse::isDragging) {
                        Camera::p += world(Mouse::current) - world(Mouse::previous);
                        Camera::velocity = (Mouse::current - Mouse::previous) * V2(20.0);
                        Mouse::previous = Mouse::current;
                    }
                    Mouse::current.x = e.motion.x;
                    Mouse::current.y = e.motion.y;
                } break;

                case SDL_TEXTINPUT: {
                    if (e.text.text[0] == 'q') quit = true;
                    if (e.text.text[0] == '0') {
                        Mouse::previous       = V2(0.0);
                        Mouse::current        = V2(0.0);
                        Mouse::scaleMagnitude = 1.0;
                        Camera::p             = V2(0.0);
                        Camera::velocity      = V2(0.0);
                    }
                    if (e.text.text[0] == 'f') {
                        Lamp::isEnabled = !Lamp::isEnabled;
                    }
                } break;
            }
        }

        // update
        lastTick = currentTick;
        currentTick = SDL_GetPerformanceCounter();
        deltaTime = (double)((currentTick - lastTick) / (double)SDL_GetPerformanceFrequency());
        update(deltaTime);
        GloballyAvail::time += deltaTime;

        // rendering
        using namespace Gfx;
        Gfx::renderAll({
            [&]() {
                glBindTexture(GL_TEXTURE_2D, texID);
                glUseProgram(programs[P_SCENE]);
                glUniform1f(glGetUniformLocation(programs[P_SCENE], uName[U_TIME]), GloballyAvail::time);
                glBindVertexArray(vao);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            },
            [&]() {
                glBindTexture(GL_TEXTURE_2D, texID);
                glUseProgram(programs[P_DESKTOP]);
                glUniform2f(glGetUniformLocation(programs[P_DESKTOP], uName[U_CAMERA]), Camera::p.x, Camera::p.y);
                glUniform1f(glGetUniformLocation(programs[P_DESKTOP], uName[U_SCREEN_SCALE]), Mouse::scaleMagnitude);
                glUniform2f(glGetUniformLocation(programs[P_DESKTOP], uName[U_IMG_SZ]), scroot.width, scroot.height);
                glUniform2f(glGetUniformLocation(programs[P_DESKTOP], uName[U_LAMP_POS]), Mouse::current.x, Mouse::current.y);
                glUniform1f(glGetUniformLocation(programs[P_DESKTOP], uName[U_LAMP_RADIUS]), Lamp::radius);
                glUniform1f(glGetUniformLocation(programs[P_DESKTOP], uName[U_LAMP_SHADOW]), Lamp::shadow);
                glBindVertexArray(vao);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }
        });
        SDL_GL_SwapWindow(appWindow);
    }

    Gfx::cleanup();

    SDL_StopTextInput();
    SDL_DestroyWindow(appWindow);
    SDL_Quit();

    return(0);
}

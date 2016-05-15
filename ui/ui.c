#include "ui/ui.h"

#include <SDL.h>
#define GL_GLEXT_PROTOTYPES
#include <SDL_opengl.h>
#include "util/err.h"
#include "util/config.h"
#include "util/glsl.h"
#include <stdio.h>
#include <stdbool.h>
#include <GL/glu.h>

static SDL_Window* window;
static SDL_GLContext context;
static bool quit;
static GLhandleARB main_shader;
static int ww;
static int wh;

static void set_coords() {
    glViewport(0, 0, ww, wh);
    glLoadIdentity();
    gluOrtho2D(0, ww, 0, wh);
}

void ui_init() {
    // Init SDL
    if(SDL_Init(SDL_INIT_VIDEO) < 0) FAIL("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    ww = config.ui.window_width;
    wh = config.ui.window_height;

    window = SDL_CreateWindow("Radiance", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, ww, wh, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if(window == NULL) FAIL("Window could not be created! SDL Error: %s\n", SDL_GetError());
    context = SDL_GL_CreateContext(window);
    if(context == NULL) FAIL("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
    if(SDL_GL_SetSwapInterval(1) < 0) fprintf(stderr, "Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());

    // Init OpenGL
    GLenum e;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    set_coords();
    glClearColor(0, 0, 0, 1);
    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));

    if((main_shader = load_shader("resources/ui_main.glsl")) == 0) FAIL("Could not load UI main shader!\n%s", load_shader_error); // careful for leaks...
}

static void handle_key(SDL_Event * e) {
    printf("key\n");
}

static void handle_window(SDL_Event * e) {
    switch(e->window.event) {
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            ww = e->window.data1;
            wh = e->window.data2;
            set_coords();
    }
}

static void render() {
    // Clear color buffer
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgramObjectARB(main_shader);

    GLint location = glGetUniformLocationARB(main_shader, "iResolution");
    glUniform2fARB(location, ww, wh);

    glBegin(GL_QUADS);
        glVertex2f(0, 0);
        glVertex2f(0, wh);
        glVertex2f(ww, wh);
        glVertex2f(ww, 0);
    glEnd();
    glUseProgramObjectARB(0);
}

void ui_run() {
        SDL_Event e;
        SDL_StartTextInput();

        quit = false;
        while(!quit) {
            while(SDL_PollEvent(&e) != 0) {
                switch(e.type) {
                    case SDL_QUIT:
                        quit = true;
                        break;
                    case SDL_TEXTINPUT:
                        handle_key(&e);
                        break;
                    case SDL_WINDOWEVENT:
                        handle_window(&e);
                        break;
                }
            }

            render();

            SDL_GL_SwapWindow(window);
        }

        SDL_StopTextInput();
}

void ui_close() {
    glDeleteObjectARB(main_shader);
    SDL_DestroyWindow(window);
    window = NULL;
    SDL_Quit();
}

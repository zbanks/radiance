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

static SDL_Window* window = NULL;
static SDL_GLContext context;
static bool quit;
static GLhandleARB main_shader;

void ui_init() {
    // Init SDL
    if(SDL_Init(SDL_INIT_VIDEO) < 0) FAIL("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    window = SDL_CreateWindow("Radiance", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, config.ui.window_width, config.ui.window_height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if(window == NULL) FAIL("Window could not be created! SDL Error: %s\n", SDL_GetError());
    context = SDL_GL_CreateContext(window);
    if(context == NULL) FAIL("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
    if(SDL_GL_SetSwapInterval(1) < 0) fprintf(stderr, "Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());

    // Init OpenGL
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glClearColor(0, 0, 0, 1);
    if(glGetError() != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(glGetError()));

    if((main_shader = load_shader("resources/ui_main.glsl")) == 0) FAIL("Could not load UI main shader!\n%s", load_shader_error); // careful for leaks...
}

static void handle_key(SDL_Event e) {
    printf("key\n");
}

static void render() {
    // Clear color buffer
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Render quad
    glRotatef(0.4f,0.0f,1.0f,0.0f);    // Rotate The cube around the Y axis
    glRotatef(0.2f,1.0f,1.0f,1.0f);
    glColor3f(0.0f,1.0f,0.0f); 

    glUseProgramObjectARB(main_shader);
    glBegin(GL_QUADS);
        glVertex2f(-0.5f, -0.5f);
        glVertex2f(0.5f, -0.5f);
        glVertex2f(0.5f, 0.5f);
        glVertex2f(-0.5f, 0.5f);
    glEnd();
    glUseProgramObjectARB(0);
}

void ui_run() {
        SDL_Event e;
        SDL_StartTextInput();

        quit = false;
        while(!quit) {
            while(SDL_PollEvent(&e) != 0) {
                if(e.type == SDL_QUIT) {
                    quit = true;
                } else if(e.type == SDL_TEXTINPUT) {
                    handle_key(e);
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

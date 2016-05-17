#include "ui/ui.h"

#include <SDL.h>
#define GL_GLEXT_PROTOTYPES
#include <SDL_opengl.h>
#include "deck/deck.h"
#include "util/config.h"
#include "util/err.h"
#include "util/glsl.h"
#include <stdio.h>
#include <stdbool.h>
#include <GL/glu.h>

static SDL_Window* window;
static SDL_GLContext context;
static bool quit;
static GLhandleARB main_shader;
static GLhandleARB pat_shader;
static GLhandleARB blit_shader;
static int ww;
static int wh;
static GLuint fb;
static GLuint * pattern_textures;

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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0, 0, 0, 0);
    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));

    // Make a framebuffer that isn't the screen to draw on
    glGenFramebuffersEXT(1, &fb);
    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));

    // Init pattern textures
    pattern_textures = calloc(config.ui.n_patterns, sizeof(GLuint));
    if(pattern_textures == NULL) FAIL("Could not allocate %d textures.", config.ui.n_patterns);
    glGenTextures(config.ui.n_patterns, pattern_textures);
    for(int i = 0; i < config.ui.n_patterns; i++) {
        glBindTexture(GL_TEXTURE_2D, pattern_textures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, config.ui.pattern_width, config.ui.pattern_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));

    if((blit_shader = load_shader("resources/blit.glsl")) == 0) FAIL("Could not load blit shader!\n%s", load_shader_error);
    if((main_shader = load_shader("resources/ui_main.glsl")) == 0) FAIL("Could not load UI main shader!\n%s", load_shader_error);
    if((pat_shader = load_shader("resources/ui_pat.glsl")) == 0) FAIL("Could not load UI pattern shader!\n%s", load_shader_error);
}

void ui_close() {
    free(pattern_textures);
    glDeleteObjectARB(main_shader);
    SDL_DestroyWindow(window);
    window = NULL;
    SDL_Quit();
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

static void fill(float w, float h) {
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(0, h);
    glVertex2f(w, h);
    glVertex2f(w, 0);
    glEnd();
}

static void blit(float x, float y, float w, float h) {
    GLint location;
    location = glGetUniformLocationARB(blit_shader, "iPosition");
    glUniform2fARB(location, x, y);
    location = glGetUniformLocationARB(blit_shader, "iResolution");
    glUniform2fARB(location, w, h);

    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x, y + h);
    glVertex2f(x + w, y + h);
    glVertex2f(x + w, y);
    glEnd();
}

static GLuint xxx_pattern_tex = 0;

static void render() {
    GLint location;
    GLenum e;

    // Render the eight patterns
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb);

    glPushMatrix();
    glLoadIdentity();
    int pw = config.ui.pattern_width;
    int ph = config.ui.pattern_height;
    gluOrtho2D(0, pw, 0, ph);
    glUseProgramObjectARB(pat_shader);
    location = glGetUniformLocationARB(pat_shader, "iResolution");
    glUniform2fARB(location, pw, ph);
    glUseProgramObjectARB(pat_shader);
    location = glGetUniformLocationARB(pat_shader, "iSelection");
    glUniform1iARB(location, false);
    GLint pattern_index = glGetUniformLocationARB(pat_shader, "iPatternIndex");

    for(int i = 0; i < config.ui.n_patterns; i++) {
        glUniform1iARB(pattern_index, i);
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, pattern_textures[i], 0);
        glClear(GL_COLOR_BUFFER_BIT);
        fill(pw, ph);
    }
    glPopMatrix();

    // Render to screen
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgramObjectARB(main_shader);

    location = glGetUniformLocationARB(main_shader, "iResolution");
    glUniform2fARB(location, ww, wh);

    fill(ww, wh);

    // Blit UI elements on top
    glUseProgramObjectARB(blit_shader);
    glActiveTexture(GL_TEXTURE0);
    location = glGetUniformLocationARB(blit_shader, "iTexture");
    glUniform1iARB(location, 0);

    for(int i = 0; i < config.ui.n_patterns; i++) {
        glBindTexture(GL_TEXTURE_2D, pattern_textures[i]);
        blit(100 + 200 * i, 300, pw, ph);
    }

    // Blit zbank's thing
    glBindTexture(GL_TEXTURE_2D, xxx_pattern_tex);
    blit(10, 10, 100, 100);

    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));
}

void ui_run() {
        SDL_Event e;
        SDL_StartTextInput();

        struct deck_pattern pattern;
        int rc = deck_pattern_init(&pattern, "resources/patterns/test");
        if (rc != 0) FAIL("Unable to init pattern.");

        float time = 0;
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

            xxx_pattern_tex = deck_pattern_render(&pattern, time, 0);
            render();

            SDL_GL_SwapWindow(window);
            time += 0.1;
        }

        SDL_StopTextInput();
}


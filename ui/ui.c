#include "ui/ui.h"

#include <SDL.h>
#define GL_GLEXT_PROTOTYPES
#include <SDL_opengl.h>
#include "deck/pattern.h"
#include "util/config.h"
#include "util/err.h"
#include "util/glsl.h"
#include "main.h"
#include <stdio.h>
#include <stdbool.h>
#include <GL/glu.h>

static SDL_Window* window;
static SDL_GLContext context;
static bool quit;
static GLhandleARB main_shader;
static GLhandleARB pat_shader;
static GLhandleARB blit_shader;
static GLuint pat_fb;
static GLuint select_fb;
static GLuint select_tex;
static GLuint * pattern_textures;

// Window
static int ww; // Window width
static int wh; // Window height

// Mouse
static int mx; // Mouse X
static int my; // Mouse Y
static int mcx; // Mouse click X
static int mcy; // Mouse click Y
static enum {MOUSE_NONE, MOUSE_DRAG_INTENSITY} ma; // Mouse action
static int mp; // Mouse pattern (index)
static double mci; // Mouse click intensity

// False colors
#define HIT_NOTHING 0
#define HIT_PATTERN 1
#define HIT_INTENSITY 2

// Mapping from UI pattern -> deck & slot
static const int map_deck[8] = {0, 0, 0, 0, 1, 1, 1, 1};
static const int map_pattern[8] = {0, 1, 2, 3, 3, 2, 1, 0};

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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0, 0, 0, 0);
    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));

    // Make framebuffers
    glGenFramebuffersEXT(1, &select_fb);
    glGenFramebuffersEXT(1, &pat_fb);
    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));

    // Init select texture
    glGenTextures(1, &select_tex);
    glBindTexture(GL_TEXTURE_2D, select_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ww, wh, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, select_fb);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, select_tex, 0);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

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

static void render(bool select) {
    GLint location;
    GLenum e;

    // Render the eight patterns
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pat_fb);

    int pw = config.ui.pattern_width;
    int ph = config.ui.pattern_height;
    glUseProgramObjectARB(pat_shader);
    location = glGetUniformLocationARB(pat_shader, "iResolution");
    glUniform2fARB(location, pw, ph);
    glUseProgramObjectARB(pat_shader);
    location = glGetUniformLocationARB(pat_shader, "iSelection");
    glUniform1iARB(location, select);
    location = glGetUniformLocationARB(pat_shader, "iPreview");
    glUniform1iARB(location, 0);
    GLint pattern_index = glGetUniformLocationARB(pat_shader, "iPatternIndex");
    GLint pattern_intensity = glGetUniformLocationARB(pat_shader, "iIntensity");

    glLoadIdentity();
    gluOrtho2D(0, pw, 0, ph);
    glViewport(0, 0, pw, ph);

    for(int i = 0; i < config.ui.n_patterns; i++) {
        struct pattern * p = deck[map_deck[i]].pattern[map_pattern[i]];
        if(p != NULL) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, p->rt[0]->tex_screen[0]);
            glUniform1iARB(pattern_index, i);
            glUniform1fARB(pattern_intensity, p->intensity);
            glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, pattern_textures[i], 0);
            glClear(GL_COLOR_BUFFER_BIT);
            fill(pw, ph);
        }
    }

    // Render to screen (or select fb)
    if(select) {
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, select_fb);
    } else {
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    }

    glLoadIdentity();
    gluOrtho2D(0, ww, 0, wh);
    glViewport(0, 0, ww, wh);

    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgramObjectARB(main_shader);

    location = glGetUniformLocationARB(main_shader, "iResolution");
    glUniform2fARB(location, ww, wh);
    location = glGetUniformLocationARB(main_shader, "iSelection");
    glUniform1iARB(location, select);

    fill(ww, wh);

    // Blit UI elements on top
    glUseProgramObjectARB(blit_shader);
    glActiveTexture(GL_TEXTURE0);
    location = glGetUniformLocationARB(blit_shader, "iTexture");
    glUniform1iARB(location, 0);

    for(int i = 0; i < config.ui.n_patterns; i++) {
        struct pattern * pattern = deck[map_deck[i]].pattern[map_pattern[i]];
        if(pattern != NULL) {
            glBindTexture(GL_TEXTURE_2D, pattern_textures[i]);
            blit(100 + 200 * i, 300, pw, ph);
        }
    }

    if(!select) {
        // Blit zbank's thing
        glBindTexture(GL_TEXTURE_2D, deck->tex_output[0]);
        blit(10, 10, 100, 100);
    }

    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));
}

struct rgba {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

static struct rgba test_hit(int x, int y) {
    struct rgba data;

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, select_fb);
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &data);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    return data;
}

static void handle_mouse_move() {
    struct pattern * p;
    switch(ma) {
        case MOUSE_NONE:
            break;
        case MOUSE_DRAG_INTENSITY:
            p = deck[map_deck[mp]].pattern[map_pattern[mp]];
            if(p != NULL) {
                p->intensity = mci + (mx - mcx) * config.ui.intensity_gain_x + (my - mcy) * config.ui.intensity_gain_y;
                if(p->intensity > 1) p->intensity = 1;
                if(p->intensity < 0) p->intensity = 0;
            }
            break;
    }
}

static void handle_mouse_up() {
    ma = MOUSE_NONE;
}

static void handle_mouse_down() {
    struct rgba hit;
    hit = test_hit(mx, wh - my);
    switch(hit.r) {
        case HIT_NOTHING:
            break;
        case HIT_PATTERN:
            printf("Click pattern. Doesn't do anything\n");
            break;
        case HIT_INTENSITY:
            if(hit.g < config.ui.n_patterns) {
                struct pattern * p = deck[map_deck[mp]].pattern[map_pattern[mp]];
                if(p != NULL) {
                    ma = MOUSE_DRAG_INTENSITY;
                    mp = hit.g;
                    mcx = mx;
                    mcy = my;
                    mci = p->intensity;
                }
            }
            break;
        default:
            printf("UNHANDLED %d\n", hit.r);
            break;
    }
}

void ui_run() {
        SDL_Event e;

        float time = 0;
        quit = false;
        while(!quit) {
            render(true);

            while(SDL_PollEvent(&e) != 0) {
                switch(e.type) {
                    case SDL_QUIT:
                        quit = true;
                        break;
                    case SDL_KEYDOWN:
                        handle_key(&e);
                        break;
                    case SDL_MOUSEMOTION:
                        mx = e.motion.x;
                        my = e.motion.y;
                        handle_mouse_move();
                        break;
                    case SDL_MOUSEBUTTONDOWN:
                        mx = e.button.x;
                        my = e.button.y;
                        switch(e.button.button) {
                            case SDL_BUTTON_LEFT:
                                handle_mouse_down();
                                break;
                        }
                        break;
                    case SDL_MOUSEBUTTONUP:
                        mx = e.button.x;
                        my = e.button.y;
                        switch(e.button.button) {
                            case SDL_BUTTON_LEFT:
                                handle_mouse_up();
                                break;
                        }
                        break;
                }
            }

            deck_render(&deck[0], 0);
            render(false);

            SDL_GL_SwapWindow(window);
            time += 0.1;
        }
}


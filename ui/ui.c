#include "ui/ui.h"

#include <SDL2/SDL.h>
#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_ttf.h>
#include "pattern/pattern.h"
#include "util/config.h"
#include "util/err.h"
#include "util/glsl.h"
#include "main.h"
#include <stdio.h>
#include <stdbool.h>
#include <GL/glu.h>

static SDL_Window * window;
static SDL_GLContext context;
static SDL_Renderer * renderer;
static bool quit;
static GLhandleARB main_shader;
static GLhandleARB pat_shader;
static GLhandleARB blit_shader;
static GLhandleARB crossfader_shader;
static GLuint pat_fb;
static GLuint select_fb;
static GLuint crossfader_fb;
static GLuint select_tex;
static GLuint * pattern_textures;
static GLuint crossfader_texture;
static int pat_entry_width;
static int pat_entry_height;
static SDL_Texture * sdltex_pat_entry;

// Window
static int ww; // Window width
static int wh; // Window height

// Mouse
static int mx; // Mouse X
static int my; // Mouse Y
static int mcx; // Mouse click X
static int mcy; // Mouse click Y
static enum {MOUSE_NONE, MOUSE_DRAG_INTENSITY, MOUSE_DRAG_CROSSFADER} ma; // Mouse action
static int mp; // Mouse pattern (index)
static double mci; // Mouse click intensity

// Selection
static int selected = 0;

// False colors
#define HIT_NOTHING 0
#define HIT_PATTERN 1
#define HIT_INTENSITY 2
#define HIT_CROSSFADER 3
#define HIT_CROSSFADER_POSITION 4

// Mapping from UI pattern -> deck & slot
// TODO make this live in the INI file
static const int map_x[8] = {100, 300, 500, 700, 1100, 1300, 1500, 1700};
static const int map_y[8] = {300, 300, 300, 300, 300, 300, 300, 300};
static const int map_deck[8] = {0, 0, 0, 0, 1, 1, 1, 1};
static const int map_pattern[8] = {0, 1, 2, 3, 3, 2, 1, 0};
static const int map_selection[8] = {1, 2, 3, 4, 6, 7, 8, 9};
static const int crossfader_selection = 5;

static const int map_left[10] =  {8, 1, 1, 2, 3, 4, 5, 6, 7, 8};
static const int map_right[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 9};
static const int map_up[10] =    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
static const int map_down[10] =  {9, 9, 9, 9, 9, 9, 9, 9, 9, 9};

// Font
TTF_Font * font;
static const SDL_Color font_color = {255, 255, 255, 255};

static SDL_Texture * render_text(char * text, int * w, int * h) {
    // We need to first render to a surface as that's what TTF_RenderText
    // returns, then load that surface into a texture
    SDL_Surface *surf = TTF_RenderText_Blended(font, text, font_color);
    if(surf == NULL) FAIL("Could not create surface: %s", SDL_GetError());
    if(w != NULL) *w = surf->w;
    if(h != NULL) *h = surf->h;
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surf);
    if(texture == NULL) FAIL("Could not create texture: %s", SDL_GetError());
    // Clean up the surface and font
    SDL_FreeSurface(surf);
    return texture;
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
    if(window == NULL) FAIL("Window could not be created: %s\n", SDL_GetError());
    context = SDL_GL_CreateContext(window);
    if(context == NULL) FAIL("OpenGL context could not be created: %s\n", SDL_GetError());
    if(SDL_GL_SetSwapInterval(1) < 0) fprintf(stderr, "Warning: Unable to set VSync: %s\n", SDL_GetError());
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if(renderer == NULL) FAIL("Could not create renderer: %s\n", SDL_GetError());
    if(TTF_Init() < 0) FAIL("Could not initialize font library: %s\n", TTF_GetError());

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
    glGenFramebuffersEXT(1, &crossfader_fb);
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

    // Init crossfader texture
    glGenTextures(1, &crossfader_texture);
    glBindTexture(GL_TEXTURE_2D, crossfader_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, config.ui.crossfader_width, config.ui.crossfader_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, crossfader_fb);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, crossfader_texture, 0);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    if((e = glGetError()) != GL_NO_ERROR) FAIL("OpenGL error: %s\n", gluErrorString(e));

    if((blit_shader = load_shader("resources/blit.glsl")) == 0) FAIL("Could not load blit shader!\n%s", load_shader_error);
    if((main_shader = load_shader("resources/ui_main.glsl")) == 0) FAIL("Could not load UI main shader!\n%s", load_shader_error);
    if((pat_shader = load_shader("resources/ui_pat.glsl")) == 0) FAIL("Could not load UI pattern shader!\n%s", load_shader_error);
    if((crossfader_shader = load_shader("resources/ui_crossfader.glsl")) == 0) FAIL("Could not load UI crossfader shader!\n%s", load_shader_error);

    // Open the font
    font = TTF_OpenFont(config.ui.font, config.ui.fontsize);
    if(font == NULL) FAIL("Could not open font %s: %s\n", config.ui.font, SDL_GetError());

    sdltex_pat_entry = render_text("Fuck text", &pat_entry_width, &pat_entry_height);
}

void ui_close() {
    TTF_CloseFont(font);
    free(pattern_textures);
    glDeleteObjectARB(blit_shader);
    glDeleteObjectARB(main_shader);
    glDeleteObjectARB(pat_shader);
    glDeleteObjectARB(crossfader_shader);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    window = NULL;
    SDL_Quit();
}

static struct pattern * selected_pattern() {
    for(int i=0; i<config.ui.n_patterns; i++) {
        if(map_selection[i] == selected) return deck[map_deck[i]].pattern[map_pattern[i]];
    }
    return NULL;
}

static void set_slider_to(float v) {
    struct pattern * p = selected_pattern();
    if(p != NULL) {
        p->intensity = v;
    } else if(selected == crossfader_selection) {
        crossfader.position = v;
    }
}

static void handle_key(SDL_KeyboardEvent * e) {
    switch(e->keysym.scancode) {
        case SDL_SCANCODE_LEFT:
            selected = map_left[selected];
            break;
        case SDL_SCANCODE_RIGHT:
            selected = map_right[selected];
            break;
        case SDL_SCANCODE_UP:
            selected = map_up[selected];
            break;
        case SDL_SCANCODE_DOWN:
            selected = map_down[selected];
            break;
        case SDL_SCANCODE_ESCAPE:
            selected = 0;
            break;
        case SDL_SCANCODE_GRAVE:
            set_slider_to(0);
            break;
        case SDL_SCANCODE_1:
            set_slider_to(0.1);
            break;
        case SDL_SCANCODE_2:
            set_slider_to(0.2);
            break;
        case SDL_SCANCODE_3:
            set_slider_to(0.3);
            break;
        case SDL_SCANCODE_4:
            set_slider_to(0.4);
            break;
        case SDL_SCANCODE_5:
            set_slider_to(0.5);
            break;
        case SDL_SCANCODE_6:
            set_slider_to(0.6);
            break;
        case SDL_SCANCODE_7:
            set_slider_to(0.7);
            break;
        case SDL_SCANCODE_8:
            set_slider_to(0.8);
            break;
        case SDL_SCANCODE_9:
            set_slider_to(0.9);
            break;
        case SDL_SCANCODE_0:
            set_slider_to(1);
            break;
        default:
            break;
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
            glBindTexture(GL_TEXTURE_2D, p->tex_output);
            glUniform1iARB(pattern_index, i);
            glUniform1fARB(pattern_intensity, p->intensity);
            glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, pattern_textures[i], 0);
            glClear(GL_COLOR_BUFFER_BIT);
            fill(pw, ph);
        }
    }

    // Render the crossfader
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, crossfader_fb);

    int cw = config.ui.crossfader_width;
    int ch = config.ui.crossfader_height;
    glUseProgramObjectARB(crossfader_shader);
    location = glGetUniformLocationARB(crossfader_shader, "iResolution");
    glUniform2fARB(location, cw, ch);
    location = glGetUniformLocationARB(crossfader_shader, "iSelection");
    glUniform1iARB(location, select);
    location = glGetUniformLocationARB(crossfader_shader, "iPreview");
    glUniform1iARB(location, 0);
    location = glGetUniformLocationARB(crossfader_shader, "iPosition");
    glUniform1fARB(location, crossfader.position);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, crossfader.tex_output);

    glLoadIdentity();
    gluOrtho2D(0, cw, 0, ch);
    glViewport(0, 0, cw, ch);
    glClear(GL_COLOR_BUFFER_BIT);
    fill(cw, ch);

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
    location = glGetUniformLocationARB(main_shader, "iSelected");
    glUniform1iARB(location, selected);

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
            blit(map_x[i], map_y[i], pw, ph);
        }
    }

    glBindTexture(GL_TEXTURE_2D, crossfader_texture);
    blit(config.ui.crossfader_x, config.ui.crossfader_y, cw, ch);

    if(!select) {
        // Blit some text
        glActiveTexture(GL_TEXTURE0);
        SDL_GL_BindTexture(sdltex_pat_entry, NULL, NULL);
        blit(10, 10, pat_entry_width, pat_entry_height);
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
        case MOUSE_DRAG_CROSSFADER:
            crossfader.position = mci + (mx - mcx) * config.ui.crossfader_gain_x + (my - mcy) * config.ui.crossfader_gain_y;
            if(crossfader.position > 1) crossfader.position = 1;
            if(crossfader.position < 0) crossfader.position = 0;
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
            selected = 0;
            break;
        case HIT_PATTERN:
            if(hit.g < config.ui.n_patterns) selected = map_selection[hit.g];
            break;
        case HIT_INTENSITY:
            if(hit.g < config.ui.n_patterns) {
                struct pattern * p = deck[map_deck[hit.g]].pattern[map_pattern[hit.g]];
                if(p != NULL) {
                    ma = MOUSE_DRAG_INTENSITY;
                    mp = hit.g;
                    mcx = mx;
                    mcy = my;
                    mci = p->intensity;
                }
            }
            break;
        case HIT_CROSSFADER:
            selected = crossfader_selection;
            break;
        case HIT_CROSSFADER_POSITION:
            ma = MOUSE_DRAG_CROSSFADER;
            mcx = mx;
            mcy = my;
            mci = crossfader.position;
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
                        handle_key(&e.key);
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

            for(int i=0; i<N_DECKS; i++) {
                deck_render(&deck[i]);
            }
            crossfader_render(&crossfader, deck[0].tex_output, deck[1].tex_output);
            render(false);

            SDL_GL_SwapWindow(window);
            time += 0.1;
        }
}


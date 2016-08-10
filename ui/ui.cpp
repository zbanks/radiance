#include "util/common.h"
#include "ui/ui.h"

#include "pattern/pattern.h"
#include "util/config.h"
#include "util/err.h"
#include "util/glsl.h"
#include "util/math.h"
#include "midi/midi.h"
#include "output/output.h"
#include "audio/analyze.h"
#include "ui/render.h"
#include "output/slice.h"
#include "main.h"

#define GL_CHECK_ERROR() \
do { \
if(auto e = glGetError()) \
    FAIL("OpenGL error: %s\n", gluErrorString(e)); \
}while(false);

static SDL_Window * window;
static SDL_GLContext context;
static SDL_Renderer * renderer;
static bool quit;
static GLuint main_shader;
static GLuint pat_shader;
static GLuint blit_shader;
static GLuint crossfader_shader;
static GLuint text_shader;
static GLuint spectrum_shader;
static GLuint waveform_shader;
static GLuint strip_shader;

static GLuint pat_fb;
static GLuint select_fb;
static GLuint crossfader_fb;
static GLuint pat_entry_fb;
static GLuint spectrum_fb;
static GLuint waveform_fb;
static GLuint strip_fb;
static GLuint select_tex;
std::vector<GLuint> pattern_textures;
std::vector<SDL_Texture*> pattern_name_textures;
std::vector<std::pair<int,int> > pattern_name_sizes;
//static GLuint * pattern_textures;
//static SDL_Texture ** pattern_name_textures;
//static int * pattern_name_width;
//static int * pattern_name_height;
static GLuint crossfader_texture;
static GLuint pat_entry_texture;
static GLuint tex_spectrum_data;
static GLuint spectrum_texture;
static GLuint tex_waveform_data;
static GLuint tex_waveform_beats_data;
static GLuint waveform_texture;
static GLuint strip_texture;
static GLuint strip_vao      = 0;
static GLuint strip_vbo      = 0;
static int    strip_vbo_size = 0;
static GLuint vao;
static GLuint vbo;
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

// Strip indicators
static enum {STRIPS_NONE, STRIPS_SOLID, STRIPS_COLORED} strip_indicator = STRIPS_NONE;

// False colors
#define HIT_NOTHING 0
#define HIT_PATTERN 1
#define HIT_INTENSITY 2
#define HIT_CROSSFADER 3
#define HIT_CROSSFADER_POSITION 4

// Mapping from UI pattern -> deck & slot
// TODO make this live in the INI file
static const int map_x[16] = {100, 275, 450, 625, 1150, 1325, 1500, 1675,
                              100, 275, 450, 625, 1150, 1325, 1500, 1675,};
static const int map_y[16] = {295, 295, 295, 295, 295, 295, 295, 295,
                              55, 55, 55, 55, 55, 55, 55, 55};
static const int map_pe_x[16] = {100, 275, 450, 625, 1150, 1325, 1500, 1675,
                                 100, 275, 450, 625, 1150, 1325, 1500, 1675};
static const int map_pe_y[16] = {420, 420, 420, 420, 420, 420, 420, 420,
                                180, 180, 180, 180, 180, 180, 180, 180};
static const int map_deck[16] = {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3};
static const int map_pattern[16] = {0, 1, 2, 3, 3, 2, 1, 0, 0, 1, 2, 3, 3, 2, 1, 0};
static const int map_selection[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

static const int crossfader_selection_top = 17;
static const int crossfader_selection_bot = 18;

//                                0   1   2   3   4   5   6   7   8   9  10   11  12  13  14  15  16  17  18
static const int map_left[19] =  {8,  1,  1,  2,  3,  17, 5,  6,  7,  9,  9,  10, 11, 18, 13, 14, 15, 4,  12};
static const int map_right[19] = {1,  2,  3,  4,  17, 6,  7,  8,  8,  10, 11, 12, 18, 14, 15, 16, 16, 5,  13};
static const int map_up[19] =    {1,  1,  2,  3,  4,  5,  6,  7,  8,  1,  2,  3,  4,  5,  6,  7,  8,  17, 17};
static const int map_down[19] =  {9,  9,  10, 11, 12, 13, 14, 15, 16, 9,  10, 11, 12, 13, 14, 15, 16, 18, 18};
static const int map_space[19] = {17, 17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 17, 18};
static const int map_tab[19] =   {1,  2,  3,  4,  17, 17, 5,  6,  7,  10, 11, 12, 18, 18, 13, 14, 15, 17, 18};
static const int map_stab[19] =  {15, 1,  1,  2,  3,  6,  7,  8,  8,  9,  9,  10, 11, 14, 15, 16, 16, 17, 18};
static const int map_home[19] =  {1,  1,  1,  1,  1,  1,  1,  1,  1,  9,  9,  9,  9,  9,  9,  9,  9,  1,  9};
static const int map_end[19] =   {8,  8,  8,  8,  8,  8,  8,  8,  8, 16, 16, 16, 16, 16, 16, 16, 16,  8, 16};

// Font
TTF_Font * font;
static const SDL_Color font_color = {255, 255, 255, 255};

// Pat entry
static bool pat_entry;
static char pat_entry_text[255];

// Timing
static double l_t;

// Deck selector
static int left_deck_selector = 0;
static int right_deck_selector = 1;

// Forward declarations
static void handle_text(const char * text);
//
static void bind_vao_fill_vbo(float x, float y, float w, float h)
{
    GLfloat vertices[] = { x, y, w, h };

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    void *_vbo = glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(vertices), GL_MAP_INVALIDATE_BUFFER_BIT|GL_MAP_WRITE_BIT); 
    memcpy(_vbo,vertices,sizeof(vertices));
    glUnmapBuffer(GL_ARRAY_BUFFER);
//    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glBindVertexArray(vao);
}
static void fill(float w, float h) {
    bind_vao_fill_vbo(0., 0., w, h);
    glDrawArrays(GL_POINTS, 0, 1);
}

static SDL_Texture * render_text(char * text, int * w, int * h) {
    // We need to first render to a surface as that's what TTF_RenderText
    // returns, then load that surface into a texture
    SDL_Surface * surf;
    if(strlen(text) > 0) {
        surf = TTF_RenderText_Blended(font, text, font_color);
    } else {
        surf = TTF_RenderText_Blended(font, " ", font_color);
    }
    if(surf == NULL) FAIL("Could not create surface: %s\n", SDL_GetError());
    if(w != NULL) *w = surf->w;
    if(h != NULL) *h = surf->h;
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surf);
    if(texture == NULL) FAIL("Could not create texture: %s\n", SDL_GetError());
    SDL_FreeSurface(surf);
    return texture;
}

static void render_textbox(char * text, int width, int height) {
    GLint location;

    glUseProgram(text_shader);
    glUniform2f(0, width, height);

    int text_w;
    int text_h;

    SDL_Texture * tex = render_text(text, &text_w, &text_h);

    location = glGetUniformLocation(text_shader, "iTextResolution");
    glUniform2f(location, text_w, text_h);
    location = glGetUniformLocation(text_shader, "iText");
    glUniform1i(location, 0);
    glActiveTexture(GL_TEXTURE0);
    SDL_GL_BindTexture(tex, NULL, NULL);
    
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);
    fill(width, height);
    SDL_DestroyTexture(tex);
}

void ui_init() {
    // Init SDL
    if(SDL_Init(SDL_INIT_VIDEO) < 0) FAIL("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_RELEASE_BEHAVIOR,SDL_GL_CONTEXT_RELEASE_BEHAVIOR_FLUSH);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, GL_TRUE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,SDL_GL_CONTEXT_DEBUG_FLAG|
                                             SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    ww = config.ui.window_width;
    wh = config.ui.window_height;

    window = SDL_CreateWindow("Radiance", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, ww, wh, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if(window == NULL) FAIL("Window could not be created: %s\n", SDL_GetError());
    context = SDL_GL_CreateContext(window);
    if(context == NULL) FAIL("OpenGL context could not be created: %s\n", SDL_GetError());
    if(SDL_GL_SetSwapInterval(1) < 0) fprintf(stderr, "Warning: Unable to set VSync: %s\n", SDL_GetError());
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if(renderer == NULL) FAIL("Could not create renderer: %s\n", SDL_GetError());
    if(TTF_Init() < 0) FAIL("Could not initialize font library: %s\n", TTF_GetError());
    
    SDL_GL_MakeCurrent(window,context);
    glGetError();
    glGenBuffers(1,&vbo);
    glGenVertexArrays(1,&vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 4, NULL, GL_STATIC_DRAW);
    glBindVertexArray(vao);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2*sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // Init OpenGL
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0, 0, 0, 0);
    GL_CHECK_ERROR();

    // Make framebuffers
    glGenFramebuffers(1, &select_fb);
    glGenFramebuffers(1, &pat_fb);
    glGenFramebuffers(1, &crossfader_fb);
    glGenFramebuffers(1, &pat_entry_fb);
    glGenFramebuffers(1, &spectrum_fb);
    glGenFramebuffers(1, &waveform_fb);
    glGenFramebuffers(1, &strip_fb);
    GL_CHECK_ERROR();

    // Init select texture
    select_tex = make_texture(ww,wh);

    glBindFramebuffer(GL_FRAMEBUFFER, select_fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, select_tex, 0);

    // Init pattern textures
    pattern_textures.resize(config.ui.n_patterns);
//    pattern_textures = (GLuint*)calloc(config.ui.n_patterns, sizeof(GLuint));
//    if(pattern_textures == NULL) MEMFAIL();
    pattern_name_textures.resize(config.ui.n_patterns);
    pattern_name_sizes.resize(config.ui.n_patterns);
//    pattern_name_textures = (SDL_Texture**)calloc(config.ui.n_patterns, sizeof(SDL_Texture *));
//    pattern_name_width = (int*)calloc(config.ui.n_patterns, sizeof(int));
//    pattern_name_height = (int*)calloc(config.ui.n_patterns, sizeof(int));
//    if(pattern_name_textures == NULL || pattern_name_width == NULL || pattern_name_height == NULL) MEMFAIL();
    GL_CHECK_ERROR();
    for(int i = 0; i < config.ui.n_patterns; i++) {
        pattern_textures[i] = make_texture( config.ui.pattern_width, config.ui.pattern_height);
    }

    // Init crossfader texture
    crossfader_texture = make_texture( config.ui.crossfader_width, config.ui.crossfader_height);
    
    glBindFramebuffer(GL_FRAMEBUFFER, crossfader_fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, crossfader_texture, 0);

    // Init pattern entry texture
    pat_entry_texture = make_texture( config.ui.pat_entry_width, config.ui.pat_entry_height);

    glBindFramebuffer(GL_FRAMEBUFFER, pat_entry_fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pat_entry_texture, 0);

    // Spectrum data texture
    tex_spectrum_data = make_texture(config.audio.spectrum_bins);
    spectrum_texture = make_texture( config.ui.spectrum_width, config.ui.spectrum_height);
    // Spectrum UI element
    glBindFramebuffer(GL_FRAMEBUFFER, spectrum_fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, spectrum_texture, 0);

    // Waveform data texture
    tex_waveform_data = make_texture( config.audio.waveform_length);
    tex_waveform_beats_data = make_texture( config.audio.waveform_length);
    // Waveform UI element
    waveform_texture = make_texture( config.ui.waveform_width, config.ui.waveform_height);
    glBindFramebuffer(GL_FRAMEBUFFER, waveform_fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, waveform_texture, 0);

    // Strip indicators
    strip_texture = make_texture ( config.pattern.master_width, config.pattern.master_height);
    glBindFramebuffer(GL_FRAMEBUFFER, strip_fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, strip_texture, 0);

    // Done allocating textures & FBOs, unbind and check for errors
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GL_CHECK_ERROR();

    if((blit_shader = load_shader("#blit.glsl")) == 0) FAIL("Could not load blit shader!\n%s", get_load_shader_error().c_str());
    if((main_shader = load_shader("#ui_main.glsl")) == 0) FAIL("Could not load UI main shader!\n%s", get_load_shader_error().c_str());
    if((pat_shader = load_shader("#ui_pat.glsl")) == 0) FAIL("Could not load UI pattern shader!\n%s", get_load_shader_error().c_str());
    if((crossfader_shader = load_shader("#ui_crossfader.glsl")) == 0) FAIL("Could not load UI crossfader shader!\n%s", get_load_shader_error().c_str());
    if((text_shader = load_shader("#ui_text.glsl")) == 0) FAIL("Could not load UI text shader!\n%s", get_load_shader_error().c_str());
    if((spectrum_shader = load_shader("#ui_spectrum.glsl")) == 0) FAIL("Could not load UI spectrum shader!\n%s", get_load_shader_error().c_str());
    if((waveform_shader = load_shader("#ui_waveform.glsl")) == 0) FAIL("Could not load UI waveform shader!\n%s", get_load_shader_error().c_str());
    if((strip_shader = load_shader("#strip.v.glsl","#strip.f.glsl")) == 0) FAIL("Could not load strip indicator shader!\n%s", get_load_shader_error().c_str());

    // Stop text input
    SDL_StopTextInput();

    // Open the font
    font = TTF_OpenFont(config.ui.font, config.ui.fontsize);
    if(font == NULL) FAIL("Could not open font %s: %s\n", config.ui.font, SDL_GetError());

    // Init statics
    pat_entry = false;

    SDL_Surface * surf;
    surf = TTF_RenderText_Blended(font, "wtf, why is this necessary", font_color);
    if(surf == NULL) FAIL("Could not create surface: %s\n", SDL_GetError());
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surf);
    if(texture == NULL) FAIL("Could not create texture: %s\n", SDL_GetError());
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(texture);
}

void ui_term() {
    TTF_CloseFont(font);
    for(int i=0; i<config.ui.n_patterns; i++) {
        if(pattern_name_textures[i] != NULL) SDL_DestroyTexture(pattern_name_textures[i]);
    }
    // TODO glDeleteTextures...
    glDeleteProgram(blit_shader);
    glDeleteProgram(main_shader);
    glDeleteProgram(pat_shader);
    glDeleteProgram(crossfader_shader);
    glDeleteProgram(text_shader);
    glDeleteProgram(spectrum_shader);
    glDeleteProgram(waveform_shader);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    window = NULL;
    SDL_Quit();
}

static struct pattern * selected_pattern(int s) {
    for(int i=0; i<config.ui.n_patterns; i++) {
        if(map_selection[i] == s)
            return deck[map_deck[i]].patterns[map_pattern[i]].get();
    }
    return NULL;
}

static void set_slider_to(int s, float v) {
    if(s == crossfader_selection_top || s == crossfader_selection_bot) {
        crossfader.position = v;
    } else {
        if(auto  &&p = selected_pattern(s)) {
            p->intensity = v;
        }
    }
}

static void increment_slider(int s, float v) {
    if(s == crossfader_selection_top || s == crossfader_selection_bot) {
        crossfader.position = CLAMP(crossfader.position + v, 0., 1.);
    } else {
        if(auto && p = selected_pattern(s))
            p->intensity = CLAMP(p->intensity + v, 0., 1.);
    }
}

static void handle_key(SDL_KeyboardEvent * e) {
    bool shift = e->keysym.mod & KMOD_SHIFT;
    bool ctrl = e->keysym.mod & KMOD_CTRL;
    bool alt = e->keysym.mod & KMOD_ALT;
    (void) (shift & ctrl & alt);

    if(pat_entry) {
        switch(e->keysym.sym) {
            case SDLK_RETURN:
                for(int i=0; i<config.ui.n_patterns; i++) {
                    if(map_selection[i] == selected) {
                        if(pat_entry_text[0] == ':') {
                            if (deck[map_deck[i]].load_set(pat_entry_text+1) == 0) {
                                // TODO: Load in the correct pattern names
                            }
                        } else if(deck[map_deck[i]].load_pattern( map_pattern[i], pat_entry_text) == 0) {
                            if(pat_entry_text[0] != '\0') {
                                if(pattern_name_textures[i] )
                                    SDL_DestroyTexture(pattern_name_textures[i]);
                                pattern_name_textures[i] = render_text(pat_entry_text, &pattern_name_sizes[i].first, &pattern_name_sizes[i].second);
                            }
                        }
                        break;
                    }
                }
                pat_entry = false;
                SDL_StopTextInput();
                break;
            case SDLK_ESCAPE:
                pat_entry = false;
                SDL_StopTextInput();
                break;
            case SDLK_BACKSPACE:
                if (pat_entry_text[0] != '\0') {
                    pat_entry_text[strlen(pat_entry_text)-1] = '\0';
                    handle_text("\0");
                }
                break;
            default:
                break;
        }
    } else {
        DEBUG("Keysym: %u '%c'", e->keysym.sym, e->keysym.sym);
        switch(e->keysym.sym) {
            case SDLK_h:
            case SDLK_LEFT:
                selected = map_left[selected];
                break;
            case SDLK_l:
            case SDLK_RIGHT:
                selected = map_right[selected];
                break;
            case SDLK_UP:
            case SDLK_k:
                if (shift) increment_slider(selected, +0.1);
                else selected = map_up[selected];
                break;
            case SDLK_DOWN:
            case SDLK_j:
                if (shift) increment_slider(selected, -0.1);
                else selected = map_down[selected];
                break;
            case SDLK_ESCAPE:
                selected = 0;
                break;
            case SDLK_DELETE:
            case SDLK_d:
                for(int i=0; i<config.ui.n_patterns; i++) {
                    if(map_selection[i] == selected) {
                        deck[map_deck[i]].unload_pattern( map_pattern[i]);
                        break;
                    }
                }
                break;
            case SDLK_BACKQUOTE:
                set_slider_to(selected, 0);
                break;
            case SDLK_1:
                set_slider_to(selected, 0.1);
                break;
            case SDLK_2:
                set_slider_to(selected, 0.2);
                break;
            case SDLK_3:
                set_slider_to(selected, 0.3);
                break;
            case SDLK_4:
                if(shift) {
                    selected = map_end[selected];
                } else {
                    set_slider_to(selected, 0.4);
                }
                break;
            case SDLK_5:
                set_slider_to(selected, 0.5);
                break;
            case SDLK_6:
                if(shift) {
                    selected = map_home[selected];
                } else {
                    set_slider_to(selected, 0.6);
                }
                break;
            case SDLK_7:
                set_slider_to(selected, 0.7);
                break;
            case SDLK_8:
                set_slider_to(selected, 0.8);
                break;
            case SDLK_9:
                set_slider_to(selected, 0.9);
                break;
            case SDLK_0:
                set_slider_to(selected, 1);
                break;
            case SDLK_SEMICOLON: if(!shift) break;
                for(int i=0; i<config.ui.n_patterns; i++) {
                    if(map_selection[i] == selected) {
                        pat_entry = true;
                        pat_entry_text[0] = '\0';
                        SDL_StartTextInput();
                        glBindFramebuffer(GL_FRAMEBUFFER, pat_entry_fb);
                        render_textbox(pat_entry_text, config.ui.pat_entry_width, config.ui.pat_entry_height);
                        glBindFramebuffer(GL_FRAMEBUFFER, 0);
                    }
                }
                break;
            case SDLK_RETURN:
                for(int i=0; i<config.ui.n_patterns; i++) {
                    if(map_selection[i] == selected) {
                        if(i < 4) {
                            left_deck_selector = 0;
                        } else if(i < 8) {
                            right_deck_selector = 1;
                        } else if(i < 12) {
                            left_deck_selector = 2;
                        } else if(i < 16) {
                            right_deck_selector = 3;
                        }
                    }
                }
                break;
            case SDLK_LEFTBRACKET:
                if(left_deck_selector == 0) {
                    left_deck_selector = 2;
                } else {
                    left_deck_selector = 0;
                }
                break;
            case SDLK_RIGHTBRACKET:
                if(right_deck_selector == 1) {
                    right_deck_selector = 3;
                } else {
                    right_deck_selector = 1;
                }
                break;
            case SDLK_SPACE:
                selected = map_space[selected];
                break;
            case SDLK_TAB:
                if(shift) {
                    selected = map_stab[selected];
                } else {
                    selected = map_tab[selected];
                }
                break;
            case SDLK_HOME:
                selected = map_home[selected];
                break;
            case SDLK_END:
                selected = map_end[selected];
                break;
            case SDLK_r:
                if (shift) {
                    midi_refresh();
                    output_refresh();
                }
                break;
            case SDLK_q:
                switch(strip_indicator) {
                    case STRIPS_NONE:
                        strip_indicator = STRIPS_SOLID;
                        break;
                    case STRIPS_SOLID:
                        strip_indicator = STRIPS_COLORED;
                        break;
                    case STRIPS_COLORED:
                    default:
                        strip_indicator = STRIPS_NONE;
                        break;
                }
                break;
            default:
                break;
        }
    }
}

static void blit(float x, float y, float w, float h) {
    bind_vao_fill_vbo(x, y, w, h);
    glDrawArrays(GL_POINTS, 0, 1);
}

static void ui_render(bool select) {
    GLint location;
    // Render strip indicators
    switch(strip_indicator) {
        case STRIPS_SOLID:
        case STRIPS_COLORED:
            glBindFramebuffer(GL_FRAMEBUFFER, strip_fb);
            glUseProgram(strip_shader);
            glProgramUniform2f(strip_shader, 0, 1., 1.);//config.pattern.master_width, config.pattern.master_height);

            location = glGetUniformLocation(strip_shader, "iPreview");
            glProgramUniform1i(strip_shader, location, 0);
            location = glGetUniformLocation(strip_shader, "iIndicator");
            glProgramUniform1i(strip_shader,location, strip_indicator);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, crossfader.tex_output);

            glClear(GL_COLOR_BUFFER_BIT);
            if(!strip_vbo || !strip_vbo) {
                glGenBuffers(1,&strip_vbo);
                glBindBuffer(GL_ARRAY_BUFFER, strip_vbo);
//                glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 4, NULL, GL_STATIC_DRAW);
                auto vvec = std::vector<GLfloat>{};
                auto vertex2d = [&vvec](auto x, auto y) { vvec.push_back((x + 1)/2); vvec.push_back((y+1)/2);};
                for(auto d = output_device_head; d ; d = d->next) {
                    bool first = true;
                    double x;
                    double y;
                    for(auto v = d->vertex_head; v ; v = v->next) {
                        if(!first) {
                            double dx = v->x - x;
                            double dy = v->y - y;
                            double dl = hypot(dx, dy);
                            dx = config.ui.strip_thickness * dx / dl;
                            dy = config.ui.strip_thickness * dy / dl;
                            vertex2d(x + dy, y - dx);
                            vertex2d(v->x + dy, v->y - dx);
                            vertex2d(x - dy, y + dx);
                            vertex2d(v->x + dy, v->y - dx);
                            vertex2d(v->x - dy, v->y + dx);
                            vertex2d(x - dy, y + dx);
                        } else {
                            first = false;
                        }
                        x = v->x;
                        y = v->y;
                    }
                }
                glBufferData(GL_ARRAY_BUFFER, vvec.size() * sizeof(vvec[0]), &vvec[0], GL_STATIC_DRAW);
                strip_vbo_size = vvec.size();
                glGenVertexArrays(1,&strip_vao);
                glBindVertexArray(strip_vao);
                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*)0);
                glEnableVertexAttribArray(0);
            }

            glBindBuffer(GL_ARRAY_BUFFER, strip_vbo);
            glBindVertexArray(strip_vao);
            glDrawArrays(GL_TRIANGLES, 0, strip_vbo_size / 2);
            break;
        default:
        case STRIPS_NONE:
            break;
    }

    // Render the patterns
    glBindFramebuffer(GL_FRAMEBUFFER, pat_fb);

    int pw = config.ui.pattern_width;
    int ph = config.ui.pattern_height;
    glUseProgram(pat_shader);
    location = glGetUniformLocation(pat_shader, "iSelection");
    glUniform1i(location, select);
    location = glGetUniformLocation(pat_shader, "iPreview");
    glUniform1i(location, 0);
    location = glGetUniformLocation(pat_shader, "iName");
    glUniform1i(location, 1);
    GLint pattern_index = glGetUniformLocation(pat_shader, "iPatternIndex");
    GLint pattern_intensity = glGetUniformLocation(pat_shader, "iIntensity");
    GLint name_resolution = glGetUniformLocation(pat_shader, "iNameResolution");
    glProgramUniform2f(pat_shader, 0, pw, ph);

    glViewport(0, 0, pw, ph);
    {
        bool first = true;
        for(int i = 0; i < config.ui.n_patterns; i++) {
            auto &p = deck[map_deck[i]].patterns[map_pattern[i]];
            if(p != NULL) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, p->tex_output);
                glActiveTexture(GL_TEXTURE1);
                SDL_GL_BindTexture(pattern_name_textures[i], NULL, NULL);
                glUniform1i(pattern_index, i);
                glUniform1f(pattern_intensity, p->intensity);
                glUniform2f(name_resolution, pattern_name_sizes[i].first, pattern_name_sizes[i].second);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pattern_textures[i], 0);
                glClear(GL_COLOR_BUFFER_BIT);
                if(first) {
                    fill(pw, ph);
                    first = false;
                }else{
                    glDrawArrays(GL_POINTS,0,1);
                }
            }
        }
    }
    // Render the crossfader
    glBindFramebuffer(GL_FRAMEBUFFER, crossfader_fb);

    int cw = config.ui.crossfader_width;
    int ch = config.ui.crossfader_height;
    glUseProgram(crossfader_shader);
    location = glGetUniformLocation(crossfader_shader, "iSelection");
    glUniform1i(location, select);
    location = glGetUniformLocation(crossfader_shader, "iPreview");
    glUniform1i(location, 0);
    location = glGetUniformLocation(crossfader_shader, "iStrips");
    glUniform1i(location, 1);
    location = glGetUniformLocation(crossfader_shader, "iIntensity");
    glUniform1f(location, crossfader.position);
    location = glGetUniformLocation(crossfader_shader, "iIndicator");
    glUniform1i(location, strip_indicator);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, crossfader.tex_output);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, strip_texture);

    glProgramUniform2f(crossfader_shader, 0, cw, ch);
    glViewport(0, 0, cw, ch);
    glClear(GL_COLOR_BUFFER_BIT);
    fill(cw, ch);

    glDisable(GL_BLEND);
    int sw = 0;
    int sh = 0;
    int vw = 0;
    int vh = 0;
    if(!select) {
        analyze_render(tex_spectrum_data, tex_waveform_data, tex_waveform_beats_data);
        // Render the spectrum
        glBindFramebuffer(GL_FRAMEBUFFER, spectrum_fb);
        sw = config.ui.spectrum_width;
        sh = config.ui.spectrum_height;
        glUseProgram(spectrum_shader);
        location = glGetUniformLocation(spectrum_shader, "iBins");
        glUniform1i(location, config.audio.spectrum_bins);
        location = glGetUniformLocation(spectrum_shader, "iSpectrum");
        glUniform1i(location, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_1D, tex_spectrum_data);

        glProgramUniform2f(spectrum_shader, 0, sw, sh);
        glViewport(0, 0, sw, sh);
        glClear(GL_COLOR_BUFFER_BIT);
        fill(sw, sh);

        // Render the waveform
        glBindFramebuffer(GL_FRAMEBUFFER, waveform_fb);

        vw = config.ui.waveform_width;
        vh = config.ui.waveform_height;
        glUseProgram(waveform_shader);
        glProgramUniform2f(waveform_shader, 0, vw, vh);
        location = glGetUniformLocation(waveform_shader, "iLength");
        glUniform1i(location, config.audio.waveform_length);
        location = glGetUniformLocation(waveform_shader, "iWaveform");
        glUniform1i(location, 0);
        location = glGetUniformLocation(waveform_shader, "iBeats");
        glUniform1i(location, 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_1D, tex_waveform_data);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_1D, tex_waveform_beats_data);

        glViewport(0, 0, vw, vh);
        glClear(GL_COLOR_BUFFER_BIT);
        fill(vw, vh);
    }

    // Render to screen (or select fb)
    if(select) {
        glBindFramebuffer(GL_FRAMEBUFFER, select_fb);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    glViewport(0, 0, ww, wh);

    GL_CHECK_ERROR();
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(main_shader);
    glProgramUniform2f(main_shader, 0, ww, wh);

    glUniform2f(3, ww, wh);
    location = glGetUniformLocation(main_shader, "iSelection");
    glUniform1i(location, select);
    location = glGetUniformLocation(main_shader, "iSelected");
    glUniform1i(location, selected);
    location = glGetUniformLocation(main_shader, "iLeftDeckSelector");
    glUniform1i(location, left_deck_selector);
    location = glGetUniformLocation(main_shader, "iRightDeckSelector");
    glUniform1i(location, right_deck_selector);
    GL_CHECK_ERROR();

    fill(ww, wh);
    GL_CHECK_ERROR();

    // Blit UI elements on top
    glUseProgram(blit_shader);
    glProgramUniform2f(blit_shader, 0, ww, wh);
    glActiveTexture(GL_TEXTURE0);
    location = glGetUniformLocation(blit_shader, "iTexture");
    glUniform1i(location, 0);

    glEnable(GL_BLEND);
    for(int i = 0; i < config.ui.n_patterns; i++) {
        if(deck[map_deck[i]].patterns[map_pattern[i]]) {
            glBindTexture(GL_TEXTURE_2D, pattern_textures[i]);
            blit(map_x[i],map_y[i], pw,ph);
            GL_CHECK_ERROR();
        }
    }
    glBindTexture(GL_TEXTURE_2D, crossfader_texture);
    GL_CHECK_ERROR();
    blit(config.ui.crossfader_x, config.ui.crossfader_y, cw, ch);
    GL_CHECK_ERROR();

    if(!select) {
        glBindTexture(GL_TEXTURE_2D, spectrum_texture);
        blit(config.ui.spectrum_x, config.ui.spectrum_y, sw, sh);
        GL_CHECK_ERROR();

        glBindTexture(GL_TEXTURE_2D, waveform_texture);
        blit(config.ui.waveform_x, config.ui.waveform_y, vw, vh);
        GL_CHECK_ERROR();

        if(pat_entry) {
            for(int i = 0; i < config.ui.n_patterns; i++) {
                if(map_selection[i] == selected) {
                    glBindTexture(GL_TEXTURE_2D, pat_entry_texture);
                    blit(map_pe_x[i], map_pe_y[i], config.ui.pat_entry_width, config.ui.pat_entry_height);
                    GL_CHECK_ERROR();
                    break;
                }
            }
        }
    }
    glDisable(GL_BLEND);
    GL_CHECK_ERROR();
}

struct rgba {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

static struct rgba test_hit(int x, int y) {
    struct rgba data;

    glBindFramebuffer(GL_FRAMEBUFFER, select_fb);
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &data);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return data;
}

static void handle_mouse_move() {
    switch(ma) {
        case MOUSE_NONE:
            break;
        case MOUSE_DRAG_INTENSITY: {
            if(auto &p = deck[map_deck[mp]].patterns[map_pattern[mp]]) {
                p->intensity = mci + (mx - mcx) * config.ui.intensity_gain_x + (my - mcy) * config.ui.intensity_gain_y;
                if(p->intensity > 1) p->intensity = 1;
                if(p->intensity < 0) p->intensity = 0;
            }
            break;
        }
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

static void handle_text(const char * text) {
    if(pat_entry) {
        if(strlen(pat_entry_text) + strlen(text) < sizeof(pat_entry_text)) {
            strcat(pat_entry_text, text);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, pat_entry_fb);
        render_textbox(pat_entry_text, config.ui.pat_entry_width, config.ui.pat_entry_height);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
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
                if(auto &p = deck[map_deck[hit.g]].patterns[map_pattern[hit.g]]) {
                    ma = MOUSE_DRAG_INTENSITY;
                    mp = hit.g;
                    mcx = mx;
                    mcy = my;
                    mci = p->intensity;
                }
            }
            break;
        case HIT_CROSSFADER:
            selected = crossfader_selection_top;
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

        quit = false;
        while(!quit) {
            ui_render(true);

            while(SDL_PollEvent(&e) != 0) {
                if (midi_command_event != (Uint32) -1 && 
                    e.type == midi_command_event) {
                    midi_event * me = static_cast<midi_event*>(e.user.data1);
                    switch (me->type) {
                    case MIDI_EVENT_SLIDER: {
                        set_slider_to(me->slider.index, me->slider.value);
                        break;
                    }
                    case MIDI_EVENT_KEY: {
                        SDL_KeyboardEvent fakekeyev;
                        memset(&fakekeyev, 0, sizeof fakekeyev);
                        fakekeyev.type = SDL_KEYDOWN;
                        fakekeyev.state = SDL_PRESSED;
                        fakekeyev.keysym.sym = me->key.keycode[0];
                        handle_key(&fakekeyev);
                        break;
                        }
                    }
                    free(e.user.data1);
                    free(e.user.data2);
                    continue;
                }
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
                    case SDL_TEXTINPUT:
                        handle_text(e.text.text);
                        break;
                }
            }
            for(auto & d : deck)
                d.render();
            crossfader_render(&crossfader, deck[left_deck_selector].tex_output, deck[right_deck_selector].tex_output);
            render_readback(&render);
            ui_render(false);
            SDL_GL_SwapWindow(window);

            double cur_t = SDL_GetTicks();
            double dt = cur_t - l_t;
            if(dt > 0) current_time += dt / 1000;
            l_t = cur_t;
        }
}


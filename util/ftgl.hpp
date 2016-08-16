_Pragma("once")

#include "util/common.h"
#include "util/glsl.h"
#include <utf8.h>
#include <string>
#include <map>
#include <vector>
#include <locale>
#include <codecvt>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <utility>
#include <iterator>
#include <memory>
#include <functional>
#include <cstdio>
#include <cstring>
#define FTGL_INCLUDE_NONE

#include "freetype-gl/freetype-gl.h"
#include "vertex-buffer.h"
#include "shader.h"
#include "mat4.h"


struct tex_atlas {
    texture_atlas_t *m_d{};
    tex_atlas(int w, int h, int d);
    constexpr tex_atlas(tex_atlas &&other) noexcept = default;
    tex_atlas&operator=(tex_atlas &&other) noexcept = default;
   ~tex_atlas();
    void create(int w, int h, int d);
    void destroy();
    operator texture_atlas_t *() const { return m_d;}
    texture_atlas_t *operator ->() const { return m_d;}
    texture_atlas_t &operator *() const { return *m_d;}
    operator bool() const { return !!m_d;}
    bool operator !() const { return !m_d;}
    friend bool operator ==(const tex_atlas &lhs, const tex_atlas &rhs) { return lhs.m_d == rhs.m_d; }
};
struct tex_font {
    struct glyph_info {
        texture_glyph_t         *m_glyph{};
        std::map<char32_t,float> m_kern{};
    };
    std::map<char32_t , glyph_info> m_glyphs{};
    texture_font_t *m_d{};
    std::shared_ptr<tex_atlas> m_atlas{};
    tex_font() = default;
    tex_font(tex_font &&) noexcept = default;
    tex_font&operator=(tex_font &&) noexcept = default;
    tex_font(std::shared_ptr<tex_atlas> atlas, int pt_size, const char *font);
   ~tex_font();
    void destroy();
    void create(std::shared_ptr<tex_atlas> atlas, int pt_size, const char *font);
    operator texture_font_t *() const { return m_d;}
    texture_font_t *operator ->() const { return m_d;}
    texture_font_t &operator *() const { return *m_d;}
    operator bool() const { return !!m_d;}
    bool operator !() const { return !m_d;}
    friend bool operator ==(const tex_font &lhs, const tex_font &rhs)
    {
        return lhs.m_d == rhs.m_d && lhs.m_atlas == rhs.m_atlas;
    }
    texture_glyph_t *get_glyph(const char *codepoint);
    texture_glyph_t *get_glyph(char32_t ucodepoint);
    float get_kerning(const char *prev, const char *curr);
    float get_kerning(char32_t uprev, char32_t ucurr);
};

struct ftgl_renderer {
    struct vertex {
        vec2 position;
        vec2 texcoord;
        vec4 fg;
        friend std::ostream &operator<<(std::ostream &ost, const vertex &vtx)
        {
            return (ost << " < vertex @ " << static_cast<const void*>(&vtx)
                        << " : (x,y) == ( " << vtx.position.x << ", " << vtx.position.y << " ),"
                        << "( x, y ) == ( " << vtx.texcoord.s << ", " << vtx.texcoord.t << " ) ,"
                        << "(r,g,b,a,) == ( "<< vtx.fg.r << "< " << vtx.fg.g << ", " << vtx.fg.b << ", " << vtx.fg.a << " )"
                        << " > ");
        }

    };
    std::shared_ptr<tex_atlas>  m_atlas {};
    std::shared_ptr<tex_font>   m_font  {};
    GLuint                      m_shader{0};
    GLuint                      m_vao   {0};
    GLuint                      m_vbo   {0};
    size_t                      m_vbo_size{0};
    std::vector<vertex>         m_vbo_data{};
    bool                        m_vbo_dirty{false};
    vec2                        m_pen   {};
    vec4                        m_color {{1.f,1.f,1.f,1.f}};
    float                       m_scale {1.f};

    ftgl_renderer() = default;
    ftgl_renderer(ftgl_renderer &&o) noexcept : ftgl_renderer() { swap(o); }
    ftgl_renderer&operator=(ftgl_renderer &&o) noexcept { swap(o);return *this; };
    void swap(ftgl_renderer &o) noexcept;
    friend void swap(ftgl_renderer &lhs, ftgl_renderer &rhs) noexcept { lhs.swap(rhs); }
    ftgl_renderer&operator=(const ftgl_renderer &o);
    ftgl_renderer(const ftgl_renderer &o);
    ftgl_renderer(int w, int h, int pt_size, const char *filename);
    void getShader();
    void getBuffer();
   ~ftgl_renderer();
    void add_glyph( const char *curr, const char *prev=nullptr);
    void add_glyph( char32_t curr, char32_t prev = 0);
    void prepare();
    void set_color(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
    vec4 get_color() const;

    void set_scale(float _scale);
    void set_dirty();
    bool get_dirty() const;
    float get_scale() const;
    void clear();
    void render(int global_w, int global_h);
    void print(GLfloat x, GLfloat y, const std::string &str);
    float size() const { return (*m_font)->size * get_scale();}
    float height() const { return (*m_font)->height * get_scale();}
    float linegap() const { return (*m_font)->linegap * get_scale();}
    float ascender() const { return (*m_font)->ascender * get_scale();}
    float descender() const { return (*m_font)->descender * get_scale();}

};

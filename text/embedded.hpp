_Pragma("once")

#include "util/common.h"
#include "util/glsl.h"
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
#include <freetype-gl/vec234.h>
#include "DejaVuSans.h"
using namespace ftgl;

struct embedded_font {
    struct glyph_info {
        texture_glyph_t         *m_glyph{};
        std::map<char,float> m_kern{};
    };
    std::map<char, glyph_info> m_glyphs{};
    texture_font_t *m_d{};
    std::shared_ptr<GLuint> m_tex{};
    embedded_font() = default;
    embedded_font(embedded_font &&) noexcept = default;
    embedded_font&operator=(embedded_font &&) noexcept = default;
    embedded_font(std::shared_ptr<GLuint> tex, int pt_size, const std::string &fontname);
   ~embedded_font();
    void destroy();
    void create(std::shared_ptr<GLuint> tex, int pt_size, const std::string &fontname);
    operator texture_font_t *() const { return m_d;}
    texture_font_t *operator ->() const { return m_d;}
    texture_font_t &operator *() const { return *m_d;}
    operator bool() const { return !!m_d;}
    bool operator !() const { return !m_d;}
    friend bool operator ==(const embedded_font &lhs, const embedded_font &rhs)
    {
        return lhs.m_d == rhs.m_d && lhs.m_tex== rhs.m_tex;
    }
    texture_glyph_t *get_glyph(const char *codepoint);
    texture_glyph_t *get_glyph(char ucodepoint);
    float get_kerning(const char *prev, const char *curr);
    float get_kerning(char uprev, char ucurr);
};

struct embedded_renderer {
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
    std::shared_ptr<GLuint>   m_tex{};
    std::shared_ptr<embedded_font>   m_font  {};
    std::map< std::string
      , std::shared_ptr<embedded_font>
        > m_fonts{};
    GLuint                      m_shader{0};
    GLuint                      m_vao   {0};
    GLuint                      m_vbo   {0};
    size_t                      m_vbo_size{0};
    std::vector<vertex>         m_vbo_data{};
    bool                        m_vbo_dirty{false};
    vec2                        m_pen   {};
    vec4                        m_color {{1.f,1.f,1.f,1.f}};
    float                       m_scale {1.f};
    int                         m_pt_size{32};

    embedded_renderer() = default;
    embedded_renderer(embedded_renderer &&o) noexcept : embedded_renderer() { swap(o); }
    embedded_renderer&operator=(embedded_renderer &&o) noexcept { swap(o);return *this; };
    void swap(embedded_renderer &o) noexcept;
    friend void swap(embedded_renderer &lhs, embedded_renderer &rhs) noexcept { lhs.swap(rhs); }
    embedded_renderer&operator=(const embedded_renderer &o);
    embedded_renderer(const embedded_renderer &o);
    embedded_renderer(int w, int h, int pt_size, const std::string &filename);
    void open_font(const std::string &filename);
    bool active_font(const std::string &filename);
    void getShader();
    void getBuffer();
   ~embedded_renderer();
    void add_glyph( const char *curr, const char *prev=nullptr);
    void add_glyph( char curr, char prev = 0);
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

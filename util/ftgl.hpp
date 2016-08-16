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

#include "freetype-gl.h"
#include "vertex-buffer.h"
#include "shader.h"
#include "mat4.h"


struct tex_atlas {
    texture_atlas_t *m_d{};
    tex_atlas(int w, int h, int d)
    : m_d(texture_atlas_new(w,h,d)) {
        m_d->id = make_texture(GL_R32F,w, h);
    }
    constexpr tex_atlas(tex_atlas &&other) noexcept = default;
    tex_atlas&operator=(tex_atlas &&other) noexcept = default;
    ~tex_atlas()
    {
            destroy();
    }
    void create(int w, int h, int d)
    {
            destroy();
            m_d = texture_atlas_new(w,h,d);
            m_d->id = make_texture(w, h);
    }
    void destroy()
    {
        if(m_d) {
            glDeleteTextures(1, &m_d->id);
            texture_atlas_delete(m_d);
        }
        m_d = nullptr;
    }
    operator texture_atlas_t *() const { return m_d;}
    texture_atlas_t *operator ->() const { return m_d;}
    texture_atlas_t &operator *() const { return *m_d;}
    operator bool() const { return !!m_d;}
    bool operator !() const { return !m_d;}
    friend bool operator ==(const tex_atlas &lhs, const tex_atlas &rhs)
    {
        return lhs.m_d == rhs.m_d;
    }
};
inline std::string utf32_to_utf8(char32_t ucodepoint)
{
    std::string codepoints{};
    utf8::utf32to8(&ucodepoint,(&ucodepoint)+1,std::back_inserter(codepoints));
    return codepoints;
}
inline char32_t utf8_to_utf32(const std::string &str)
{
    std::vector<uint32_t> codepoints{};
    utf8::utf8to32(str.begin(),str.end(),std::back_inserter(codepoints));
    return codepoints[0];
}
inline std::vector<uint32_t> utf8_to_utf32_vec(const std::string &str)
{
    std::vector<uint32_t> codepoints{};
    utf8::utf8to32(str.begin(),str.end(),std::back_inserter(codepoints));
    return codepoints;
}
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
    tex_font(std::shared_ptr<tex_atlas> atlas, int pt_size, const char *font)
    {
        create(atlas,pt_size,font);
    }
   ~tex_font()
    {
        destroy();
    }
    void destroy()
    {
        if(m_d) {
            texture_font_delete(m_d);
            m_d = nullptr;
        }
        m_atlas.reset();
    }
    void create(std::shared_ptr<tex_atlas> atlas, int pt_size, const char *font)
    {
        destroy();
        m_atlas = atlas;
        m_d = texture_font_new_from_file(*m_atlas, pt_size, font);
        for(auto i = char{32}; i < char{127}; ++i){
            const char _str[] = { i, 0 };
            auto glyph = get_glyph(_str);
            (void)sizeof(glyph);
            for(auto j = char{32}; j < char{127}; ++j) {
            const char __str[] = { i, 0 };
                get_kerning(__str, _str);
            }
        }
        glBindTexture(GL_TEXTURE_2D, (*m_atlas)->id);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (*m_atlas)->width, (*m_atlas)->height, GL_RED, GL_UNSIGNED_BYTE, (*m_atlas)->data);
    }
    operator texture_font_t *() const { return m_d;}
    texture_font_t *operator ->() const { return m_d;}
    texture_font_t &operator *() const { return *m_d;}
    operator bool() const { return !!m_d;}
    bool operator !() const { return !m_d;}
    friend bool operator ==(const tex_font &lhs, const tex_font &rhs)
    {
        return lhs.m_d == rhs.m_d && lhs.m_atlas == rhs.m_atlas;
    }
    texture_glyph_t *get_glyph(const char *codepoint)
    {
        auto ucodepoint = utf8_to_utf32(std::string{codepoint});
        return get_glyph(ucodepoint);
    }
    texture_glyph_t *get_glyph(char32_t ucodepoint)
    {
        auto it = m_glyphs.find(ucodepoint);
        if(it == m_glyphs.end()) {
            auto codepoints = utf32_to_utf8(ucodepoint);
            auto new_glyph = texture_font_get_glyph(m_d, codepoints.c_str());
            std::tie(it,std::ignore) = m_glyphs.insert(std::make_pair(ucodepoint,glyph_info{new_glyph}));
        }
        return it->second.m_glyph;
    }
    float get_kerning(const char *prev, const char *curr)
    {
        auto uprev = utf8_to_utf32(std::string{prev});
        auto ucurr = utf8_to_utf32(std::string{curr});
        return get_kerning(uprev,ucurr);
    }
    float get_kerning(char32_t uprev, char32_t ucurr)
    {
        auto it = m_glyphs.find(uprev);
        if(it == m_glyphs.end()) {
            auto codepoint = utf32_to_utf8(ucurr);
            auto new_glyph = texture_font_get_glyph(m_d, codepoint.c_str());
            std::tie(it,std::ignore) = m_glyphs.insert(std::make_pair(uprev,glyph_info{new_glyph}));
        }
        auto kit = it->second.m_kern.find(uprev);
        if(kit == it->second.m_kern.end()) {
            auto prev = utf32_to_utf8(uprev);
            auto kern = texture_glyph_get_kerning(it->second.m_glyph, prev.c_str());
            it->second.m_kern[uprev] = kern;
        }
        return it->second.m_kern[uprev];
    }
};

struct ftgl_renderer {
    struct vertex {
        GLfloat x; GLfloat y;
        GLfloat s; GLfloat t;
        GLfloat r; GLfloat g; GLfloat b; GLfloat a;
        friend std::ostream &operator<<(std::ostream &ost, const vertex &vtx)
        {
            return (ost << " < vertex @ " << static_cast<const void*>(&vtx)
                        << " : (x,y) == ( " << vtx.x << ", " << vtx.y << " ), ( x, y ) == ( " << vtx.s << ", " << vtx.t << " ) , (r,g,b,a,) == ( "
                        << vtx.r << "< " << vtx.g << ", " << vtx.b << ", " << vtx.a << " ) > ");
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
//    size_t                      m_vbo_data_offset{};
//    vertex                     *m_vbo_mapped{};
//    size_t                      m_vbo_mapped_size{};
//    size_t                      m_vbo_active_begin{};
//    size_t                      m_vbo_active_end{};
    vec2                        m_pen   {};
    vec4                        m_color {{1.f,1.f,1.f,1.f}};
    float                       m_scale {1.f};

    ftgl_renderer() = default;
    ftgl_renderer(ftgl_renderer &&o) noexcept
    : ftgl_renderer()
    {
        using std::swap;
        swap(*this,o);
    }
    ftgl_renderer&operator=(ftgl_renderer &&o) noexcept
    {
        using std::swap;
        swap(*this, o);
        return *this;
    };
    void swap(ftgl_renderer &o) noexcept
    {
        using std::swap;
        swap(m_atlas,o.m_atlas);
        swap(m_font,o.m_font);
        swap(m_vao,o.m_vao);
        swap(m_vbo,o.m_vbo);
        swap(m_vbo_size,o.m_vbo_size);
        swap(m_vbo_data,o.m_vbo_data);
        swap(m_vbo_dirty,o.m_vbo_dirty);
        swap(m_pen,o.m_pen);
        swap(m_color,o.m_color);
        swap(m_scale,o.m_scale);
    }
    friend void swap(ftgl_renderer &lhs, ftgl_renderer &rhs) noexcept
    {
        lhs.swap(rhs);
    }
    ftgl_renderer&operator=(const ftgl_renderer &o)
    {
        m_atlas = o.m_atlas;
        m_font  = o.m_font;
        m_scale = o.m_scale;
        m_color = o.m_color;
        getShader();
        getBuffer();
        return *this;
    }
    ftgl_renderer(const ftgl_renderer &o)
    : m_atlas(o.m_atlas)
    , m_font(o.m_font)
    , m_color(o.m_color)
    , m_scale(o.m_scale)
    {
        getShader();
        getBuffer();
    }
    ftgl_renderer(int w, int h, int pt_size, const char *filename)
    : m_atlas(std::make_shared<tex_atlas>(w, h, 1))
    , m_font (std::make_shared<tex_font>(m_atlas, pt_size, filename))
    {
        getShader();
        getBuffer();
    }
    void getShader()
    {
        if(!m_shader) {
            m_shader = load_shader_noheader("#sdf.v.glsl","#sdf.f.glsl");
            glProgramUniform1i(m_shader, 1, 0);
        }
    }
    void getBuffer()
    {
        if(m_vao && m_vbo)
            return;
        if(!m_vao)
            glGenVertexArrays(1, &m_vao);
        if(!m_vbo) {
            glGenBuffers(1, &m_vbo);
        }
        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glVertexAttribPointer(0, 2, GL_FLOAT,GL_FALSE,8*sizeof(GLfloat),(void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,8*sizeof(GLfloat),(void*)(2*sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE,8*sizeof(GLfloat),(void*)(4*sizeof(GLfloat)));
        glEnableVertexAttribArray(2);
    }
   ~ftgl_renderer()
    {
        if(m_vao)
            glDeleteVertexArrays(1,&m_vao);
        if(m_vbo)
            glDeleteBuffers(1, &m_vbo);
        if(m_shader) {
            glDeleteProgram(m_shader);
        }
        m_shader = 0;
        m_vao = 0;
        m_vbo = 0;
    }
    void add_glyph( const char *curr, const char *prev=nullptr)
    {
        add_glyph(utf8_to_utf32(std::string{curr}),utf8_to_utf32(std::string{prev}));
    }
    void add_glyph( char32_t curr, char32_t prev = 0)
    {
        auto glyph = m_font->get_glyph(curr);
        if(prev && curr)
            m_pen.x += m_font->get_kerning(prev,curr) ;//* m_scale;
        auto r = m_color.r;
        auto g = m_color.g;
        auto b = m_color.b;
        auto a = m_color.a;
        auto x0  = m_pen.x + (glyph->offset_x )* m_scale;
        auto y0  = m_pen.y + (glyph->offset_y - (*m_font)->ascender)* m_scale;
        auto x1  = x0 + glyph->width * m_scale;
        auto y1  = y0 - glyph->height* m_scale;
        auto s0 = glyph->s0;
        auto t0 = glyph->t0;
        auto s1 = glyph->s1;
        auto t1 = glyph->t1;

        for(auto && v : { vertex{ x0,y0,  s0,t0,  r,g,b,a },
                          vertex{ x0,y1,  s0,t1,  r,g,b,a },
                          vertex{ x1,y0,  s1,t0,  r,g,b,a },
                          vertex{ x0,y1,  s0,t1,  r,g,b,a },
                          vertex{ x1,y1,  s1,t1,  r,g,b,a },
                          vertex{ x1,y0,  s1,t0,  r,g,b,a } }) {
            m_vbo_data.push_back(v);
        }
        m_vbo_dirty = true;
        m_pen.x += glyph->advance_x * m_scale;
        m_pen.y += glyph->advance_y * m_scale;
    }

    void prepare()
    {
        if(m_vbo_dirty) {
            glBindVertexArray(m_vao);
            glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
            glVertexAttribPointer(0, 2, GL_FLOAT,GL_FALSE,8*sizeof(GLfloat),(void*)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,8*sizeof(GLfloat),(void*)(2*sizeof(GLfloat)));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE,8*sizeof(GLfloat),(void*)(4*sizeof(GLfloat)));
            glEnableVertexAttribArray(2);

            m_vbo_size = m_vbo_data.size() * sizeof(m_vbo_data[0]);
            glBufferData(GL_ARRAY_BUFFER,m_vbo_size, m_vbo_data.data(), GL_DYNAMIC_DRAW);
            m_vbo_dirty = false;
        }
    }
    void set_color(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
    {
        m_color = vec4({r,g,b,a});
    }
    void set_scale(float _scale)
    {
        m_scale = _scale;
    }
    void clear()
    {
        if(m_vbo_data.size()) {
            m_vbo_data.clear();
            m_vbo_dirty = true;
        }
    }
    void render(int global_w, int global_h)
    {
        getShader();
        getBuffer();
        prepare();
        if(m_vbo_size) {
            glUseProgram(m_shader);
            glUniform2f(0, global_w, global_h);
            glUniform1i(1, 0);
            glBindVertexArray(m_vao);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, (*m_atlas)->id);
            glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
            glDrawArrays(GL_TRIANGLES, 0, m_vbo_size / sizeof(vertex));
            CHECK_GL();
        }
    }
    void print(GLfloat x, GLfloat y, const std::string &str)
    {
        auto u32str = utf8_to_utf32_vec(str);
        m_pen.x = x;
        m_pen.y = y;
        char32_t prev = 0;
        for(auto && curr : u32str) {
            add_glyph(curr,prev);
            prev = curr;
        }
    }
};

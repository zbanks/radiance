#define EMBEDDED_IMPLEMENTATION
#include "text/embedded.hpp"

struct texture_deleter {
    void operator() (GLuint *t) const { glDeleteTextures(1, t); *t = 0; }
};

embedded_font::embedded_font(std::shared_ptr<GLuint> tex, int pt_size, const std::string &fontname)
{
    create(tex,pt_size,fontname);
}
embedded_font::~embedded_font()
{
    destroy();
}
void embedded_font::destroy()
{
    if(m_d) {
//        texture_font_delete(m_d);
        m_d = nullptr;
    }
    m_tex.reset();
}
void embedded_font::create(std::shared_ptr<GLuint> tex, int pt_size, const std::string &fontname)
{
    destroy();
    m_tex = tex;
    m_d = &dejavu_sans;
    if(!m_d)
        return;
    for(auto & glyph : m_d->glyphs) {
        auto its = m_glyphs.insert(std::make_pair(glyph.codepoint, glyph_info{&glyph}));
        auto it = its.first;
        for(auto &kern : glyph.kerning) {
            it->second.m_kern.insert(std::make_pair(kern.codepoint,kern.kerning));
        }
    }

    m_tex = std::make_shared<GLuint>(make_texture(m_d->tex_width, m_d->tex_height));
    glBindTexture(GL_TEXTURE_2D, (*m_tex));
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_d->tex_width, m_d->tex_height, GL_RED, GL_UNSIGNED_BYTE, m_d->tex_data);
}
texture_glyph_t *embedded_font::get_glyph(const char *codepoint)
{
    return get_glyph(*codepoint);
}
texture_glyph_t *embedded_font::get_glyph(char ucodepoint)
{
    auto it = m_glyphs.find(ucodepoint);
    if(it == m_glyphs.end()) {
        return nullptr;
    }
    return it->second.m_glyph;
}
float embedded_font::get_kerning(const char *prev, const char *curr)
{
    return get_kerning(prev ? *prev : 0,curr ? *curr : 0);
}
float embedded_font::get_kerning(char uprev, char ucurr)
{
    auto it = m_glyphs.find(uprev);
    if(it == m_glyphs.end()) {
        return 0.;
    }
    auto kit = it->second.m_kern.find(uprev);
    if(kit == it->second.m_kern.end()) {
        return 0.;
    }
    return it->second.m_kern[uprev];
}

void embedded_renderer::swap(embedded_renderer &o) noexcept
{
    using std::swap;
    swap(m_tex,o.m_tex);
    swap(m_font,o.m_font);
    swap(m_fonts,o.m_fonts);
    swap(m_vao,o.m_vao);
    swap(m_vbo,o.m_vbo);
    swap(m_vbo_size,o.m_vbo_size);
    swap(m_vbo_data,o.m_vbo_data);
    swap(m_vbo_dirty,o.m_vbo_dirty);
    swap(m_pen,o.m_pen);
    swap(m_color,o.m_color);
    swap(m_scale,o.m_scale);
    swap(m_pt_size,o.m_pt_size);
}
embedded_renderer&embedded_renderer::operator=(const embedded_renderer &o)
{
    m_tex= o.m_tex;
    m_font  = o.m_font;
    m_fonts = o.m_fonts;
    m_scale = o.m_scale;
    m_color = o.m_color;
    m_pt_size = o.m_pt_size;
    getShader();
    getBuffer();
    return *this;
}
embedded_renderer::embedded_renderer(const embedded_renderer &o)
: m_tex(o.m_tex)
, m_font(o.m_font)
, m_fonts(o.m_fonts)
, m_color(o.m_color)
, m_scale(o.m_scale)
, m_pt_size(o.m_pt_size)
{
    getShader();
    getBuffer();
}
embedded_renderer::embedded_renderer(int w, int h, int pt_size, const std::string &fontname)
: m_font{std::make_shared<embedded_font>(m_tex, pt_size, fontname)}
, m_fonts{std::make_pair(fontname,m_font)}
, m_pt_size(pt_size)
{
    m_tex = m_font->m_tex;
    getShader();
    getBuffer();
}
void embedded_renderer::getShader()
{
    if(!m_shader) {
        m_shader = load_program_noheader("#sdf.v.glsl","#sdf.f.glsl");
        glProgramUniform1i(m_shader, 1, 0);
    }
}
void embedded_renderer::getBuffer()
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
embedded_renderer::~embedded_renderer()
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
void embedded_renderer::add_glyph( const char *curr, const char *prev)
{
    add_glyph(*curr, *prev);
}
void embedded_renderer::add_glyph( char curr, char prev )
{
    auto glyph = m_font->get_glyph(curr);
    if(prev && curr)
        m_pen.x += m_font->get_kerning(prev,curr) ;//* m_scale;
    auto x0  = m_pen.x + (glyph->offset_x) * m_scale;
    auto y0  = m_pen.y + (glyph->offset_y) * m_scale;
    auto x1  = x0 + glyph->width * m_scale;
    auto y1  = y0 - glyph->height* m_scale;
    auto s0 = glyph->s0;
    auto t0 = glyph->t0;
    auto s1 = glyph->s1;
    auto t1 = glyph->t1;

    vertex verts[] = { vertex{ x0,y0,  s0,t0,  m_color},
                        vertex{ x0,y1,  s0,t1,  m_color},
                        vertex{ x1,y0,  s1,t0,  m_color},
                        vertex{ x0,y1,  s0,t1,  m_color},
                        vertex{ x1,y1,  s1,t1,  m_color},
                        vertex{ x1,y0,  s1,t0,  m_color}};
    std::copy(std::begin(verts),std::end(verts),std::back_inserter(m_vbo_data));
    m_vbo_dirty = true;
    m_pen.x += glyph->advance_x * m_scale;
    m_pen.y += glyph->advance_y * m_scale;
}

void embedded_renderer::prepare()
{
    if(m_vbo_dirty) {
        m_vbo_size = m_vbo_data.size() * sizeof(m_vbo_data[0]);
        glNamedBufferData(m_vbo,m_vbo_size, m_vbo_data.data(), GL_DYNAMIC_DRAW);
        m_vbo_dirty = false;
    }
}
void embedded_renderer::set_color(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    m_color = vec4({r,g,b,a});
}
vec4 embedded_renderer::get_color() const { return m_color;}
void embedded_renderer::set_scale(float _scale)
{
    m_scale = _scale;
}
float embedded_renderer::get_scale() const { return m_scale;}
void  embedded_renderer::set_dirty() { m_vbo_dirty=true;}
bool embedded_renderer::get_dirty() const { return m_vbo_dirty;}
void embedded_renderer::clear()
{
    if(m_vbo_data.size()) {
        m_vbo_data.clear();
        m_vbo_dirty = true;
    }
}
void embedded_renderer::open_font(const std::string &fontname)
{
    if(m_fonts.find(fontname) != m_fonts.end())
        return;
    if(!m_tex)
        return;
    m_fonts.insert(std::make_pair(fontname, std::make_shared<embedded_font>(m_tex,m_pt_size,fontname)));
}
bool embedded_renderer::active_font(const std::string &fontname)
{
    auto it = m_fonts.find(fontname);
    if(it != m_fonts.end()) {
        m_font = it->second;
        return true;
    }else{
        return false;
    }
}
void embedded_renderer::render(int global_w, int global_h)
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
        glBindTexture(GL_TEXTURE_2D, *m_tex);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glDrawArrays(GL_TRIANGLES, 0, m_vbo_size / sizeof(vertex));
        CHECK_GL();
    }
}
void embedded_renderer::print(GLfloat x, GLfloat y, const std::string &str)
{
    m_pen.x = x;
    m_pen.y = y;
    char prev = 0;
    for(auto && curr : str) {
        add_glyph(curr,prev);
        prev = curr;
    }
}

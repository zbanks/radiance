#include "text/ftgl.hpp"

tex_atlas::tex_atlas(int w, int h, int d)
: m_d(texture_atlas_new(w,h,d)) {
    m_d->id = make_texture(GL_R32F,w, h);
}
tex_atlas::~tex_atlas()
{
        destroy();
}
void tex_atlas::create(int w, int h, int d)
{
        destroy();
        m_d = texture_atlas_new(w,h,d);
        m_d->id = make_texture(w, h);
}
void tex_atlas::destroy()
{
    if(m_d) {
        glDeleteTextures(1, &m_d->id);
        texture_atlas_delete(m_d);
    }
    m_d = nullptr;
}
namespace {
std::string utf32_to_utf8(char32_t ucodepoint)
{
    std::string codepoints{};
    utf8::utf32to8(&ucodepoint,(&ucodepoint)+1,std::back_inserter(codepoints));
    return codepoints;
}
char32_t utf8_to_utf32(const std::string &str)
{
    std::vector<uint32_t> codepoints{};
    utf8::utf8to32(str.begin(),str.end(),std::back_inserter(codepoints));
    return codepoints[0];
}
std::vector<uint32_t> utf8_to_utf32_vec(const std::string &str)
{
    std::vector<uint32_t> codepoints{};
    utf8::utf8to32(str.begin(),str.end(),std::back_inserter(codepoints));
    return codepoints;
}
}
tex_font::tex_font(std::shared_ptr<tex_atlas> atlas, int pt_size, const std::string &fontname)
{
    create(atlas,pt_size,fontname);
}
tex_font::~tex_font()
{
    destroy();
}
void tex_font::destroy()
{
    if(m_d) {
        texture_font_delete(m_d);
        m_d = nullptr;
    }
    m_atlas.reset();
}
void tex_font::create(std::shared_ptr<tex_atlas> atlas, int pt_size, const std::string &fontname)
{
    destroy();
    m_atlas = atlas;
    m_d = texture_font_new_from_file(*m_atlas, pt_size, fontname.c_str());
    if(!m_d) {
        return;
    }
    m_d->rendermode = RENDER_SIGNED_DISTANCE_FIELD;

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
texture_glyph_t *tex_font::get_glyph(const char *codepoint)
{
    auto ucodepoint = utf8_to_utf32(std::string{codepoint});
    return get_glyph(ucodepoint);
}
texture_glyph_t *tex_font::get_glyph(char32_t ucodepoint)
{
    auto it = m_glyphs.find(ucodepoint);
    if(it == m_glyphs.end()) {
        auto codepoints = utf32_to_utf8(ucodepoint);
        auto new_glyph = texture_font_get_glyph(m_d, codepoints.c_str());
        if(new_glyph) {
            std::tie(it,std::ignore) = m_glyphs.insert(std::make_pair(ucodepoint,glyph_info{new_glyph}));
        }else{
            return nullptr;
        }
    }
    return it->second.m_glyph;
}
float tex_font::get_kerning(const char *prev, const char *curr)
{
    auto uprev = utf8_to_utf32(std::string{prev});
    auto ucurr = utf8_to_utf32(std::string{curr});
    return get_kerning(uprev,ucurr);
}
float tex_font::get_kerning(char32_t uprev, char32_t ucurr)
{
    auto it = m_glyphs.find(uprev);
    if(it == m_glyphs.end()) {
        auto codepoint = utf32_to_utf8(ucurr);
        auto new_glyph = texture_font_get_glyph(m_d, codepoint.c_str());
        if(new_glyph) {
            std::tie(it,std::ignore) = m_glyphs.insert(std::make_pair(uprev,glyph_info{new_glyph}));
        }else{
            return 0.f;
        }
    }
    auto kit = it->second.m_kern.find(uprev);
    if(kit == it->second.m_kern.end()) {
        auto prev = utf32_to_utf8(uprev);
        auto kern = texture_glyph_get_kerning(it->second.m_glyph, prev.c_str());
        it->second.m_kern[uprev] = kern;
    }
    return it->second.m_kern[uprev];
}

void ftgl_renderer::swap(ftgl_renderer &o) noexcept
{
    using std::swap;
    swap(m_atlas,o.m_atlas);
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
ftgl_renderer&ftgl_renderer::operator=(const ftgl_renderer &o)
{
    m_atlas = o.m_atlas;
    m_font  = o.m_font;
    m_fonts = o.m_fonts;
    m_scale = o.m_scale;
    m_color = o.m_color;
    m_pt_size = o.m_pt_size;
    getShader();
    getBuffer();
    return *this;
}
ftgl_renderer::ftgl_renderer(const ftgl_renderer &o)
: m_atlas(o.m_atlas)
, m_font(o.m_font)
, m_fonts(o.m_fonts)
, m_color(o.m_color)
, m_scale(o.m_scale)
, m_pt_size(o.m_pt_size)
{
    getShader();
    getBuffer();
}
ftgl_renderer::ftgl_renderer(int w, int h, int pt_size, const std::string &fontname)
: m_atlas(std::make_shared<tex_atlas>(w, h, 1))
, m_font{std::make_shared<tex_font>(m_atlas, pt_size, fontname)}
, m_fonts{std::make_pair(fontname,m_font)}
, m_pt_size(pt_size)
{
    getShader();
    getBuffer();
}
void ftgl_renderer::getShader()
{
    if(!m_shader) {
        m_shader = load_program_noheader("#sdf.v.glsl","#sdf.f.glsl");
        glProgramUniform1i(m_shader, 1, 0);
    }
}
void ftgl_renderer::getBuffer()
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
ftgl_renderer::~ftgl_renderer()
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
void ftgl_renderer::add_glyph( const char *curr, const char *prev)
{
    add_glyph(utf8_to_utf32(std::string{curr}),utf8_to_utf32(std::string{prev}));
}
void ftgl_renderer::add_glyph( char32_t curr, char32_t prev )
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

void ftgl_renderer::prepare()
{
    if(m_vbo_dirty) {
        m_vbo_size = m_vbo_data.size() * sizeof(m_vbo_data[0]);
        glNamedBufferData(m_vbo,m_vbo_size, m_vbo_data.data(), GL_DYNAMIC_DRAW);
        m_vbo_dirty = false;
    }
}
void ftgl_renderer::set_color(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    m_color = vec4({r,g,b,a});
}
vec4 ftgl_renderer::get_color() const { return m_color;}
void ftgl_renderer::set_scale(float _scale)
{
    m_scale = _scale;
}
float ftgl_renderer::get_scale() const { return m_scale;}
void  ftgl_renderer::set_dirty() { m_vbo_dirty=true;}
bool ftgl_renderer::get_dirty() const { return m_vbo_dirty;}
void ftgl_renderer::clear()
{
    if(m_vbo_data.size()) {
        m_vbo_data.clear();
        m_vbo_dirty = true;
    }
}
void ftgl_renderer::open_font(const std::string &fontname)
{
    if(m_fonts.find(fontname) != m_fonts.end())
        return;
    if(!m_atlas)
        return;
    m_fonts.insert(std::make_pair(fontname, std::make_shared<tex_font>(m_atlas,m_pt_size,fontname)));
}
bool ftgl_renderer::active_font(const std::string &fontname)
{
    auto it = m_fonts.find(fontname);
    if(it != m_fonts.end()) {
        m_font = it->second;
        return true;
    }else{
        return false;
    }
}
void ftgl_renderer::render(int global_w, int global_h)
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
void ftgl_renderer::print(GLfloat x, GLfloat y, const std::string &str)
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

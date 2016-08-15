_Pragma("once")

#include <iostream>
#include <fstream>
#include <sstream>
#include <iterator>
#include "util/glsl.h"
#include "util/common.h"

extern "C" {
#   include <stb/stb_truetype.h>
}
struct font_renderer {
    stbtt_bakedchar cdata[96];
    GLuint          ftex{};
    GLuint          vbo{};
    GLuint          vao{};
    GLuint          prog{};
    std::vector<uint8_t> font_data;
    std::vector<float> vdata{};
    font_renderer() = default;
    font_renderer(font_renderer &&) noexcept = default;
    font_renderer&operator=(font_renderer &&o) noexcept
    {
        using std::swap;
        swap(cdata,o.cdata);
        swap(ftex,o.ftex);
        swap(vbo,o.vbo);
        swap(vao,o.vao);
        swap(prog,o.prog);
        swap(font_data,o.font_data);
        swap(vdata,o.vdata);
        return *this;
    };
    font_renderer(const char *filename, float ptsize)
    {
        {
            std::ifstream ifs{filename};
            std::copy(std::istreambuf_iterator<char>{ifs},
                      std::istreambuf_iterator<char>{},
                      std::back_inserter(font_data));
        }
        {
            auto bitmap = std::make_unique<uint8_t[]>(512*512);
            stbtt_BakeFontBitmap(font_data.data(),0, ptsize, bitmap.get(), 512,512,32,96,cdata);
            glGenTextures(1, &ftex);
            glBindTexture(GL_TEXTURE_2D, ftex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512,512,0,GL_RED,GL_UNSIGNED_BYTE, bitmap.get());
        }
        glGenBuffers(1, &vbo);
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        prog = load_shader_noheader("#text.v.glsl", "#text.f.glsl");
        glUseProgram(prog);
        glUniform1i(1,0);
        CHECK_GL();
    }
    void print(float &x, float &y, const char *text)
    {
        while(*text){
            if(*text >= 32) {
                auto q = stbtt_aligned_quad{};
                stbtt_GetBakedQuad(cdata, 512,512, *text-32, &x, &y, &q, 1);
                GLfloat vertices[] = {
                    q.x0, 2 * y - q.y0, q.s0, q.t0,
                    q.x0, 2 * y - q.y1, q.s0, q.t1,
                    q.x1, 2 * y - q.y0, q.s1, q.t0,
                    q.x1, 2 * y - q.y0, q.s1, q.t0,
                    q.x0, 2 * y - q.y1, q.s0, q.t1,
                    q.x1, 2 * y - q.y1, q.s1, q.t1 };
                for(auto && v : vertices) {
                    vdata.push_back(v);
                }
            }
            text++;
        }
    }
    void prepare()
    {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vdata[0]) * vdata.size(), &vdata[0], GL_STREAM_DRAW);
        CHECK_GL();
    }
    void draw( int w, int h)
    {
        glUseProgram(prog);
        glUniform2f( 0, w, h);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBindVertexArray(vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ftex);
        glDrawArrays(GL_TRIANGLES, 0, vdata.size()/4);
        glBindVertexArray(0);
        CHECK_GL();
    }
    void clear()
    {
        vdata.clear();
    }
   ~font_renderer()
    {
        glDeleteBuffers(1,&vbo);
        glDeleteVertexArrays(1,&vao);
        glDeleteTextures(1, &ftex);
        CHECK_GL();
    }
    std::tuple<float,float,float,float> bbox()
    {
        auto x0 = std::numeric_limits<float>::max(), x1 = 0.f, y0 = x0, y1 = 0.f;
        for(auto i = 0ul; i < vdata.size(); i+=4){
            x0 = std::min(x0,vdata[i]);
            x1 = std::max(x1,vdata[i]);
            y0 = std::min(y0,vdata[i+1]);
            y1 = std::max(y1,vdata[i+1]);

        }
        return std::make_tuple(x0,x1,y0,y1);
    }
};

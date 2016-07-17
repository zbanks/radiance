#include "util/glsl.h"
#include "util/err.h"

#include "util/string.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char * load_shader_error = 0;

/*
const char default_vertex_shader[] = "                          \n\
#version 130                                                    \n\
void main(void) {                                               \n\
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;     \n\
}                                                               \n\
";
*/

static char * read_file(const char * filename, ssize_t * length) {
    char * buffer = 0;
    FILE * f = fopen(filename, "r");

    if(f == NULL) {
        load_shader_error = rsprintf("Could not open file: %s (%s)", filename, strerror(errno));
        return 0;
    }

    int r;

    if((r = fseek(f, 0, SEEK_END)) != 0) {
        load_shader_error = rsprintf("fseek returned %d on file: %s (%s)", r, filename, strerror(errno));
        return 0;
    }
    if((*length = ftell(f)) < 0) {
        load_shader_error = rsprintf("ftell returned %d on file: %s (%s)", r, filename, strerror(errno));
        return 0;
    }
    
    if((r = fseek(f, 0, SEEK_SET)) != 0) {
        load_shader_error = rsprintf("fseek returned %d on file: %s (%s)", r, filename, strerror(errno));
        return 0;
    }

    buffer = calloc(*length + 1, 1);
    if(buffer == NULL) {
        load_shader_error = rsprintf("Could not allocate memory for contents of %s (%s)", filename, strerror(errno));
        return 0;
    }

    if(fread(buffer, 1, *length, f) != (size_t) *length) {
        load_shader_error = rsprintf("Could not read entire contents of %s (%s)", filename, strerror(errno));
        return 0;
    }

    fclose (f);

    buffer[*length] = '\0';

    return buffer;
}

GLhandleARB load_shader(const char * filename) {
    // Load file
    GLcharARB * buffer = NULL;
    GLint length;
    ssize_t prog_len = 0, head_len = 0;
    char * prog_buffer = read_file(filename, &prog_len);
    char * head_buffer = read_file("resources/header.glsl", &head_len);
    if (prog_buffer != NULL || head_buffer != NULL) {
        buffer = rsprintf("%s%s", head_buffer, prog_buffer);
    }
    free(prog_buffer);
    free(head_buffer);
    if (buffer == NULL) return 0;
    length = strlen(buffer);

    GLint compiled;

    // Compile
    GLhandleARB fragmentShaderObj;
    fragmentShaderObj = glCreateShaderObjectARB(GL_FRAGMENT_SHADER);
    glShaderSourceARB(fragmentShaderObj, 1, (const GLcharARB **)&buffer, (const GLint *)&length);
    glCompileShader(fragmentShaderObj);
    glGetShaderiv(fragmentShaderObj, GL_COMPILE_STATUS, &compiled);
    if(!compiled) {
        GLint blen = 0; 
        GLsizei slen = 0;

        glGetShaderiv(fragmentShaderObj, GL_INFO_LOG_LENGTH , &blen);
        if(blen > 1) {
            GLchar* compiler_log = (GLchar*)calloc(blen, 1);
            glGetShaderInfoLog(fragmentShaderObj, blen, &slen, compiler_log);
            load_shader_error = rsprintf("Shader compilation failed, Log:\n%s", compiler_log);
            free(compiler_log);
        } else {
            load_shader_error = strdup("Shader compilation failed!");
        }
        glDeleteObjectARB(fragmentShaderObj);
        free(buffer);
        return 0;
    }
    free(buffer);

    /*
    length = sizeof(default_vertex_shader) - 1;
    buffer = calloc(length, 1);
    if(buffer == NULL) {
        load_shader_error = rsprintf("Could not allocate memory for default vertex shader");
        return 0;
    }
    memcpy(buffer, default_vertex_shader, sizeof(default_vertex_shader) - 1);

    // Compile default vertex shader
    GLhandleARB vertexShaderObj;
    vertexShaderObj = glCreateShaderObjectARB(GL_VERTEX_SHADER);
    glShaderSourceARB(vertexShaderObj, 1, (const GLcharARB **)&buffer, (const GLint *)&length);
    glCompileShader(vertexShaderObj);
    glGetShaderiv(vertexShaderObj, GL_COMPILE_STATUS, &compiled);
    if(!compiled) {
        GLint blen = 0; 
        GLsizei slen = 0;

        glGetShaderiv(vertexShaderObj, GL_INFO_LOG_LENGTH , &blen);
        if(blen > 1) {
            GLchar* compiler_log = (GLchar*)calloc(blen, 1);
            glGetShaderInfoLog(vertexShaderObj, blen, &slen, compiler_log);
            load_shader_error = rsprintf("Shader compilation failed!\n%s", compiler_log);
            free(compiler_log);
        } else {
            load_shader_error = strdup("Shader compilation failed!");
        }
        glDeleteObjectARB(vertexShaderObj);
        free(buffer);
        return 0;
    }
    free(buffer);
    */

    // Link
    GLhandleARB programObj;
    programObj = glCreateProgramObjectARB();
    //glAttachShader(programObj, vertexShaderObj);
    glAttachShader(programObj, fragmentShaderObj);
    glLinkProgram(programObj);

    GLint linked;
    glGetProgramiv(programObj, GL_LINK_STATUS, &linked);
    if(!linked) {
        GLint blen = 0; 
        GLsizei slen = 0;

        glGetProgramiv(programObj, GL_INFO_LOG_LENGTH , &blen);
        if(blen > 1) {
            GLchar* linker_log = (GLchar*)calloc(blen, 1);
            glGetProgramInfoLog(programObj, blen, &slen, linker_log);
            load_shader_error = rsprintf("Shader linking failed; Log:\n%s", linker_log);
            free(linker_log);
        } else {
            load_shader_error = strdup("Shader linking failed!");
        }
        glDetachShader(programObj, fragmentShaderObj);
        glDeleteObjectARB(fragmentShaderObj);
        glDeleteObjectARB(programObj);
        return 0;
    }
    glDetachShader(programObj, fragmentShaderObj);
    glDeleteShader(fragmentShaderObj);
    return programObj;
}

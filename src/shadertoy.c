/** @file shadertoy.c
 *  @brief see shadertoy.h , file loading reused from
 *         opengl_depth_and_color_renderer shader_loader.c
 *  @author Ammar Qammaz (AmmarkoV)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GL/glew.h>
#include <GL/gl.h>

#include "shadertoy.h"
#include "../opengl_depth_and_color_renderer/src/Library/Rendering/ShaderPipeline/shader_loader.h"

static const char * shadertoyPreamble =
"#version 130\n"
"uniform vec3  iResolution;\n"
"uniform float iTime;\n"
"uniform float iTimeDelta;\n"
"uniform int   iFrame;\n"
"uniform vec4  iMouse;\n"
"uniform vec4  iDate;\n"
"uniform float iSampleRate;\n"
"uniform sampler2D iChannel0;\n"
"uniform sampler2D iChannel1;\n"
"uniform sampler2D iChannel2;\n"
"uniform sampler2D iChannel3;\n"
"uniform vec3  iChannelResolution[4];\n"
"#define iGlobalTime iTime\n"
"#line 1\n";

static const char * shadertoyFooter =
"\nvoid main()\n"
"{\n"
"  vec4 color = vec4(0.0);\n"
"  mainImage(color,gl_FragCoord.xy);\n"
"  gl_FragColor = vec4(color.rgb,1.0);\n"
"}\n";

static const char * fullscreenVertexShader =
"#version 130\n"
"in vec2 vPos;\n"
"void main() { gl_Position = vec4(vPos,0.0,1.0); }\n";

static GLuint quadVAO = 0;
static GLuint quadVBO = 0;

static GLuint compileOneShader(GLenum type,const char * source,const char * label)
{
  GLuint sh = glCreateShader(type);
  glShaderSource(sh,1,(const GLchar **) &source,0);
  glCompileShader(sh);
  GLint isok=0;
  glGetShaderiv(sh, GL_COMPILE_STATUS, &isok);
  if (!isok)
  {
    GLchar info[2048]; GLsizei length;
    glGetShaderInfoLog(sh,2048,&length,info);
    fprintf(stderr,"Could not compile %s shader :\n%s\n",label,info);
    glDeleteShader(sh);
    return 0;
  }
  return sh;
}

unsigned int shadertoy_compileProgram(const char * vertexSource,const char * fragmentSource)
{
  GLuint vs = compileOneShader(GL_VERTEX_SHADER  ,vertexSource  ,"vertex");
  if (!vs) { return 0; }
  GLuint fs = compileOneShader(GL_FRAGMENT_SHADER,fragmentSource,"fragment");
  if (!fs) { glDeleteShader(vs); return 0; }

  GLuint program = glCreateProgram();
  glAttachShader(program,vs);
  glAttachShader(program,fs);
  glBindAttribLocation(program,0,"vPos");
  glLinkProgram(program);

  GLint isok=0;
  glGetProgramiv(program, GL_LINK_STATUS, &isok);
  if (!isok)
  {
    GLchar info[2048]; GLsizei length;
    glGetProgramInfoLog(program,2048,&length,info);
    fprintf(stderr,"Could not link shaders :\n%s\n",info);
    glDeleteProgram(program); program=0;
  }
  glDeleteShader(vs);
  glDeleteShader(fs);
  return program;
}

int shadertoy_init()
{
  static const float quad[] = { -1.0f,-1.0f,  1.0f,-1.0f,  1.0f,1.0f,
                                -1.0f,-1.0f,  1.0f, 1.0f, -1.0f,1.0f };
  glGenVertexArrays(1,&quadVAO);
  glBindVertexArray(quadVAO);
  glGenBuffers(1,&quadVBO);
  glBindBuffer(GL_ARRAY_BUFFER,quadVBO);
  glBufferData(GL_ARRAY_BUFFER,sizeof(quad),quad,GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,0);
  glBindVertexArray(0);
  return 1;
}

struct shadertoyEffect * shadertoy_load(const char * fragmentFile)
{
  int fileLength=0;
  char * body = loadShaderFileToMem((char *) fragmentFile,&fileLength);
  if (body==0) { fprintf(stderr,"Could not load shadertoy file %s \n",fragmentFile); return 0; }

  unsigned int fullLength = strlen(shadertoyPreamble) + fileLength + strlen(shadertoyFooter) + 1;
  char * full = (char *) malloc(fullLength);
  if (full==0) { free(body); return 0; }
  snprintf(full,fullLength,"%s%s%s",shadertoyPreamble,body,shadertoyFooter);
  free(body);

  unsigned int program = shadertoy_compileProgram(fullscreenVertexShader,full);
  free(full);
  if (program==0) { fprintf(stderr,"Failed building shadertoy effect %s \n",fragmentFile); return 0; }

  struct shadertoyEffect * fx = (struct shadertoyEffect *) malloc(sizeof(struct shadertoyEffect));
  if (fx==0) { glDeleteProgram(program); return 0; }
  fx->program       = program;
  fx->locResolution = glGetUniformLocation(program,"iResolution");
  fx->locTime       = glGetUniformLocation(program,"iTime");
  fx->locMouse      = glGetUniformLocation(program,"iMouse");
  fx->locFrame      = glGetUniformLocation(program,"iFrame");
  fx->locChannel0   = glGetUniformLocation(program,"iChannel0");
  fx->locChannel1   = glGetUniformLocation(program,"iChannel1");
  fprintf(stderr,"Loaded shadertoy effect %s \n",fragmentFile);
  return fx;
}

void shadertoy_draw(struct shadertoyEffect * fx,
                    float time,int frame,
                    float width,float height,
                    float mouseX,float mouseY,
                    unsigned int chan0Tex,unsigned int chan1Tex)
{
  if (fx==0) { return; }
  glUseProgram(fx->program);

  if (fx->locResolution>=0) { glUniform3f(fx->locResolution,width,height,1.0f); }
  if (fx->locTime>=0)       { glUniform1f(fx->locTime,time); }
  if (fx->locFrame>=0)      { glUniform1i(fx->locFrame,frame); }
  //ShaderToy mouse coordinates have y going up
  if (fx->locMouse>=0)      { glUniform4f(fx->locMouse,mouseX,height-mouseY,mouseX,height-mouseY); }

  if (chan0Tex)
  {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,chan0Tex);
    if (fx->locChannel0>=0) { glUniform1i(fx->locChannel0,0); }
  }
  if (chan1Tex)
  {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D,chan1Tex);
    if (fx->locChannel1>=0) { glUniform1i(fx->locChannel1,1); }
  }

  glBindVertexArray(quadVAO);
  glDrawArrays(GL_TRIANGLES,0,6);
  glBindVertexArray(0);
  glActiveTexture(GL_TEXTURE0);
  glUseProgram(0);
}

void shadertoy_unload(struct shadertoyEffect * fx)
{
  if (fx==0) { return; }
  glDeleteProgram(fx->program);
  free(fx);
}

/** @file shadertoy.h
 *  @brief Compile ShaderToy-style fragment shaders ( files containing a
 *         mainImage(out vec4,in vec2) function ) behind a common preamble
 *         that declares iResolution / iTime / iMouse / iChannel0..3 , and
 *         draw them on a fullscreen quad.
 *         Any single-pass shader from shadertoy.com can be pasted in a file
 *         under shaders/ and loaded with shadertoy_load()
 *  @author Ammar Qammaz (AmmarkoV)
 */

#ifndef SHADERTOY_H_INCLUDED
#define SHADERTOY_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

struct shadertoyEffect
{
  unsigned int program;
  int locResolution;
  int locTime;
  int locMouse;
  int locFrame;
  int locChannel0;
  int locChannel1;
  int locDate;
  int locMoonPhase;
};

/* Current lunar phase used by the sleepy scene shader through the
   iMoonPhase uniform : 0=new , 0.25=first quarter , 0.5=full , 0.75=last */
void shadertoy_setMoonPhase(float phase);

/* Create the shared fullscreen quad , call once after GL context + GLEW are up */
int shadertoy_init();

/* Load + compile shaders/<file> , retval 0=Failure */
struct shadertoyEffect * shadertoy_load(const char * fragmentFile);

/* Draw the effect on a fullscreen quad
   chan0Tex / chan1Tex are GL texture ids bound to iChannel0 / iChannel1 , 0 = none */
void shadertoy_draw(struct shadertoyEffect * fx,
                    float time,int frame,
                    float width,float height,
                    float mouseX,float mouseY,
                    unsigned int chan0Tex,unsigned int chan1Tex);

void shadertoy_unload(struct shadertoyEffect * fx);

/* Also used by sprites : compile a program from in-memory sources ,
   binds attribute "vPos" to location 0 , retval 0=Failure */
unsigned int shadertoy_compileProgram(const char * vertexSource,const char * fragmentSource);

#ifdef __cplusplus
}
#endif

#endif

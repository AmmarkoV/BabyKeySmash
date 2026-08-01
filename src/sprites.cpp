/** @file sprites.cpp
 *  @brief see sprites.h , C-style code , OpenCV used for PNG loading and
 *         rasterizing letters ( cv::putText ) into GL textures
 *  @author Ammar Qammaz (AmmarkoV)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <GL/glew.h>
#include <GL/gl.h>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/utils/filesystem.hpp>

#include "sprites.h"
#include "shadertoy.h"

#define MAX_SPRITES 512
#define MAX_TEXTURES 256

struct spriteTexture
{
  GLuint tex;
  float aspect; /* width / height */
  int noTint;   /* emoji keep their original colors */
};

struct sprite
{
  int active;
  int isTrail;
  float x,y;           /* pixels , center */
  float vx,vy;
  float rot,vrot;
  float targetHeight;  /* pixels */
  float age,life;
  float r,g,b;
  GLuint tex;
  float aspect;
};

static struct spriteTexture textures[MAX_TEXTURES];
static int numberOfTextures = 0;
static GLuint trailTexture = 0;

static struct spriteTexture letterCache[128];

#define GREEK_LETTERS 24
static struct spriteTexture greekLetters[GREEK_LETTERS];
static int greekLoaded = 0;

/* Greek keyboard layout : which Greek letter ( alphabet index 0=alpha ..
   23=omega ) each Latin key a-z produces , -1 = no Greek letter on that key */
static const int latinToGreek[26] =
{
  /*a*/ 0,  /*b*/ 1,  /*c*/ 22, /*d*/ 3,  /*e*/ 4,  /*f*/ 20,
  /*g*/ 2,  /*h*/ 6,  /*i*/ 8,  /*j*/ 13, /*k*/ 9,  /*l*/ 10,
  /*m*/ 11, /*n*/ 12, /*o*/ 14, /*p*/ 15, /*q*/ -1, /*r*/ 16,
  /*s*/ 17, /*t*/ 18, /*u*/ 7,  /*v*/ 23, /*w*/ 17, /*x*/ 21,
  /*y*/ 19, /*z*/ 5
};

static struct sprite sprites[MAX_SPRITES];

static int screenW = 0 , screenH = 0;

static GLuint spriteProgram = 0;
static GLint locCenter,locSize,locRot,locScreen,locTint,locTexture;
static GLuint spriteVAO = 0 , spriteVBO = 0;

static const char * spriteVertexShader =
"#version 130\n"
"in vec2 vPos;\n"                      /* unit quad -0.5 .. 0.5 */
"uniform vec2 uCenter;\n"              /* pixels , y down */
"uniform vec2 uSize;\n"                /* pixels */
"uniform float uRot;\n"
"uniform vec2 uScreen;\n"
"out vec2 uv;\n"
"void main()\n"
"{\n"
"  uv = vPos + vec2(0.5);\n"
"  float c = cos(uRot); float s = sin(uRot);\n"
"  vec2 offset = vec2( c*vPos.x - s*vPos.y , s*vPos.x + c*vPos.y ) * uSize;\n"
"  vec2 pixel = uCenter + vec2(offset.x,-offset.y);\n"
"  vec2 ndc = (pixel/uScreen)*2.0 - 1.0;\n"
"  gl_Position = vec4(ndc.x,-ndc.y,0.0,1.0);\n"
"}\n";

static const char * spriteFragmentShader =
"#version 130\n"
"in vec2 uv;\n"
"uniform sampler2D uTexture;\n"
"uniform vec4 uTint;\n"
"void main()\n"
"{\n"
"  vec4 t = texture(uTexture,uv);\n"
"  gl_FragColor = vec4(t.rgb*uTint.rgb , t.a*uTint.a);\n"
"}\n";

static float randomFloat(float minimum,float maximum)
{
  return minimum + ( (float) rand() / (float) RAND_MAX ) * (maximum-minimum);
}

static GLuint uploadRGBAMat(const cv::Mat & rgba)
{
  GLuint tex=0;
  glGenTextures(1,&tex);
  glBindTexture(GL_TEXTURE_2D,tex);
  glPixelStorei(GL_UNPACK_ALIGNMENT,1);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,rgba.cols,rgba.rows,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D,0);
  return tex;
}

static int loadTexturesFromDirectory(const char * directory)
{
  std::vector<cv::String> files;
  cv::glob(cv::String(directory)+"/*.png",files,false);

  unsigned int i;
  for (i=0; i<files.size(); i++)
  {
    if (numberOfTextures>=MAX_TEXTURES) { break; }
    cv::Mat img = cv::imread(files[i],cv::IMREAD_UNCHANGED);
    if (img.empty()) { fprintf(stderr,"Could not read texture %s \n",files[i].c_str()); continue; }

    cv::Mat rgba;
    if (img.channels()==4)      { cv::cvtColor(img,rgba,cv::COLOR_BGRA2RGBA); } else
    if (img.channels()==3)      { cv::cvtColor(img,rgba,cv::COLOR_BGR2RGBA);  } else
    if (img.channels()==1)      { cv::cvtColor(img,rgba,cv::COLOR_GRAY2RGBA); } else
                                { continue; }
    cv::flip(rgba,rgba,0); /* GL textures have their origin bottom-left */

    GLuint tex = uploadRGBAMat(rgba);
    float aspect = (float) rgba.cols / (float) rgba.rows;

    //The soft dot is reserved for the mouse trail
    if (files[i].find("trail")!=cv::String::npos)
       { trailTexture = tex; }
        else
       {
         textures[numberOfTextures].tex    = tex;
         textures[numberOfTextures].aspect = aspect;
         textures[numberOfTextures].noTint = ( files[i].find("emoji")!=cv::String::npos );
         numberOfTextures++;
       }
    fprintf(stderr,"Loaded texture %s \n",files[i].c_str());
  }
  return numberOfTextures;
}

static GLuint getLetterTexture(char character,float * aspect)
{
  int idx = (int) character;
  if ( (idx<0) || (idx>=128) ) { return 0; }
  if (letterCache[idx].tex!=0) { *aspect = letterCache[idx].aspect; return letterCache[idx].tex; }

  char text[2] = { character , 0 };
  int fontFace = cv::FONT_HERSHEY_DUPLEX;
  double fontScale = 8.0;
  int thickness = 16;
  int baseline = 0;
  cv::Size sz = cv::getTextSize(text,fontFace,fontScale,thickness,&baseline);

  int pad = 40;
  cv::Mat rgba(sz.height+baseline+2*pad , sz.width+2*pad , CV_8UC4 , cv::Scalar(0,0,0,0));
  cv::Point origin(pad , pad+sz.height);
  //Dark outline first then white body ( tinted per sprite at draw time )
  cv::putText(rgba,text,origin,fontFace,fontScale,cv::Scalar( 40, 40, 40,255),thickness+10,cv::LINE_AA);
  cv::putText(rgba,text,origin,fontFace,fontScale,cv::Scalar(255,255,255,255),thickness   ,cv::LINE_AA);
  cv::flip(rgba,rgba,0);

  letterCache[idx].tex    = uploadRGBAMat(rgba);
  letterCache[idx].aspect = (float) rgba.cols / (float) rgba.rows;
  *aspect = letterCache[idx].aspect;
  return letterCache[idx].tex;
}

static struct sprite * findFreeSprite()
{
  int i;
  for (i=0; i<MAX_SPRITES; i++)
    { if (!sprites[i].active) { return &sprites[i]; } }
  return 0;
}

static void randomBrightColor(float * r,float * g,float * b)
{
  //A random hue at full saturation keeps colors kid-bright
  float h = randomFloat(0.0f,6.0f);
  float x = 1.0f - fabsf(fmodf(h,2.0f)-1.0f);
  float cr=0,cg=0,cb=0;
       if (h<1.0f) { cr=1; cg=x;  } else
       if (h<2.0f) { cr=x; cg=1;  } else
       if (h<3.0f) { cg=1; cb=x;  } else
       if (h<4.0f) { cg=x; cb=1;  } else
       if (h<5.0f) { cr=x; cb=1;  } else
                   { cr=1; cb=x;  }
  //Mix towards white so baked texture colors still show through
  *r = 0.55f + 0.45f*cr;
  *g = 0.55f + 0.45f*cg;
  *b = 0.55f + 0.45f*cb;
}

static void spawnCommon(struct sprite * s,float x,float y,GLuint tex,float aspect,float targetHeight,float life)
{
  if (x<0) { x = randomFloat(0.1f,0.9f) * screenW; }
  if (y<0) { y = randomFloat(0.15f,0.85f) * screenH; }
  s->active = 1;
  s->isTrail = 0;
  s->x = x; s->y = y;
  s->vx = randomFloat(-30.0f,30.0f);
  s->vy = randomFloat(-90.0f,-30.0f); /* balloons drift up */
  s->rot = randomFloat(-0.3f,0.3f);
  s->vrot = randomFloat(-0.8f,0.8f);
  s->targetHeight = targetHeight;
  s->age = 0.0f;
  s->life = life;
  s->tex = tex;
  s->aspect = aspect;
  randomBrightColor(&s->r,&s->g,&s->b);
}

void sprites_spawnRandomTexture(float x,float y)
{
  if (numberOfTextures==0) { sprites_spawnLetter('A'+(rand()%26)); return; }
  struct sprite * s = findFreeSprite();
  if (s==0) { return; }
  int pick = rand()%numberOfTextures;
  spawnCommon(s,x,y,textures[pick].tex,textures[pick].aspect,
              randomFloat(0.18f,0.32f)*screenH,randomFloat(1.8f,2.8f));
  if (textures[pick].noTint) { s->r=1.0f; s->g=1.0f; s->b=1.0f; }
}

int sprites_loadGreek(const char * greekDirectory)
{
  //Greek glyphs can not be rasterized at runtime ( cv::putText only covers
  //ASCII ) so they are pre-baked by tools/make_textures.py in alphabet order
  std::vector<cv::String> files;
  cv::glob(cv::String(greekDirectory)+"/*.png",files,false);
  if (files.size()<GREEK_LETTERS)
  {
    fprintf(stderr,"Found %u/%u Greek letters in %s , run tools/make_textures.py\n",
            (unsigned int) files.size(),GREEK_LETTERS,greekDirectory);
    return 0;
  }

  int i;
  for (i=0; i<GREEK_LETTERS; i++)
  {
    cv::Mat img = cv::imread(files[i],cv::IMREAD_UNCHANGED);
    if ( (img.empty()) || (img.channels()!=4) )
       { fprintf(stderr,"Could not read Greek letter %s \n",files[i].c_str()); return 0; }
    cv::Mat rgba;
    cv::cvtColor(img,rgba,cv::COLOR_BGRA2RGBA);
    cv::flip(rgba,rgba,0);
    greekLetters[i].tex    = uploadRGBAMat(rgba);
    greekLetters[i].aspect = (float) rgba.cols / (float) rgba.rows;
  }
  greekLoaded = 1;
  fprintf(stderr,"Loaded %u Greek letters from %s \n",GREEK_LETTERS,greekDirectory);
  return GREEK_LETTERS;
}

void sprites_spawnGreek(int alphabetIndex)
{
  if ( (!greekLoaded) || (alphabetIndex<0) || (alphabetIndex>=GREEK_LETTERS) )
     { sprites_spawnRandomTexture(-1,-1); return; }
  struct sprite * s = findFreeSprite();
  if (s==0) { return; }
  spawnCommon(s,-1,-1,greekLetters[alphabetIndex].tex,greekLetters[alphabetIndex].aspect,
              randomFloat(0.2f,0.35f)*screenH,randomFloat(1.8f,2.8f));
}

void sprites_spawnLetter(char character)
{
  if ( (greekLoaded) && (character>='A') && (character<='Z') )
     { sprites_spawnGreek(latinToGreek[character-'A']); return; }

  struct sprite * s = findFreeSprite();
  if (s==0) { return; }
  float aspect=1.0f;
  GLuint tex = getLetterTexture(character,&aspect);
  if (tex==0) { return; }
  spawnCommon(s,-1,-1,tex,aspect,randomFloat(0.2f,0.35f)*screenH,randomFloat(1.8f,2.8f));
}

void sprites_spawnTrail(float x,float y)
{
  struct sprite * s = findFreeSprite();
  if (s==0) { return; }
  GLuint tex = trailTexture;
  float aspect = 1.0f;
  if (tex==0)
  {
    if (numberOfTextures==0) { return; }
    tex = textures[rand()%numberOfTextures].tex;
  }
  spawnCommon(s,x,y,tex,aspect,randomFloat(0.04f,0.07f)*screenH,0.8f);
  s->isTrail = 1;
  s->vx = randomFloat(-20.0f,20.0f);
  s->vy = randomFloat(-20.0f,20.0f);
}

int sprites_init(const char * textureDirectory,int screenWidth,int screenHeight)
{
  screenW = screenWidth;
  screenH = screenHeight;
  memset(sprites,0,sizeof(sprites));
  memset(letterCache,0,sizeof(letterCache));

  loadTexturesFromDirectory(textureDirectory);
  if (numberOfTextures==0)
    { fprintf(stderr,"No textures found in %s , only letters will pop\n",textureDirectory); }

  spriteProgram = shadertoy_compileProgram(spriteVertexShader,spriteFragmentShader);
  if (spriteProgram==0) { return 0; }
  locCenter  = glGetUniformLocation(spriteProgram,"uCenter");
  locSize    = glGetUniformLocation(spriteProgram,"uSize");
  locRot     = glGetUniformLocation(spriteProgram,"uRot");
  locScreen  = glGetUniformLocation(spriteProgram,"uScreen");
  locTint    = glGetUniformLocation(spriteProgram,"uTint");
  locTexture = glGetUniformLocation(spriteProgram,"uTexture");

  static const float quad[] = { -0.5f,-0.5f,  0.5f,-0.5f,  0.5f,0.5f,
                                -0.5f,-0.5f,  0.5f, 0.5f, -0.5f,0.5f };
  glGenVertexArrays(1,&spriteVAO);
  glBindVertexArray(spriteVAO);
  glGenBuffers(1,&spriteVBO);
  glBindBuffer(GL_ARRAY_BUFFER,spriteVBO);
  glBufferData(GL_ARRAY_BUFFER,sizeof(quad),quad,GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,0);
  glBindVertexArray(0);
  return 1;
}

static float easeOutBack(float t)
{
  //Overshooting ease makes sprites "pop"
  float c1 = 1.70158f;
  float c3 = c1 + 1.0f;
  float u = t - 1.0f;
  return 1.0f + c3*u*u*u + c1*u*u;
}

void sprites_updateAndDraw(float deltaSeconds)
{
  glUseProgram(spriteProgram);
  glUniform2f(locScreen,(float) screenW,(float) screenH);
  glUniform1i(locTexture,0);
  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glBindVertexArray(spriteVAO);

  int i;
  for (i=0; i<MAX_SPRITES; i++)
  {
    struct sprite * s = &sprites[i];
    if (!s->active) { continue; }

    s->age += deltaSeconds;
    if (s->age >= s->life) { s->active=0; continue; }

    s->x += s->vx * deltaSeconds;
    s->y += s->vy * deltaSeconds;
    s->rot += s->vrot * deltaSeconds;

    float popPhase = s->age * 5.0f;
    float scale = (popPhase<1.0f) ? easeOutBack(popPhase) : 1.0f;
    float height = s->targetHeight * scale;

    //Fade out during the last 30% of the sprite lifetime
    float fadeStart = s->life * 0.7f;
    float alpha = (s->age<fadeStart) ? 1.0f : 1.0f - (s->age-fadeStart)/(s->life-fadeStart);

    glUniform2f(locCenter,s->x,s->y);
    glUniform2f(locSize,height*s->aspect,height);
    glUniform1f(locRot,s->rot);
    glUniform4f(locTint,s->r,s->g,s->b,alpha);
    glBindTexture(GL_TEXTURE_2D,s->tex);
    glDrawArrays(GL_TRIANGLES,0,6);
  }

  glBindVertexArray(0);
  glDisable(GL_BLEND);
  glUseProgram(0);
}

void sprites_close()
{
  int i;
  for (i=0; i<numberOfTextures; i++) { glDeleteTextures(1,&textures[i].tex); }
  for (i=0; i<128; i++) { if (letterCache[i].tex) { glDeleteTextures(1,&letterCache[i].tex); } }
  if (greekLoaded) { for (i=0; i<GREEK_LETTERS; i++) { glDeleteTextures(1,&greekLetters[i].tex); } greekLoaded=0; }
  if (trailTexture) { glDeleteTextures(1,&trailTexture); }
  if (spriteProgram) { glDeleteProgram(spriteProgram); }
  numberOfTextures=0;
}

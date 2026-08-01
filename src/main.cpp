/** @file main.cpp
 *  @brief BabyKeySmash : a fullscreen toy for toddlers , every keystroke and
 *         mouse move pops colorful sprites while microphone and webcam feed
 *         the shader background . Hold Escape 3 seconds to quit.
 *  @author Ammar Qammaz (AmmarkoV)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

#include <GL/glew.h>
#include <GL/gl.h>

#define XK_GREEK
#include <X11/keysym.h>

#include "glx_window.h"
#include "shadertoy.h"
#include "sprites.h"
#include "webcam.h"
#include "audio_alsa.h"

static float mouseX = 0.0f , mouseY = 0.0f;
static float lastTrailX = -1000.0f , lastTrailY = -1000.0f;

static int quitWordTyped = 0;
static char typedWordBuffer[32];

static unsigned long keysHandled = 0;
static unsigned long mouseMovesHandled = 0;
static unsigned long clicksHandled = 0;

/* Optional hook scripts , e.g. switching keyboard lighting with
   polychromatic-cli while the app runs and restoring it on exit */
static void runHookScript(const char * script)
{
  if (access(script,X_OK)!=0) { return; }
  fprintf(stderr,"Running hook %s ..\n",script);
  int result = system(script);
  if (result!=0) { fprintf(stderr,"Hook %s returned %u \n",script,result); }
}

/* Typing "quit" or "closeapplication" ( Ctrl held or not ) closes the app */
static void rememberTypedLetter(char c)
{
  unsigned int len = strlen(typedWordBuffer);
  if (len+1>=sizeof(typedWordBuffer))
  {
    memmove(typedWordBuffer,typedWordBuffer+1,len);
    len--;
  }
  typedWordBuffer[len]   = c;
  typedWordBuffer[len+1] = 0;
  len++;

  static const char * magicWords[] = { "quit" , "closeapplication" };
  unsigned int w;
  for (w=0; w<sizeof(magicWords)/sizeof(magicWords[0]); w++)
  {
    unsigned int wordLen = strlen(magicWords[w]);
    if ( (len>=wordLen) && (strcmp(typedWordBuffer+len-wordLen,magicWords[w])==0) )
    {
      fprintf(stderr,"Magic word %s typed , quitting..\n",magicWords[w]);
      quitWordTyped = 1;
    }
  }
}

static double getTimeSeconds()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC,&ts);
  return (double) ts.tv_sec + (double) ts.tv_nsec / 1000000000.0;
}

/* Keysym -> Greek alphabet index ( 0=alpha .. 23=omega ) for a Greek
   keyboard layout , -1 = not a Greek letter . The keysym table has a gap at
   0x7d3 in the capitals and the final sigma at 0x7f3 in the smalls */
static int greekKeysymToIndex(unsigned long keysym)
{
  if ( (keysym>=XK_Greek_ALPHA) && (keysym<=XK_Greek_SIGMA) ) { return keysym-XK_Greek_ALPHA; }
  if ( (keysym>=XK_Greek_TAU)   && (keysym<=XK_Greek_OMEGA) ) { return 18 + (keysym-XK_Greek_TAU); }
  if ( (keysym>=XK_Greek_alpha) && (keysym<=XK_Greek_sigma) ) { return keysym-XK_Greek_alpha; }
  if (keysym==XK_Greek_finalsmallsigma)                       { return 17; }
  if ( (keysym>=XK_Greek_tau)   && (keysym<=XK_Greek_omega) ) { return 18 + (keysym-XK_Greek_tau); }
  return -1;
}

static void onKey(unsigned long keysym)
{
  keysHandled++;
  int greekIndex = greekKeysymToIndex(keysym);
  if (greekIndex>=0)
       { sprites_spawnGreek(greekIndex); } else
  if ( (keysym>=XK_a && keysym<=XK_z) )
       { rememberTypedLetter('a' + (keysym-XK_a)); sprites_spawnLetter('A' + (keysym-XK_a)); } else
  if ( (keysym>=XK_A && keysym<=XK_Z) )
       { rememberTypedLetter('a' + (keysym-XK_A)); sprites_spawnLetter('A' + (keysym-XK_A)); } else
  if ( (keysym>=XK_0 && keysym<=XK_9) )
       { sprites_spawnLetter('0' + (keysym-XK_0)); }
      else
       { sprites_spawnRandomTexture(-1,-1); }
}

static void onMouseMove(int x,int y)
{
  mouseMovesHandled++;
  mouseX = (float) x;
  mouseY = (float) y;
  float dx = x-lastTrailX , dy = y-lastTrailY;
  if (dx*dx+dy*dy > 24.0f*24.0f)
  {
    sprites_spawnTrail((float) x,(float) y);
    lastTrailX = (float) x;
    lastTrailY = (float) y;
  }
}

static void onButton(int button,int isDown,int x,int y)
{
  if (!isDown) { return; }
  clicksHandled++;
  //A little burst where the kid clicked
  int i;
  for (i=0; i<4; i++)
    { sprites_spawnRandomTexture(x+rand()%200-100 , y+rand()%200-100); }
}

int main(int argc,const char ** argv)
{
  srand(time(0));

  int greekMode = 0;
  int i;
  for (i=1; i<argc; i++)
    { if (strcmp(argv[i],"--greek")==0) { greekMode=1; } }

  //Allow launching from anywhere : fall back to the source tree for assets
  if (access("shaders/background.frag",R_OK)!=0)
  {
     fprintf(stderr,"Assets not in current directory , falling back to %s \n",BKS_SOURCE_DIR);
     if (chdir(BKS_SOURCE_DIR)!=0) { fprintf(stderr,"Could not chdir to source directory\n"); }
  }

  runHookScript("scripts/on_start.sh");

  struct babyWindowCallbacks callbacks;
  callbacks.onKey       = onKey;
  callbacks.onMouseMove = onMouseMove;
  callbacks.onButton    = onButton;

  if (!babywin_open(&callbacks))
     { fprintf(stderr,"Could not open window..\n"); runHookScript("scripts/on_exit.sh"); return 1; }

  glewExperimental = GL_TRUE;
  GLenum glewStatus = glewInit();
  if (glewStatus!=GLEW_OK)
     { fprintf(stderr,"GLEW error : %s \n",glewGetErrorString(glewStatus)); runHookScript("scripts/on_exit.sh"); return 1; }

  int width  = babywin_width();
  int height = babywin_height();

  shadertoy_init();
  struct shadertoyEffect * backgroundFx = shadertoy_load("shaders/background.frag");
  struct shadertoyEffect * webcamFx     = shadertoy_load("shaders/webcam.frag");
  if (backgroundFx==0) { fprintf(stderr,"Background shader is required , exiting\n"); runHookScript("scripts/on_exit.sh"); return 1; }

  sprites_init("textures",width,height);
  if (greekMode) { sprites_loadGreek("textures/greek"); }

  int haveWebcam = webcam_start(0);
  int haveAudio  = audio_start();

  fprintf(stderr,"BabyKeySmash running at %ux%u , webcam=%u , microphone=%u \n",
          width,height,haveWebcam,haveAudio);
  fprintf(stderr,"Hold Escape for 3 seconds , type quit / closeapplication , or Ctrl+Shift+Q to quit \n");

  glViewport(0,0,width,height);
  glDisable(GL_DEPTH_TEST);

  double startTime = getTimeSeconds();
  double lastFrameTime = startTime;
  int frame = 0;

  while ( (babywin_processEvents()) && (!quitWordTyped) )
  {
    double now = getTimeSeconds();
    float t  = (float) (now-startTime);
    float dt = (float) (now-lastFrameTime);
    lastFrameTime = now;
    if (dt>0.1f) { dt=0.1f; }

    if (haveAudio)  { audio_update();  }
    int webcamReady = 0;
    if (haveWebcam) { webcamReady = webcam_update(); }

    //Background : always on , microphone spectrum on iChannel0
    shadertoy_draw(backgroundFx,t,frame,(float) width,(float) height,
                   mouseX,mouseY,audio_texture(),0);

    //Webcam effect blended on top with a slowly breathing mix factor
    if ( (webcamReady) && (webcamFx) )
    {
      float mix = 0.45f + 0.30f * (float) sin(now*0.15);
      glEnable(GL_BLEND);
      glBlendColor(0.0f,0.0f,0.0f,mix);
      glBlendFunc(GL_CONSTANT_ALPHA,GL_ONE_MINUS_CONSTANT_ALPHA);
      shadertoy_draw(webcamFx,t,frame,(float) width,(float) height,
                     mouseX,mouseY,webcam_texture(),audio_texture());
      glDisable(GL_BLEND);
    }

    sprites_updateAndDraw(dt);

    babywin_swap();
    frame++;

    //Modest frame cap so a driver without vsync does not spin at 100% CPU
    double frameTime = getTimeSeconds()-now;
    if (frameTime<0.016) { usleep((unsigned int)((0.016-frameTime)*1000000.0)); }
  }

  fprintf(stderr,"Shutting down..\n");
  fprintf(stderr,"Session smash report : %lu keystrokes , %lu mouse moves , %lu clicks , %lu events total :)\n",
          keysHandled,mouseMovesHandled,clicksHandled,
          keysHandled+mouseMovesHandled+clicksHandled);
  webcam_stop();
  audio_stop();
  sprites_close();
  shadertoy_unload(backgroundFx);
  shadertoy_unload(webcamFx);
  babywin_close();
  runHookScript("scripts/on_exit.sh");
  return 0;
}

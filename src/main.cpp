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
#include "soundbank.h"
#include "speech.h"

static float mouseX = 0.0f , mouseY = 0.0f;
static float lastTrailX = -1000.0f , lastTrailY = -1000.0f;

static int quitWordTyped = 0;
static char typedWordBuffer[32];
static char typedQuitReason[64];

static unsigned long keysHandled = 0;
static unsigned long mouseMovesHandled = 0;
static unsigned long clicksHandled = 0;

static int greekMode = 0;
static int calmMode = 0;    /* after --minutes N : sleepy scene , no spawns */

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
      snprintf(typedQuitReason,sizeof(typedQuitReason),"magic word \"%s\" typed",magicWords[w]);
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

static void onLetter(char letter)   /* letter is 'A'..'Z' */
{
  sprites_spawnLetter(letter);
  int spoken = 0;
  if (greekMode) { spoken = speech_sayGreekLetter(sprites_latinToGreekIndex(letter)); }
            else { spoken = speech_sayCharacter(letter); }
  if (!spoken) { soundbank_playRandom(); }
}

static void onDigit(int digit)
{
  int count = (digit==0) ? 10 : digit;
  sprites_spawnLetter('0'+digit);
  const char * name = sprites_spawnCounted(count);   /* three ducks for '3' */
  if (!speech_sayNumber(count,greekMode))
     { if ( !( (name) && (soundbank_playNamed(name)) ) ) { soundbank_playRandom(); } }
}

static void onKey(unsigned long keysym)
{
  keysHandled++;

  //the typed-word quit detector keeps working even in calm mode
  if ( (keysym>=XK_a) && (keysym<=XK_z) ) { rememberTypedLetter('a' + (keysym-XK_a)); }
  if ( (keysym>=XK_A) && (keysym<=XK_Z) ) { rememberTypedLetter('a' + (keysym-XK_A)); }

  if (calmMode) { return; }

  int greekIndex = greekKeysymToIndex(keysym);
  if (greekIndex>=0)
  {
    sprites_spawnGreek(greekIndex);
    if (!speech_sayGreekLetter(greekIndex)) { soundbank_playRandom(); }
  } else
  if ( (keysym>=XK_a) && (keysym<=XK_z) ) { onLetter('A' + (keysym-XK_a)); } else
  if ( (keysym>=XK_A) && (keysym<=XK_Z) ) { onLetter('A' + (keysym-XK_A)); } else
  if ( (keysym>=XK_0) && (keysym<=XK_9) ) { onDigit(keysym-XK_0); }
      else
  {
    const char * name = sprites_spawnRandomTexture(-1,-1);
    //paired sound first : an emoji cow pops -> a cow moos
    if ( !( (name) && (soundbank_playNamed(name)) ) ) { soundbank_playRandom(); }
  }
}

static void onMouseMove(int x,int y)
{
  mouseMovesHandled++;
  mouseX = (float) x;
  mouseY = (float) y;
  if (calmMode) { return; }
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
  if (calmMode) { return; }
  //A little burst where the kid clicked
  int i;
  for (i=0; i<4; i++)
    { sprites_spawnRandomTexture(x+rand()%200-100 , y+rand()%200-100); }
}

static void appendSessionStats(double sessionSeconds,const char * exitReason)
{
  const char * home = getenv("HOME");
  if (home==0) { return; }
  char path[512];
  snprintf(path,sizeof(path),"%s/.babykeysmash_stats",home);
  FILE * f = fopen(path,"a");
  if (f==0) { return; }
  time_t now = time(0);
  char stamp[64];
  strftime(stamp,sizeof(stamp),"%Y-%m-%d %H:%M",localtime(&now));
  fprintf(f,"%s , duration %um%02us , %lu keystrokes , %lu mouse moves , %lu clicks , exit : %s\n",
          stamp,(unsigned int) (sessionSeconds/60),(unsigned int) sessionSeconds%60,
          keysHandled,mouseMovesHandled,clicksHandled,exitReason);
  fclose(f);
}

int main(int argc,const char ** argv)
{
  srand(time(0));

  int playMinutes = 0;   /* 0 = unlimited */
  int volumePercent = 100;
  int i;
  for (i=1; i<argc; i++)
  {
    if (strcmp(argv[i],"--greek")==0)                    { greekMode=1; } else
    if ( (strcmp(argv[i],"--minutes")==0) && (i+1<argc) ) { playMinutes = atoi(argv[++i]); } else
    if ( (strcmp(argv[i],"--volume")==0)  && (i+1<argc) ) { volumePercent = atoi(argv[++i]); }
  }
  if (volumePercent<0) { volumePercent=0; } if (volumePercent>100) { volumePercent=100; }

  //Allow launching from anywhere : fall back to the system installation
  //( see install.sh ) and then to the source tree for assets
  if (access("shaders/background.frag",R_OK)!=0)
  {
     if (access("/usr/share/babykeysmash/shaders/background.frag",R_OK)==0)
        {
          fprintf(stderr,"Using assets of system installation /usr/share/babykeysmash \n");
          if (chdir("/usr/share/babykeysmash")!=0) { fprintf(stderr,"Could not chdir to installation directory\n"); }
        }
          else
        {
          fprintf(stderr,"Assets not in current directory , falling back to %s \n",BKS_SOURCE_DIR);
          if (chdir(BKS_SOURCE_DIR)!=0) { fprintf(stderr,"Could not chdir to source directory\n"); }
        }
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

  //Backgrounds and webcam effects rotate every few minutes for variety
  struct shadertoyEffect * backgrounds[4];
  int numberOfBackgrounds = 0;
  backgrounds[numberOfBackgrounds] = shadertoy_load("shaders/background.frag");
  if (backgrounds[numberOfBackgrounds]==0) { fprintf(stderr,"Background shader is required , exiting\n"); runHookScript("scripts/on_exit.sh"); return 1; }
  numberOfBackgrounds++;
  backgrounds[numberOfBackgrounds] = shadertoy_load("shaders/sea.frag");
  if (backgrounds[numberOfBackgrounds]!=0) { numberOfBackgrounds++; }

  struct shadertoyEffect * webcamEffects[4];
  int numberOfWebcamEffects = 0;
  webcamEffects[numberOfWebcamEffects] = shadertoy_load("shaders/webcam.frag");
  if (webcamEffects[numberOfWebcamEffects]!=0) { numberOfWebcamEffects++; }
  webcamEffects[numberOfWebcamEffects] = shadertoy_load("shaders/kaleido.frag");
  if (webcamEffects[numberOfWebcamEffects]!=0) { numberOfWebcamEffects++; }

  struct shadertoyEffect * sleepyFx = shadertoy_load("shaders/sleepy.frag");

  sprites_init("textures",width,height);
  if (greekMode) { sprites_loadGreek("textures/greek"); }

  int haveWebcam = webcam_start(0);
  int haveAudio  = audio_start();
  soundbank_init("sounds");
  soundbank_setVolume(0.8f*volumePercent/100.0f);
  speech_init();
  speech_setVolume((float) volumePercent/100.0f);
  if (playMinutes>0) { fprintf(stderr,"Playtime limited to %u minutes , then the sleepy scene comes up\n",playMinutes); }

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

    //Playtime over -> calm sleepy scene , no more spawns or sounds
    if ( (playMinutes>0) && (t>playMinutes*60.0f) && (!calmMode) )
       { calmMode=1; fprintf(stderr,"Playtime is over , switching to the sleepy scene\n"); }
    float sleepyFade = 0.0f;
    if ( (calmMode) && (sleepyFx) )
       { sleepyFade = (t-playMinutes*60.0f)/10.0f; if (sleepyFade>1.0f) { sleepyFade=1.0f; } }

    if (sleepyFade<1.0f)
    {
      //Backgrounds rotate every few minutes with a short crossfade
      #define ROTATION_SECONDS 180.0f
      int slot = (int) (t/ROTATION_SECONDS);
      float phase = fmodf(t,ROTATION_SECONDS);
      int backgroundIndex = slot % numberOfBackgrounds;

      shadertoy_draw(backgrounds[( (slot>0) && (phase<3.0f) ) ? (slot-1)%numberOfBackgrounds : backgroundIndex],
                     t,frame,(float) width,(float) height,mouseX,mouseY,audio_texture(),0);
      if ( (slot>0) && (phase<3.0f) )
      {
        glEnable(GL_BLEND);
        glBlendColor(0.0f,0.0f,0.0f,phase/3.0f);
        glBlendFunc(GL_CONSTANT_ALPHA,GL_ONE_MINUS_CONSTANT_ALPHA);
        shadertoy_draw(backgrounds[backgroundIndex],t,frame,(float) width,(float) height,
                       mouseX,mouseY,audio_texture(),0);
        glDisable(GL_BLEND);
      }

      //Webcam effect ( also rotating ) blended on top with a breathing mix factor
      if ( (webcamReady) && (numberOfWebcamEffects>0) && (!calmMode) )
      {
        struct shadertoyEffect * camFx = webcamEffects[slot % numberOfWebcamEffects];
        float mix = 0.45f + 0.30f * (float) sin(now*0.15);
        glEnable(GL_BLEND);
        glBlendColor(0.0f,0.0f,0.0f,mix);
        glBlendFunc(GL_CONSTANT_ALPHA,GL_ONE_MINUS_CONSTANT_ALPHA);
        shadertoy_draw(camFx,t,frame,(float) width,(float) height,
                       mouseX,mouseY,webcam_texture(),audio_texture());
        glDisable(GL_BLEND);
      }
    }

    if (sleepyFade>0.0f)
    {
      if (sleepyFade<1.0f)
      {
        glEnable(GL_BLEND);
        glBlendColor(0.0f,0.0f,0.0f,sleepyFade);
        glBlendFunc(GL_CONSTANT_ALPHA,GL_ONE_MINUS_CONSTANT_ALPHA);
      }
      shadertoy_draw(sleepyFx,t,frame,(float) width,(float) height,
                     mouseX,mouseY,audio_texture(),0);
      if (sleepyFade<1.0f) { glDisable(GL_BLEND); }
    }

    //Waving in front of the camera sprinkles sparkles where the motion is
    if (!calmMode)
    {
      static double lastMotionSpawn = 0.0;
      float motionX,motionY;
      if ( (now-lastMotionSpawn>0.12) && (webcam_getMotion(&motionX,&motionY)) )
      {
        sprites_spawnTrail(motionX*width,motionY*height);
        lastMotionSpawn = now;
      }
    }

    sprites_updateAndDraw(dt,mouseX,mouseY);

    babywin_swap();
    frame++;

    //Modest frame cap so a driver without vsync does not spin at 100% CPU
    double frameTime = getTimeSeconds()-now;
    if (frameTime<0.016) { usleep((unsigned int)((0.016-frameTime)*1000000.0)); }
  }

  const char * exitReason = (quitWordTyped) ? typedQuitReason : babywin_exitReason();
  fprintf(stderr,"Shutting down.. exit triggered by : %s \n",exitReason);
  fprintf(stderr,"Session smash report : %lu keystrokes , %lu mouse moves , %lu clicks , %lu events total :)\n",
          keysHandled,mouseMovesHandled,clicksHandled,
          keysHandled+mouseMovesHandled+clicksHandled);
  appendSessionStats(getTimeSeconds()-startTime,exitReason);
  webcam_stop();
  audio_stop();
  soundbank_close();
  sprites_close();
  for (i=0; i<numberOfBackgrounds; i++)   { shadertoy_unload(backgrounds[i]);   }
  for (i=0; i<numberOfWebcamEffects; i++) { shadertoy_unload(webcamEffects[i]); }
  shadertoy_unload(sleepyFx);
  babywin_close();
  runHookScript("scripts/on_exit.sh");
  return 0;
}

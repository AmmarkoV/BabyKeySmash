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
#include <glob.h>
#include <X11/X.h>

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

/* Shader options are discovered by filename : shaders/background_*.frag ,
   shaders/webcam_*.frag , shaders/sleepy_*.frag . Drop a new ShaderToy
   style file in and it joins the rotation ; pin one with --background NAME
   etc. or cycle live with Ctrl+Shift+B / Ctrl+Shift+W */
#define MAX_EFFECT_OPTIONS 16
struct effectOption
{
  struct shadertoyEffect * fx;
  char name[64];
};
static struct effectOption backgrounds[MAX_EFFECT_OPTIONS];
static struct effectOption webcamEffects[MAX_EFFECT_OPTIONS];
static struct effectOption sleepyScenes[MAX_EFFECT_OPTIONS];
static int numberOfBackgrounds = 0;
static int numberOfWebcamEffects = 0;
static int numberOfSleepyScenes = 0;
static int backgroundCycleOffset = 0;
static int webcamCycleOffset = 0;

static void effectNameFromPath(const char * path,const char * prefix,char * name,unsigned int nameSize)
{
  const char * base = strrchr(path,'/');
  base = (base) ? base+1 : path;
  snprintf(name,nameSize,"%s",base+strlen(prefix));
  char * dot = strrchr(name,'.');
  if (dot) { *dot=0; }
}

static int discoverEffects(const char * pattern,const char * prefix,const char * pinnedName,
                           struct effectOption * out,int maxOut)
{
  glob_t files;
  memset(&files,0,sizeof(files));
  if (glob(pattern,0,0,&files)!=0) { return 0; }

  int n=0;
  unsigned int i;
  for (i=0; (i<files.gl_pathc) && (n<maxOut); i++)
  {
    char name[64];
    effectNameFromPath(files.gl_pathv[i],prefix,name,sizeof(name));
    if ( (pinnedName) && (strcmp(name,pinnedName)!=0) ) { continue; }
    struct shadertoyEffect * fx = shadertoy_load(files.gl_pathv[i]);
    if (fx)
    {
      out[n].fx = fx;
      snprintf(out[n].name,sizeof(out[n].name),"%s",name);
      n++;
    }
  }
  globfree(&files);
  return n;
}

static void listShaderOptions(const char * label,const char * pattern,const char * prefix)
{
  printf("%s :",label);
  glob_t files;
  memset(&files,0,sizeof(files));
  if (glob(pattern,0,0,&files)==0)
  {
    unsigned int i;
    for (i=0; i<files.gl_pathc; i++)
    {
      char name[64];
      effectNameFromPath(files.gl_pathv[i],prefix,name,sizeof(name));
      printf(" %s",name);
    }
    globfree(&files);
  }
  printf("\n");
}

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

static void onKey(unsigned long keysym,unsigned int modifiers)
{
  keysHandled++;

  //the typed-word quit detector keeps working even in calm mode
  if ( (keysym>=XK_a) && (keysym<=XK_z) ) { rememberTypedLetter('a' + (keysym-XK_a)); }
  if ( (keysym>=XK_A) && (keysym<=XK_Z) ) { rememberTypedLetter('a' + (keysym-XK_A)); }

  //Parent shortcuts to switch effects live , Ctrl+Shift is toddler proof
  if ( (modifiers & ControlMask) && (modifiers & ShiftMask) )
  {
    if ( (keysym==XK_b) || (keysym==XK_B) )
    {
      backgroundCycleOffset++;
      fprintf(stderr,"Background effect switched\n");
      return;
    }
    if ( (keysym==XK_w) || (keysym==XK_W) )
    {
      webcamCycleOffset++;
      fprintf(stderr,"Webcam effect switched\n");
      return;
    }
  }

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

/* Lunar phase of right now : 0=new , 0.25=first quarter , 0.5=full ,
   0.75=last quarter . Counted from the new moon of 2000-01-06 18:14 UTC
   over the mean synodic month , which stays within a few hours for decades */
static float computeMoonPhase()
{
  double daysSinceNewMoon = difftime(time(0),(time_t) 947182440) / 86400.0;
  double phase = fmod(daysSinceNewMoon / 29.530588853 , 1.0);
  if (phase<0.0) { phase += 1.0; }
  return (float) phase;
}

static const char * moonPhaseName(float phase)
{
  if (phase<0.03) { return "new moon"; }
  if (phase<0.22) { return "waxing crescent"; }
  if (phase<0.28) { return "first quarter"; }
  if (phase<0.47) { return "waxing gibbous"; }
  if (phase<0.53) { return "full moon"; }
  if (phase<0.72) { return "waning gibbous"; }
  if (phase<0.78) { return "last quarter"; }
  if (phase<0.97) { return "waning crescent"; }
  return "new moon";
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
  int listShaders = 0;
  int speechEnabled = 0;  /* speech is opt in through --speech */
  int simpleBackground = 0;
  const char * pinnedBackground = 0;  /* 0 = use every option and rotate */
  const char * pinnedWebcam = 0;
  const char * pinnedSleepy = 0;
  const char * speechVoice = 0;
  float moonPhaseOverride = -1.0f;   /* <0 = use tonight's real lunar phase */
  int i;
  for (i=1; i<argc; i++)
  {
    if (strcmp(argv[i],"--greek")==0)                        { greekMode=1; } else
    if (strcmp(argv[i],"--speech")==0)                       { speechEnabled=1; } else
    if (strcmp(argv[i],"--simplebg")==0)                     { simpleBackground=1; } else
    if (strcmp(argv[i],"--list-shaders")==0)                 { listShaders=1; } else
    if ( (strcmp(argv[i],"--minutes")==0)    && (i+1<argc) ) { playMinutes = atoi(argv[++i]); } else
    if ( (strcmp(argv[i],"--volume")==0)     && (i+1<argc) ) { volumePercent = atoi(argv[++i]); } else
    if ( (strcmp(argv[i],"--background")==0) && (i+1<argc) ) { pinnedBackground = argv[++i]; } else
    if ( (strcmp(argv[i],"--webcam")==0)     && (i+1<argc) ) { pinnedWebcam = argv[++i]; } else
    if ( (strcmp(argv[i],"--sleepy")==0)     && (i+1<argc) ) { pinnedSleepy = argv[++i]; } else
    if ( (strcmp(argv[i],"--moon-phase")==0) && (i+1<argc) ) { moonPhaseOverride = (float) atof(argv[++i]); } else
    if ( (strcmp(argv[i],"--voice")==0)      && (i+1<argc) ) { speechVoice = argv[++i]; speechEnabled=1; } else
    if ( (strcmp(argv[i],"--help")==0) || (strcmp(argv[i],"-h")==0) )
    {
      printf("Usage : babykeysmash [options]\n"
             "  --greek               Greek letters ( spoken with Greek names if --speech )\n"
             "  --speech              speak letters and numbers aloud through espeak-ng\n"
             "                        ( off by default , only sound effects are played )\n"
             "  --minutes N           after N minutes fade into the calm sleepy scene\n"
             "  --volume 0..100       volume of sound effects and speech\n"
             "  --simplebg            calm deep blue to black gradient and no webcam\n"
             "                        effect , for winding down or a dark room\n"
             "  --background NAME     use only this background ( see --list-shaders )\n"
             "  --webcam NAME         use only this webcam effect\n"
             "  --sleepy NAME         use this end of playtime scene\n"
             "  --moon-phase 0..1     force the moon phase of the sleepy scene\n"
             "                        ( 0=new , 0.5=full , default = tonight's real one )\n"
             "  --voice NAME          espeak-ng voice variant , e.g. f3 , m5 , whisper\n"
             "                        ( implies --speech )\n"
             "  --list-shaders        show the available shader and voice options\n"
             "Effects not pinned rotate every 3 minutes , a parent can also switch\n"
             "them live with Ctrl+Shift+B ( background ) and Ctrl+Shift+W ( webcam )\n");
      return 0;
    }
  }
  if (volumePercent<0) { volumePercent=0; } if (volumePercent>100) { volumePercent=100; }

  //--simplebg is a preset : the calm gradient and no busy webcam overlay on
  //top of it . An explicit --background still wins over it
  if ( (simpleBackground) && (pinnedBackground==0) ) { pinnedBackground = "calm"; }

  //Allow launching from anywhere : fall back to the system installation
  //( see install.sh ) and then to the source tree for assets
  if (access("shaders",R_OK)!=0)
  {
     if (access("/usr/share/babykeysmash/shaders",R_OK)==0)
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

  if (listShaders)
  {
    listShaderOptions("Backgrounds   (--background)","shaders/background_*.frag","background_");
    listShaderOptions("Webcam effects(--webcam)    ","shaders/webcam_*.frag"    ,"webcam_");
    listShaderOptions("Sleepy scenes (--sleepy)    ","shaders/sleepy_*.frag"    ,"sleepy_");
    speech_listVoices();
    return 0;
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

  //Every shaders/<slot>_*.frag is an option , unpinned slots rotate
  numberOfBackgrounds = discoverEffects("shaders/background_*.frag","background_",pinnedBackground,
                                        backgrounds,MAX_EFFECT_OPTIONS);
  if ( (numberOfBackgrounds==0) && (pinnedBackground) )
  {
    fprintf(stderr,"No background named %s , falling back to all of them ( --list-shaders shows the options )\n",pinnedBackground);
    numberOfBackgrounds = discoverEffects("shaders/background_*.frag","background_",0,
                                          backgrounds,MAX_EFFECT_OPTIONS);
  }
  if (numberOfBackgrounds==0)
     { fprintf(stderr,"At least one background shader is required , exiting\n"); runHookScript("scripts/on_exit.sh"); return 1; }

  //In simple background mode the webcam effect is left out entirely , it
  //would drown out the calm gradient it is blended over
  if (!simpleBackground)
  {
    numberOfWebcamEffects = discoverEffects("shaders/webcam_*.frag","webcam_",pinnedWebcam,
                                            webcamEffects,MAX_EFFECT_OPTIONS);
    if ( (numberOfWebcamEffects==0) && (pinnedWebcam) )
    {
      fprintf(stderr,"No webcam effect named %s , falling back to all of them\n",pinnedWebcam);
      numberOfWebcamEffects = discoverEffects("shaders/webcam_*.frag","webcam_",0,
                                              webcamEffects,MAX_EFFECT_OPTIONS);
    }
  }
    else
  { fprintf(stderr,"Simple calm background , the webcam effect stays off\n"); }

  numberOfSleepyScenes = discoverEffects("shaders/sleepy_*.frag","sleepy_",pinnedSleepy,
                                         sleepyScenes,MAX_EFFECT_OPTIONS);
  if ( (numberOfSleepyScenes==0) && (pinnedSleepy) )
     { numberOfSleepyScenes = discoverEffects("shaders/sleepy_*.frag","sleepy_",0,sleepyScenes,MAX_EFFECT_OPTIONS); }
  struct shadertoyEffect * sleepyFx = (numberOfSleepyScenes>0) ? sleepyScenes[0].fx : 0;

  //The sleepy scene shows a photograph of the Moon lit with tonight's phase
  unsigned int moonTexture = sprites_loadImageTexture("textures/moon.jpg");
  float moonPhase = (moonPhaseOverride>=0.0f) ? moonPhaseOverride : computeMoonPhase();
  shadertoy_setMoonPhase(moonPhase);
  fprintf(stderr,"Moon phase is %0.2f ( %s )\n",moonPhase,moonPhaseName(moonPhase));

  fprintf(stderr,"Effects ready : %u backgrounds , %u webcam effects , %u sleepy scenes\n",
          numberOfBackgrounds,numberOfWebcamEffects,numberOfSleepyScenes);

  sprites_init("textures",width,height);
  if (greekMode) { sprites_loadGreek("textures/greek"); }

  int haveWebcam = webcam_start(0);
  int haveAudio  = audio_start();
  soundbank_init("sounds");
  soundbank_setVolume(0.8f*volumePercent/100.0f);
  //Speech is opt in : without --speech every key just plays a sound effect
  if (speechEnabled)
  {
    speech_init();
    speech_setVolume((float) volumePercent/100.0f);
    speech_setVoice(speechVoice);
  }
    else
  { fprintf(stderr,"Speech is off , add --speech to hear letters and numbers spoken\n"); }
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
    {
      calmMode=1;
      fprintf(stderr,"Playtime is over , switching to the sleepy scene\n");
      runHookScript("scripts/on_sleep.sh");
    }
    float sleepyFade = 0.0f;
    if ( (calmMode) && (sleepyFx) )
       { sleepyFade = (t-playMinutes*60.0f)/10.0f; if (sleepyFade>1.0f) { sleepyFade=1.0f; } }

    if (sleepyFade<1.0f)
    {
      //Backgrounds rotate every few minutes ( plus any live Ctrl+Shift+B
      //switches ) with a short crossfade
      #define ROTATION_SECONDS 180.0f
      int slot = (int) (t/ROTATION_SECONDS) + backgroundCycleOffset;
      float phase = fmodf(t,ROTATION_SECONDS);
      int backgroundIndex = slot % numberOfBackgrounds;
      int crossfading = ( (slot>0) && (phase<3.0f) && (numberOfBackgrounds>1) );

      shadertoy_draw(backgrounds[crossfading ? (slot-1)%numberOfBackgrounds : backgroundIndex].fx,
                     t,frame,(float) width,(float) height,mouseX,mouseY,audio_texture(),0);
      if (crossfading)
      {
        glEnable(GL_BLEND);
        glBlendColor(0.0f,0.0f,0.0f,phase/3.0f);
        glBlendFunc(GL_CONSTANT_ALPHA,GL_ONE_MINUS_CONSTANT_ALPHA);
        shadertoy_draw(backgrounds[backgroundIndex].fx,t,frame,(float) width,(float) height,
                       mouseX,mouseY,audio_texture(),0);
        glDisable(GL_BLEND);
      }

      //Webcam effect ( also rotating ) blended on top with a breathing mix factor
      if ( (webcamReady) && (numberOfWebcamEffects>0) && (!calmMode) )
      {
        struct shadertoyEffect * camFx =
          webcamEffects[((int) (t/ROTATION_SECONDS) + webcamCycleOffset) % numberOfWebcamEffects].fx;
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
                     mouseX,mouseY,audio_texture(),moonTexture);
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
  for (i=0; i<numberOfBackgrounds; i++)   { shadertoy_unload(backgrounds[i].fx);   }
  for (i=0; i<numberOfWebcamEffects; i++) { shadertoy_unload(webcamEffects[i].fx); }
  for (i=0; i<numberOfSleepyScenes; i++)  { shadertoy_unload(sleepyScenes[i].fx);  }
  if (moonTexture) { glDeleteTextures(1,&moonTexture); }
  babywin_close();
  runHookScript("scripts/on_exit.sh");
  return 0;
}

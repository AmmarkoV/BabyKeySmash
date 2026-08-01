/** @file soundbank.c
 *  @brief see soundbank.h , plain C . The ogg files are guaranteed
 *         44100 Hz / stereo by tools/import_sounds.sh and are decoded to
 *         memory at startup with libvorbisfile ( ogg keeps the repository
 *         ~15x smaller than wav )
 *  @author Ammar Qammaz (AmmarkoV)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <time.h>
#include <glob.h>
#include <pthread.h>
#include <alsa/asoundlib.h>
#include <vorbis/vorbisfile.h>

#include "soundbank.h"

#define SOUNDBANK_MAX_SOUNDS 128
#define SOUNDBANK_MAX_VOICES 2      /* clips playing simultaneously */
#define SOUNDBANK_COOLDOWN 0.35     /* seconds between triggers */
#define SOUNDBANK_SAMPLE_RATE 44100
#define SOUNDBANK_CHANNELS 2
#define SOUNDBANK_CHUNK_FRAMES 512
static float soundbankVolume = 0.8f;

struct soundClip
{
  short * samples;             /* interleaved stereo */
  unsigned int frames;
  char name[64];               /* lowercase basename without extension */
};

struct voice
{
  int active;
  int soundIndex;
  unsigned int position;       /* in frames */
};

static struct soundClip sounds[SOUNDBANK_MAX_SOUNDS];
static int numberOfSounds = 0;

static struct voice voices[SOUNDBANK_MAX_VOICES];
static pthread_mutex_t voiceLock = PTHREAD_MUTEX_INITIALIZER;
static double lastTriggerTime = 0.0;

static pthread_t playbackThread;
static volatile int running = 0;
static int active = 0;

static double getTimeSeconds()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC,&ts);
  return (double) ts.tv_sec + (double) ts.tv_nsec / 1000000000.0;
}

/* Decode a whole ogg/vorbis file ( fixed format produced by
   tools/import_sounds.sh ) into an interleaved S16 buffer */
static int loadOgg(const char * filename,struct soundClip * clip)
{
  OggVorbis_File vf;
  if (ov_fopen(filename,&vf)<0)
     { fprintf(stderr,"Could not open sound %s \n",filename); return 0; }

  vorbis_info * vi = ov_info(&vf,-1);
  if ( (vi==0) || (vi->channels!=SOUNDBANK_CHANNELS) || (vi->rate!=SOUNDBANK_SAMPLE_RATE) )
  {
    fprintf(stderr,"Rejected sound %s , rerun tools/import_sounds.sh to fix its format\n",filename);
    ov_clear(&vf);
    return 0;
  }

  long totalFrames = (long) ov_pcm_total(&vf,-1);
  if (totalFrames<=0) { ov_clear(&vf); return 0; }

  unsigned int totalBytes = totalFrames * SOUNDBANK_CHANNELS * sizeof(short);
  clip->samples = (short *) malloc(totalBytes);
  if (clip->samples==0) { ov_clear(&vf); return 0; }

  unsigned int decoded = 0;
  int section = 0;
  while (decoded<totalBytes)
  {
    long got = ov_read(&vf,((char *) clip->samples)+decoded,totalBytes-decoded,
                       0 /*little endian*/,2 /*16bit*/,1 /*signed*/,&section);
    if (got<=0) { break; }
    decoded += got;
  }
  ov_clear(&vf);

  if (decoded==0) { free(clip->samples); clip->samples=0; return 0; }
  clip->frames = decoded / (SOUNDBANK_CHANNELS*sizeof(short));
  return 1;
}

static void * playbackLoop(void * arg)
{
  snd_pcm_t * pcm = (snd_pcm_t *) arg;
  int mix[SOUNDBANK_CHUNK_FRAMES*SOUNDBANK_CHANNELS];
  short out[SOUNDBANK_CHUNK_FRAMES*SOUNDBANK_CHANNELS];

  while (running)
  {
    memset(mix,0,sizeof(mix));

    pthread_mutex_lock(&voiceLock);
    int v;
    for (v=0; v<SOUNDBANK_MAX_VOICES; v++)
    {
      if (!voices[v].active) { continue; }
      struct soundClip * clip = &sounds[voices[v].soundIndex];
      unsigned int left = clip->frames - voices[v].position;
      unsigned int todo = (left<SOUNDBANK_CHUNK_FRAMES) ? left : SOUNDBANK_CHUNK_FRAMES;
      const short * src = clip->samples + voices[v].position*SOUNDBANK_CHANNELS;
      unsigned int s;
      for (s=0; s<todo*SOUNDBANK_CHANNELS; s++)
        { mix[s] += (int) (src[s]*soundbankVolume); }
      voices[v].position += todo;
      if (voices[v].position>=clip->frames) { voices[v].active=0; }
    }
    pthread_mutex_unlock(&voiceLock);

    unsigned int s;
    for (s=0; s<SOUNDBANK_CHUNK_FRAMES*SOUNDBANK_CHANNELS; s++)
    {
      int value = mix[s];
      if (value> 32767) { value= 32767; }
      if (value<-32768) { value=-32768; }
      out[s] = (short) value;
    }

    snd_pcm_sframes_t wrote = snd_pcm_writei(pcm,out,SOUNDBANK_CHUNK_FRAMES);
    if (wrote<0) { snd_pcm_recover(pcm,wrote,1); }
  }

  snd_pcm_close(pcm);
  return 0;
}

int soundbank_init(const char * directory)
{
  char pattern[512];
  snprintf(pattern,sizeof(pattern),"%s/*.ogg",directory);

  glob_t files;
  memset(&files,0,sizeof(files));
  if (glob(pattern,0,0,&files)!=0)
  {
    fprintf(stderr,"No sounds in %s , run tools/import_sounds.sh , keys stay silent\n",directory);
    return 0;
  }

  unsigned int i;
  for (i=0; i<files.gl_pathc; i++)
  {
    if (numberOfSounds>=SOUNDBANK_MAX_SOUNDS) { break; }
    if (loadOgg(files.gl_pathv[i],&sounds[numberOfSounds]))
    {
      //remember the lowercase basename ( without .ogg ) for soundbank_playNamed
      const char * base = strrchr(files.gl_pathv[i],'/');
      base = (base) ? base+1 : files.gl_pathv[i];
      struct soundClip * clip = &sounds[numberOfSounds];
      unsigned int c;
      for (c=0; (c<sizeof(clip->name)-1) && (base[c]!=0) && (base[c]!='.'); c++)
        { clip->name[c] = tolower(base[c]); }
      clip->name[c]=0;
      numberOfSounds++;
    }
  }
  globfree(&files);

  if (numberOfSounds==0) { fprintf(stderr,"No usable sounds in %s , keys stay silent\n",directory); return 0; }

  snd_pcm_t * pcm = 0;
  if (snd_pcm_open(&pcm,"default",SND_PCM_STREAM_PLAYBACK,0) < 0)
  {
    fprintf(stderr,"Could not open ALSA playback device , keys stay silent\n");
    return 0;
  }
  if (snd_pcm_set_params(pcm,
                         SND_PCM_FORMAT_S16_LE,
                         SND_PCM_ACCESS_RW_INTERLEAVED,
                         SOUNDBANK_CHANNELS,
                         SOUNDBANK_SAMPLE_RATE,
                         1 /*allow resampling*/,
                         100000 /*0.1s latency*/) < 0)
  {
    fprintf(stderr,"Could not configure ALSA playback device , keys stay silent\n");
    snd_pcm_close(pcm);
    return 0;
  }

  running = 1;
  if (pthread_create(&playbackThread,0,playbackLoop,pcm)!=0)
  {
    fprintf(stderr,"Could not start sound playback thread\n");
    running = 0;
    snd_pcm_close(pcm);
    return 0;
  }

  active = 1;
  fprintf(stderr,"Loaded %u sounds from %s \n",numberOfSounds,directory);
  return numberOfSounds;
}

static void triggerSound(int soundIndex)
{
  double now = getTimeSeconds();

  pthread_mutex_lock(&voiceLock);
  if (now-lastTriggerTime >= SOUNDBANK_COOLDOWN)
  {
    int v;
    for (v=0; v<SOUNDBANK_MAX_VOICES; v++)
    {
      if (!voices[v].active)
      {
        voices[v].soundIndex = soundIndex;
        voices[v].position = 0;
        voices[v].active = 1;
        lastTriggerTime = now;
        break;
      }
    }
  }
  pthread_mutex_unlock(&voiceLock);
}

void soundbank_setVolume(float volume)
{
  soundbankVolume = volume;
}

void soundbank_playRandom()
{
  if (!active) { return; }
  triggerSound(rand()%numberOfSounds);
}

int soundbank_playNamed(const char * name)
{
  if ( (!active) || (name==0) || (name[0]==0) ) { return 0; }

  unsigned int nameLength = strlen(name);
  int matches[SOUNDBANK_MAX_SOUNDS];
  int numberOfMatches = 0;
  int i;
  for (i=0; i<numberOfSounds; i++)
  {
    if (strncasecmp(sounds[i].name,name,nameLength)==0)
      { matches[numberOfMatches++] = i; }
  }
  if (numberOfMatches==0) { return 0; }

  triggerSound(matches[rand()%numberOfMatches]);
  return 1;
}

void soundbank_close()
{
  if (active)
  {
    running = 0;
    pthread_join(playbackThread,0);
    active = 0;
  }
  int i;
  for (i=0; i<numberOfSounds; i++) { free(sounds[i].samples); }
  numberOfSounds = 0;
}

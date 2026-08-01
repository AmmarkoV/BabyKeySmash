/** @file audio_alsa.c
 *  @brief see audio_alsa.h , plain C , own tiny radix-2 FFT , capture on a
 *         pthread , GL upload on the main thread
 *  @author Ammar Qammaz (AmmarkoV)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <alsa/asoundlib.h>

#include <GL/glew.h>
#include <GL/gl.h>

#include "audio_alsa.h"

#define FFT_SIZE 1024          /* samples per FFT window , power of two */
#define SPECTRUM_BINS 512      /* = FFT_SIZE/2 , also the texture width */
#define RING_SIZE 4096

static short ringBuffer[RING_SIZE];
static unsigned int ringWritePosition = 0;
static pthread_mutex_t ringLock = PTHREAD_MUTEX_INITIALIZER;

static pthread_t captureThread;
static volatile int running = 0;
static int active = 0;

static unsigned int sampleRate = 44100;

static GLuint audioTex = 0;
static float smoothedSpectrum[SPECTRUM_BINS];
static float smoothedLevel = 0.0f;

static void * captureLoop(void * arg)
{
  snd_pcm_t * pcm = (snd_pcm_t *) arg;
  short chunk[512];
  while (running)
  {
    snd_pcm_sframes_t got = snd_pcm_readi(pcm,chunk,512);
    if (got<0)
    {
      snd_pcm_recover(pcm,got,1);
      continue;
    }
    pthread_mutex_lock(&ringLock);
    int i;
    for (i=0; i<got; i++)
    {
      ringBuffer[ringWritePosition] = chunk[i];
      ringWritePosition = (ringWritePosition+1) % RING_SIZE;
    }
    pthread_mutex_unlock(&ringLock);
  }
  snd_pcm_close(pcm);
  return 0;
}

int audio_start()
{
  snd_pcm_t * pcm = 0;
  if (snd_pcm_open(&pcm,"default",SND_PCM_STREAM_CAPTURE,0) < 0)
  {
    fprintf(stderr,"Could not open default ALSA capture device , microphone effect disabled\n");
    return 0;
  }

  if (snd_pcm_set_params(pcm,
                         SND_PCM_FORMAT_S16_LE,
                         SND_PCM_ACCESS_RW_INTERLEAVED,
                         1 /*channels*/,
                         sampleRate,
                         1 /*allow resampling*/,
                         100000 /*0.1s latency*/) < 0)
  {
    fprintf(stderr,"Could not configure ALSA capture device , microphone effect disabled\n");
    snd_pcm_close(pcm);
    return 0;
  }

  running = 1;
  if (pthread_create(&captureThread,0,captureLoop,pcm)!=0)
  {
    fprintf(stderr,"Could not start audio thread\n");
    running = 0;
    snd_pcm_close(pcm);
    return 0;
  }
  active = 1;
  fprintf(stderr,"Microphone capture started\n");
  return 1;
}

/* In-place iterative radix-2 FFT */
static void fft(float * re,float * im,int n)
{
  int i,j,k,len;
  //bit reversal permutation
  j=0;
  for (i=1; i<n; i++)
  {
    int bit = n>>1;
    for (; j & bit; bit>>=1) { j ^= bit; }
    j |= bit;
    if (i<j)
    {
      float t;
      t=re[i]; re[i]=re[j]; re[j]=t;
      t=im[i]; im[i]=im[j]; im[j]=t;
    }
  }
  for (len=2; len<=n; len<<=1)
  {
    float ang = -2.0f*M_PI/len;
    float wRe = cosf(ang) , wIm = sinf(ang);
    for (i=0; i<n; i+=len)
    {
      float curRe=1.0f , curIm=0.0f;
      for (k=0; k<len/2; k++)
      {
        float uRe = re[i+k]          , uIm = im[i+k];
        float vRe = re[i+k+len/2]*curRe - im[i+k+len/2]*curIm;
        float vIm = re[i+k+len/2]*curIm + im[i+k+len/2]*curRe;
        re[i+k]       = uRe+vRe;  im[i+k]       = uIm+vIm;
        re[i+k+len/2] = uRe-vRe;  im[i+k+len/2] = uIm-vIm;
        float nextRe = curRe*wRe - curIm*wIm;
        curIm = curRe*wIm + curIm*wRe;
        curRe = nextRe;
      }
    }
  }
}

int audio_update()
{
  if (!active) { return 0; }

  float samples[FFT_SIZE];
  pthread_mutex_lock(&ringLock);
  unsigned int start = (ringWritePosition + RING_SIZE - FFT_SIZE) % RING_SIZE;
  int i;
  for (i=0; i<FFT_SIZE; i++)
    { samples[i] = ringBuffer[(start+i)%RING_SIZE] / 32768.0f; }
  pthread_mutex_unlock(&ringLock);

  //overall loudness ( RMS , smoothed )
  float rms = 0.0f;
  for (i=0; i<FFT_SIZE; i++) { rms += samples[i]*samples[i]; }
  rms = sqrtf(rms/FFT_SIZE);
  float level = rms*6.0f; if (level>1.0f) { level=1.0f; }
  smoothedLevel = 0.9f*smoothedLevel + 0.1f*level;

  float re[FFT_SIZE],im[FFT_SIZE];
  for (i=0; i<FFT_SIZE; i++)
  {
    //Hann window
    float w = 0.5f * (1.0f - cosf(2.0f*M_PI*i/(FFT_SIZE-1)));
    re[i] = samples[i]*w;
    im[i] = 0.0f;
  }
  fft(re,im,FFT_SIZE);

  unsigned char pixels[2*SPECTRUM_BINS];
  for (i=0; i<SPECTRUM_BINS; i++)
  {
    float mag = sqrtf(re[i]*re[i]+im[i]*im[i]) / (FFT_SIZE/4);
    //dB-ish mapping similar to the shadertoy FFT texture
    float dB = 20.0f*log10f(mag+1e-6f);
    float v = (dB+60.0f)/60.0f;
    if (v<0.0f) { v=0.0f; } if (v>1.0f) { v=1.0f; }
    //peaks appear instantly and decay smoothly
    if (v>smoothedSpectrum[i]) { smoothedSpectrum[i]=v; }
                          else { smoothedSpectrum[i]*=0.92f; }
    pixels[i] = (unsigned char) (smoothedSpectrum[i]*255.0f); /* row 0 : spectrum */
  }
  for (i=0; i<SPECTRUM_BINS; i++)
  {
    float s = samples[FFT_SIZE-SPECTRUM_BINS+i];
    pixels[SPECTRUM_BINS+i] = (unsigned char) ((0.5f+0.5f*s)*255.0f); /* row 1 : waveform */
  }

  if (audioTex==0)
  {
    glGenTextures(1,&audioTex);
    glBindTexture(GL_TEXTURE_2D,audioTex);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexImage2D(GL_TEXTURE_2D,0,GL_R8,SPECTRUM_BINS,2,0,GL_RED,GL_UNSIGNED_BYTE,pixels);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  }
    else
  {
    glBindTexture(GL_TEXTURE_2D,audioTex);
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,SPECTRUM_BINS,2,GL_RED,GL_UNSIGNED_BYTE,pixels);
  }
  glBindTexture(GL_TEXTURE_2D,0);
  return 1;
}

unsigned int audio_texture() { return audioTex; }

float audio_level() { return smoothedLevel; }

void audio_stop()
{
  if (!active) { return; }
  running = 0;
  pthread_join(captureThread,0);
  if (audioTex) { glDeleteTextures(1,&audioTex); audioTex=0; }
  active = 0;
}

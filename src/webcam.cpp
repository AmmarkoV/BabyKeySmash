/** @file webcam.cpp
 *  @brief see webcam.h , C-style code , capture runs on its own pthread ,
 *         GL upload happens on the main thread in webcam_update()
 *  @author Ammar Qammaz (AmmarkoV)
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <GL/glew.h>
#include <GL/gl.h>

#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>

#include "webcam.h"

static pthread_t captureThread;
static pthread_mutex_t frameLock = PTHREAD_MUTEX_INITIALIZER;

static cv::Mat sharedFrame;      /* RGB , flipped for GL , mirrored for selfie view */
static int haveNewFrame = 0;
static volatile int running = 0;
static int active = 0;

static GLuint camTexture = 0;
static int texWidth = 0 , texHeight = 0;

static void * captureLoop(void * arg)
{
  cv::VideoCapture * cap = (cv::VideoCapture *) arg;
  cv::Mat frame,rgb;
  while (running)
  {
    if (!cap->read(frame)) { usleep(100*1000); continue; }
    cv::cvtColor(frame,rgb,cv::COLOR_BGR2RGB);
    //flip both axes : vertical for the GL texture origin , horizontal for mirror/selfie view
    cv::flip(rgb,rgb,-1);

    pthread_mutex_lock(&frameLock);
    rgb.copyTo(sharedFrame);
    haveNewFrame = 1;
    pthread_mutex_unlock(&frameLock);
  }
  cap->release();
  delete cap;
  return 0;
}

int webcam_start(int deviceIndex)
{
  cv::VideoCapture * cap = new cv::VideoCapture(deviceIndex);
  if (!cap->isOpened())
  {
    fprintf(stderr,"No webcam found on device %u , webcam effect disabled\n",deviceIndex);
    delete cap;
    return 0;
  }
  cap->set(cv::CAP_PROP_FRAME_WIDTH ,640);
  cap->set(cv::CAP_PROP_FRAME_HEIGHT,480);

  running = 1;
  if (pthread_create(&captureThread,0,captureLoop,cap)!=0)
  {
    fprintf(stderr,"Could not start webcam thread\n");
    running = 0;
    cap->release();
    delete cap;
    return 0;
  }
  active = 1;
  fprintf(stderr,"Webcam capture started\n");
  return 1;
}

int webcam_update()
{
  if (!active) { return 0; }

  pthread_mutex_lock(&frameLock);
  if (haveNewFrame)
  {
    if (camTexture==0)
    {
      glGenTextures(1,&camTexture);
      glBindTexture(GL_TEXTURE_2D,camTexture);
      texWidth = sharedFrame.cols;
      texHeight = sharedFrame.rows;
      glPixelStorei(GL_UNPACK_ALIGNMENT,1);
      glTexImage2D(GL_TEXTURE_2D,0,GL_RGB8,texWidth,texHeight,0,GL_RGB,GL_UNSIGNED_BYTE,sharedFrame.data);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    }
      else
    {
      glBindTexture(GL_TEXTURE_2D,camTexture);
      glTexSubImage2D(GL_TEXTURE_2D,0,0,0,texWidth,texHeight,GL_RGB,GL_UNSIGNED_BYTE,sharedFrame.data);
    }
    glBindTexture(GL_TEXTURE_2D,0);
    haveNewFrame = 0;
  }
  pthread_mutex_unlock(&frameLock);

  return (camTexture!=0);
}

unsigned int webcam_texture() { return camTexture; }

void webcam_stop()
{
  if (!active) { return; }
  running = 0;
  pthread_join(captureThread,0);
  if (camTexture) { glDeleteTextures(1,&camTexture); camTexture=0; }
  active = 0;
}

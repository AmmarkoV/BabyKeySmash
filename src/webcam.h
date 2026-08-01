/** @file webcam.h
 *  @brief OpenCV webcam capture thread feeding a GL texture ( iChannel0
 *         of the webcam effect shader )
 *  @author Ammar Qammaz (AmmarkoV)
 */

#ifndef WEBCAM_H_INCLUDED
#define WEBCAM_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/* Start the capture thread , retval 1=Success 0=No camera ( app keeps working ) */
int webcam_start(int deviceIndex);

/* Upload the latest frame to the GL texture if a new one arrived ,
   call from the GL thread every frame , retval 1=webcam active */
int webcam_update();

unsigned int webcam_texture();

/* Motion detection : did something move in front of the camera since the
   last call ? Outputs the motion centroid in normalized screen coordinates
   ( 0..1 , matching the on-screen mirrored image ) , retval 1=motion */
int webcam_getMotion(float * normalizedX,float * normalizedY);

void webcam_stop();

#ifdef __cplusplus
}
#endif

#endif

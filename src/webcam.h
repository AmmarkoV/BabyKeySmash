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

void webcam_stop();

#ifdef __cplusplus
}
#endif

#endif

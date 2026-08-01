/** @file audio_alsa.h
 *  @brief ALSA microphone capture -> FFT -> 512x2 texture in the ShaderToy
 *         audio texture layout ( row 0 = spectrum , row 1 = waveform ) so
 *         shaders sampling iChannel0 like on shadertoy.com work unmodified
 *  @author Ammar Qammaz (AmmarkoV)
 */

#ifndef AUDIO_ALSA_H_INCLUDED
#define AUDIO_ALSA_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/* Start the capture thread on the default ALSA device ,
   retval 1=Success 0=No microphone ( app keeps working ) */
int audio_start();

/* Recompute FFT from the freshest samples and upload the texture ,
   call from the GL thread every frame , retval 1=audio active */
int audio_update();

unsigned int audio_texture();

/* Smoothed overall loudness 0..1 , usable CPU-side */
float audio_level();

void audio_stop();

#ifdef __cplusplus
}
#endif

#endif

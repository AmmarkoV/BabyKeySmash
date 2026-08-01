/** @file speech.h
 *  @brief Spoken letters and numbers through espeak-ng ( optional , keys
 *         stay silent if it is not installed ) . English by default , Greek
 *         letter names / numbers in --greek mode
 *  @author Ammar Qammaz (AmmarkoV)
 */

#ifndef SPEECH_H_INCLUDED
#define SPEECH_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/* Detect espeak-ng / espeak , retval 1=Available 0=Speech disabled */
int speech_init();

/* volume 0.0 .. 1.0 , 0 disables speech */
void speech_setVolume(float volume);

/* espeak-ng voice variant , e.g. "f3" ( girl ) , "m5" , "whisper" .
   0 or an empty string keeps the plain default voice */
void speech_setVoice(const char * variant);

/* Print the available voice variants ( for --list-shaders ) */
void speech_listVoices();

/* Say the name of a Latin letter ( 'A'..'Z' ) or digit ( '0'..'9' ) ,
   retval 1=spoken 0=unavailable/rate limited */
int speech_sayCharacter(char character);

/* Say the name of a Greek letter by alphabet index ( 0=alpha .. 23=omega ) */
int speech_sayGreekLetter(int alphabetIndex);

/* Say a number , in Greek when greekMode is set */
int speech_sayNumber(int number,int greekMode);

#ifdef __cplusplus
}
#endif

#endif

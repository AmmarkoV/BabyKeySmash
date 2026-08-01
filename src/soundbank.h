/** @file soundbank.h
 *  @brief Keypress sound effects : loads sounds/*.ogg ( fixed format written
 *         by tools/import_sounds.sh , decoded with libvorbisfile at startup )
 *         and mixes them on an ALSA playback
 *         thread . At most SOUNDBANK_MAX_VOICES clips play simultaneously
 *         and triggers are rate limited , so key smashing can not spam audio
 *  @author Ammar Qammaz (AmmarkoV)
 */

#ifndef SOUNDBANK_H_INCLUDED
#define SOUNDBANK_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/* Load every ogg in directory and start the playback thread ,
   retval = number of sounds loaded , 0 = sound disabled ( app keeps working ) */
int soundbank_init(const char * directory);

/* Trigger a random clip ; silently dropped when both voices are busy or the
   last trigger was less than the cooldown ago */
void soundbank_playRandom();

void soundbank_close();

#ifdef __cplusplus
}
#endif

#endif

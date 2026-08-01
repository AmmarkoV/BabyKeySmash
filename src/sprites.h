/** @file sprites.h
 *  @brief Textured sprites ( balloons / dinos / letters ) popping on
 *         keystrokes and mouse movement , tinyfingers.net style
 *  @author Ammar Qammaz (AmmarkoV)
 */

#ifndef SPRITES_H_INCLUDED
#define SPRITES_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/* Loads every PNG found in textureDirectory and builds the sprite shader ,
   retval 1=Success ( even with zero textures , letters still work ) */
int sprites_init(const char * textureDirectory,int screenWidth,int screenHeight);

/* A random texture pops at x,y ( pixels ) , pass -1,-1 for a random spot .
   retval = short name of the spawned texture ( e.g. "cow" for
   emoji_cow.png , usable with soundbank_playNamed ) or 0 */
const char * sprites_spawnRandomTexture(float x,float y);

/* count copies of one random texture pop together ( pressing digit 3 pops
   three ducks ) , retval = short name of the texture like above */
const char * sprites_spawnCounted(int count);

/* Greek keyboard layout mapping , 'A'..'Z' -> alphabet index 0..23 or -1 */
int sprites_latinToGreekIndex(char character);

/* Load an image ( png / jpg ) into a plain GL texture , used for scene
   artwork like the moon photo of the sleepy scene , retval 0 = failure */
unsigned int sprites_loadImageTexture(const char * filename);

/* Load the pre-baked Greek alphabet ( see tools/make_textures.py ) so that
   letter keys pop Greek letters , retval = number of letters loaded */
int sprites_loadGreek(const char * greekDirectory);

/* The pressed letter/digit pops at a random spot ; when the Greek alphabet
   is loaded , letter keys pop their Greek layout equivalent */
void sprites_spawnLetter(char character);

/* Pop a Greek letter by alphabet index ( 0=alpha .. 23=omega ) , used for
   Greek keyboard layout keysyms ; falls back to a random texture when the
   Greek alphabet is not loaded */
void sprites_spawnGreek(int alphabetIndex);

/* Small short-lived puff following the mouse */
void sprites_spawnTrail(float x,float y);

/* mouse position lets sprites be pushed around by the cursor */
void sprites_updateAndDraw(float deltaSeconds,float mouseX,float mouseY);

void sprites_close();

#ifdef __cplusplus
}
#endif

#endif

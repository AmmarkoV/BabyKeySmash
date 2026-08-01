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

/* A random texture pops at x,y ( pixels ) , pass -1,-1 for a random spot */
void sprites_spawnRandomTexture(float x,float y);

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

void sprites_updateAndDraw(float deltaSeconds);

void sprites_close();

#ifdef __cplusplus
}
#endif

#endif

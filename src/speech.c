/** @file speech.c
 *  @brief see speech.h , plain C . Utterances are fired through system()
 *         with a trailing & so the render loop never blocks , and are rate
 *         limited so key smashing does not queue endless speech
 *  @author Ammar Qammaz (AmmarkoV)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "speech.h"

#define SPEECH_COOLDOWN 0.7 /* seconds between utterances */

static const char * espeakBinary = 0;
static float speechVolume = 1.0f;
static double lastSpokenTime = 0.0;
static char speechVariant[32] = "";  /* espeak-ng +variant , "" = default voice */

/* espeak-ng variants that sound nice for kids , f* are female / child like */
static const char * voiceVariants[] =
{
  "f1","f2","f3","f4","f5","m1","m3","m5","m7","croak","whisper"
};

/* Greek letter names in alphabet order , matches sprites.cpp indexing */
static const char * greekLetterNames[24] =
{
  "άλφα","βήτα","γάμμα","δέλτα","έψιλον","ζήτα","ήτα","θήτα",
  "γιώτα","κάππα","λάμδα","μι","νι","ξι","όμικρον","πι",
  "ρο","σίγμα","ταυ","ύψιλον","φι","χι","ψι","ωμέγα"
};

static double getTimeSeconds()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC,&ts);
  return (double) ts.tv_sec + (double) ts.tv_nsec / 1000000000.0;
}

int speech_init()
{
  if (access("/usr/bin/espeak-ng",X_OK)==0) { espeakBinary="espeak-ng"; } else
  if (access("/usr/bin/espeak",X_OK)==0)    { espeakBinary="espeak";    } else
  {
    fprintf(stderr,"espeak-ng not found , letters will not be spoken ( sudo apt install espeak-ng )\n");
    return 0;
  }
  fprintf(stderr,"Speech enabled through %s \n",espeakBinary);
  return 1;
}

void speech_setVolume(float volume)
{
  speechVolume = volume;
}

void speech_setVoice(const char * variant)
{
  if ( (variant==0) || (variant[0]==0) ) { speechVariant[0]=0; return; }

  //only accept known variants , the value ends up in a shell command
  unsigned int v;
  for (v=0; v<sizeof(voiceVariants)/sizeof(voiceVariants[0]); v++)
  {
    if (strcmp(variant,voiceVariants[v])==0)
    {
      snprintf(speechVariant,sizeof(speechVariant),"+%s",variant);
      fprintf(stderr,"Speech voice variant set to %s \n",variant);
      return;
    }
  }
  fprintf(stderr,"Unknown voice variant %s , using the default voice ( --list-shaders shows them )\n",variant);
  speechVariant[0]=0;
}

void speech_listVoices()
{
  printf("Speech voices (--voice)     :");
  unsigned int v;
  for (v=0; v<sizeof(voiceVariants)/sizeof(voiceVariants[0]); v++)
    { printf(" %s",voiceVariants[v]); }
  printf("\n");
}

/* text must come from the fixed tables / characters of this file only ,
   it ends up inside a shell command */
static int say(const char * language,const char * text)
{
  if ( (espeakBinary==0) || (speechVolume<=0.0f) ) { return 0; }

  double now = getTimeSeconds();
  if (now-lastSpokenTime < SPEECH_COOLDOWN) { return 0; }
  lastSpokenTime = now;

  char command[256];
  snprintf(command,sizeof(command),"%s -a %u -v %s%s \"%s\" >/dev/null 2>&1 &",
           espeakBinary,(unsigned int) (speechVolume*180.0f),language,speechVariant,text);
  int result = system(command);
  return (result==0);
}

int speech_sayCharacter(char character)
{
  if ( ! ( ((character>='A')&&(character<='Z')) ||
           ((character>='a')&&(character<='z')) ||
           ((character>='0')&&(character<='9')) ) ) { return 0; }
  char text[2] = { character , 0 };
  return say("en",text);
}

int speech_sayGreekLetter(int alphabetIndex)
{
  if ( (alphabetIndex<0) || (alphabetIndex>=24) ) { return 0; }
  return say("el",greekLetterNames[alphabetIndex]);
}

int speech_sayNumber(int number,int greekMode)
{
  char text[16];
  snprintf(text,sizeof(text),"%u",number);
  return say( (greekMode) ? "el" : "en" , text);
}

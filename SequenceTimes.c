//  Star Wars TSG
//  Darel Rex Finley, 2006-2011



#include           "nil.h"
#include          "bool.h"
#include   "definitions.h"

#include "SequenceTimes.h"



//  Constants describing each part of the opening sequence, and how much time (in seconds) each takes to occur.
//
//  Note:  If you change these values, you may need to change FILMCO_SPARKLES_BEGIN and FILMCO_SPARKLES_END to work
//         nicely with your new values.  At the current time, FILMCO_SPARKLES_BEGIN is set to about 75% of the way
//         through the FILMCO_FADE_UP segment, and FILMCO_SPARKLES_END is set to approximately the beginning of the
//         FILMCO_ORANGE_TO_YELLOW segment.
//
//  Warning:  With the exception of SEQUENCE_END, all of these times should be greater than the largest allowed
//            duration of a single frame.  Do not allow frame rates low enough to violate that rule -- otherwise
//            some frames may be rendered incorrectly.  Since, currently, the lowest allowed frame rate is 6 fps,
//            the largest possible frame duration is 0.16667 seconds; hence each of these values is greater than
//            0.16667.

double  SequenceTimes[]={

  /*
  time      name
  =======   =============================   */
  2.79167,  DARK_A                       ,
  0.79167,  DISTRIBUTOR_FADE_UP          ,
  5.5    ,  DISTRIBUTOR_HOLD             ,
  0.79167,  DISTRIBUTOR_FADE_DOWN        ,
  0.25   ,  DARK_B                       ,
  0.45833,  FILMCO_FADE_UP               ,
  1.41667,  FILMCO_SPARKLE_GREEN         ,
  2.16667,  FILMCO_SPARKLE_GREEN_TO_BLUE ,
  1.25   ,  FILMCO_SPARKLE_BLUE_TO_ORANGE,
  0.83333,  FILMCO_ORANGE_TO_YELLOW      ,
  1.625  ,  FILMCO_FADE_DOWN             ,
  3.70833,  DARK_C                       ,
  0.83333,  INTRO_FADE_UP                ,
  3.04167,  INTRO                        ,
  0.83333,  INTRO_FADE_DOWN              ,
  2.625  ,  DARK_D                       ,
  10.875 ,  TITLE_PULL_BACK              ,
  0.58333,  CRAWL_AND_TITLE_PULL_BACK    ,
  0.70833,  CRAWL_AND_TITLE_FADE_DOWN    ,
  50     ,  CRAWL_TO_ALL_TEXT_VISIBLE    ,   //  <-- This value gets increased (by code) to accommodate a lengthy crawl.
  23     ,  CRAWL_OFF_TO_DISTANCE        ,
  2.29167,  CRAWL_FADE_DOWN              ,
  1      ,  JUST_STARS                   ,
  0      ,  SEQUENCE_END                 }
;

int4  SequenceTimes_elementCount=sizeof(SequenceTimes)/sizeof(SequenceTimes[0]) ;



//  Verifies the integrity of the SequenceTimes array, and totals up the times contained in it.
//
//  This function is called at app startup, but also when the CRAWL_TO_ALL_TEXT_VISIBLE value gets changed.
//
//  Returns an error message, or nil (not an empty string) if there was no error.

BYTE *verifySequenceTimes(double *totalTime) {

  int4  i ;

  if (SequenceTimes_elementCount!=2*(SEQUENCE_END+1)) return "SequenceTimes incorrectly sized.";
  (*totalTime)=0.;
  for (i=0; i<=SEQUENCE_END; i++) {
    (*totalTime)  +=SequenceTimes[i*2  ];
    if ((double) i!=SequenceTimes[i*2+1]) return "SequenceTimes contains bad data."; }

  //  Success.
  return nil; }



//  Get the duration of a specified stage of the opening sequence.

double getSequenceTime(int4 i) {
  return SequenceTimes[i*2]; }



//  Set the duration of a specified stage of the opening sequence.
//
//  Used only by the handleGoStopButton method, to change the duration of
//  CRAWL_TO_ALL_TEXT_VISIBLE for the purpose of accommodating larger-than-normal crawl text.

void setSequenceTime(int4 i, double val) {
  SequenceTimes[i*2]=val; }
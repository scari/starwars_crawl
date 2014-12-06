//  Star Wars TSG
//  Darel Rex Finley, 2006-2011



#include              "definitions.h"
#include "user interface functions.h"



//  Returns the width of a user-chosen image size.

int4 getScreenWidth(int4 sel) {

  int4  wid[]={
    320, 480, 640, 720, 720, 1280, 1920 }
  ;

  //  Return zero if the user selection is outside the range of the “wid” array.
  if (sel<0L || sel>=sizeof(wid)/sizeof(wid[0])) return 0;

  return wid[sel]; }



//  Returns the height of a user-chosen image size.

int4 getScreenHeight(int4 sel) {

  int4  hei[]={
    136, 204, 272, 480, 576, 720, 1080 }
  ;

  //  Return zero if the user selection is outside the range of the “hei” array.
  if (sel<0L || sel>=sizeof(hei)/sizeof(hei[0])) return 0;

  return hei[sel]; }



//  Returns the height-used of a user-chosen image size.

int4 getScreenHeightUsed(int4 sel) {

  int4  hei[]={
    136, 204, 272, 363, 436, 545, 817 }
  ;

  //  Return zero if the user selection is outside the range of the “hei” array.
  if (sel<0L || sel>=sizeof(hei)/sizeof(hei[0])) return 0;

  return hei[sel]; }



//  Returns the frame rate of the user-chosen content system.

double getFrameRate(int4 sel) {

  double framesPerSec[]={
    24., 25., 29.97 }
  ;

  //  Return zero if the user selection is outside the range of the “framesPerSec” array.
  if (sel<0L || sel>=sizeof(framesPerSec)/sizeof(framesPerSec[0])) return 0.;

  return framesPerSec[sel]; }
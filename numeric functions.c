//  Star Wars TSG
//  Darel Rex Finley, 2006-2011



#include              "bool.h"
#include       "definitions.h"
#include            "stdlib.h"

#include "numeric functions.h"



//  Returns the smaller of two values (doubles).

double minDouble(double a, double b) {
  if (a<b) return a;
  else     return b; }



//  Returns the larger of two values (int4s).

int4 maxLong(int4 a, int4 b) {
  if (a>b) return a;
  else     return b; }



//  Returns a random number from 0 to 1 -- inclusive of 0, but not inclusive of 1.
//
//  Important:  Assumes that the system function random() returns an integer
//              ranging from 0 to MAX_INT4, inclusive.
//
//  Note:  Code that uses this function probably should first seed the system’s
//         random number generator by calling the system function “srandomdev”.

double rnd0to1() {
  return (double) random()/((double) MAX_INT4+1.); }



//  Forces an integer to be divible by “div”.  Rounds UP to the nearest multiple of “div”.

int4 forceDiv(int4 n, int4 div) {
  return (n+div-1L)/div*div; }



//  Get a little-endian value from memory, without the need for a little-endian processor.

unsigned int4 getULongLittleEndian(BYTE *src) {
  return
  (unsigned int4) src[0]               +
  (unsigned int4) src[1]*256L          +
  (unsigned int4) src[2]*256L*256L     +
  (unsigned int4) src[3]*256L*256L*256L; }



//  Put a little-endian value to memory, without the need for a little-endian processor.

void putULongLittleEndian(BYTE *dst, unsigned int4 val) {
  dst[0]=(BYTE) (val%256L); val/=256L;
  dst[1]=(BYTE) (val%256L); val/=256L;
  dst[2]=(BYTE) (val%256L);
  dst[3]=(BYTE) (val/256L); }
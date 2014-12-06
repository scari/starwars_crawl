//  Star Wars TSG
//  Darel Rex Finley, 2006-2011



//  “cbf” stands for “Compressed Bitmap Font”.  This file contains functions related to the use of those fonts.
//
//  See the file “Compressed Bitmap Font (CBF) Definition” for detail on the structure of a compressed bitmap font.



#include               "nil.h"
#include              "bool.h"
#include       "definitions.h"
#include "numeric functions.h"
#include  "string functions.h"

#include     "cbf functions.h"



//  This function corrects (i.e. nativizes) the endianness of multibyte values in a compressed bitmap font (cbf),
//  so that those values can be accessed natively by the processor during rendering.
//
//  Returns NO if the data looks too corrupted to work on.

bool correctBitmapFontEndianness(BYTE *font, int4 fontLen) {

  int4   *ptrToLong, glyphI, i, row, glyphHei, glyphs ;

  //  Ensure that the minimal data structure is present.
  if (fontLen<8L) return NO;

  //  Obtain the glyph height.
  glyphHei=(int4) getULongLittleEndian(font); if (glyphHei<1L) return NO;

  //  Correct the glyph height.
  ptrToLong=(int4 *) font; *ptrToLong=glyphHei;

  //  Obtain the glyph count.
  glyphs=(int4) getULongLittleEndian(&font[4]); if (glyphs<1L || fontLen<8L+4L*glyphs) return NO;

  //  Correct the glyph count.
  ptrToLong=(int4 *) &font[4]; *ptrToLong=glyphs;

  //  Loop through all the glyphs.
  for (glyphI=0L; glyphI<glyphs; glyphI++) {
    ptrToLong=(int4 *)            &font[8L+4L*glyphI];
    i=(int4) getULongLittleEndian(&font[8L+4L*glyphI]); if (i<0L || i>fontLen) return NO;
    if (i) {   //  (process only supported glyphs)

      //  Correct the glyph-data offset.
      *ptrToLong=i;

      //  Correct the glyph width.
      if (i+4L>fontLen) return NO;
      ptrToLong=(int4 *) &font[i]; *ptrToLong=(short) getULongLittleEndian(&font[i]); i+=4L;

      //  Loop through each row of the glyph.
      for (row=0L; row<(int4) glyphHei; row++) {

        //  Correct the glyph-row-data offset.
        if (i+4L>fontLen) return NO;
        ptrToLong=(int4 *) &font[i]; *ptrToLong=(int4) getULongLittleEndian(&font[i]); i+=4L; }}}

  //  Success.
  return YES; }



//  This frees the rendering code from the burden of noticing unsupported glyphs, by forcing all glyphs to be supported.
//  Unsupported glyphs are either re-pointed to an uppercase version of the same glyph, or to the first supported glyph.  (This
//  code assumes that at least one glyph is supported.)
//
//  Note:  Before calling this function, ensure that multibyte values use the native endianness of the current processor -- see
//         correctBitmapFontEndianness.  Actually:  In theory it might not matter, since this code is concerned only whether
//         the values are zero or non-zero, and otherwise just copies the values around.  But it can’t hurt to be sure.

void correctBitmapFontMissingGlyphs(BYTE *font) {

  int4   c, i, glyphs, *ptrToGlyph, *ptrToDefaultGlyph=nil, *ptrToSubstituteGlyph ;

  //  Get the number of glyphs in the font.
  ptrToGlyph=(int4 *) &font[4]; glyphs=*ptrToGlyph;

  //  Find the first supported glyph.
  for (i=0; i<glyphs; i++) {
    ptrToGlyph=(int4 *) &font[8L+4L*i];
    if (*ptrToGlyph) {
      ptrToDefaultGlyph=ptrToGlyph; break; }}

  //  Correct all unsupported glyphs.
  for (i=0; i<glyphs; i++) {
    ptrToGlyph=(int4 *) &font[8L+4L*i];
    if (!(*ptrToGlyph)) {   //  (This glyph is unsupported; point it to an uppercase version, or to the first supported glyph.)
      c=i; uniCharToUpperCase(&c);  ptrToSubstituteGlyph=(int4 *) &font[8L+4L*c];
      if (!(*ptrToSubstituteGlyph)) ptrToSubstituteGlyph=ptrToDefaultGlyph;
      *ptrToGlyph=*ptrToSubstituteGlyph; }}}



//  Returns the pixel height of a compressed bitmap font.
//
//  Warning:  This function assumes that the compressed-bitmap-font structure already has been
//            processed to the processor’s native endianness -- see correctBitmapFontEndianness.)

int4 cbfHeight(BYTE *font) {

  int4  *ptrToLong=(int4 *) font;

  return *ptrToLong; }



//  Returns the pixel width of a specified glyph of a compressed bitmap font.
//
//  Also corrects the character value to the range of the specified font.
//
//  Warning:  This function assumes that the compressed-bitmap-font structure already has been
//            processed to the processor’s native endianness -- see correctBitmapFontEndianness.)

int4 cbfWidth(BYTE *font, int4 *c) {

  int4  *ptrToLong=(int4 *) &font[4] ;

  //  Prevent the character value from going out of the range of this font.
  if ((*c)>=(*ptrToLong)) *c=1;

  //  Construct a pointer to the glyph width.
  ptrToLong=(int4 *) &font[8];
  ptrToLong=(int4 *) ((long) font+ptrToLong[*c]);

  //  Return the glyph width.
  return *ptrToLong; }
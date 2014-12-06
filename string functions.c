//  Star Wars TSG
//  Darel Rex Finley, 2006-2011



#include             "bool.h"
#include      "definitions.h"

#include "string functions.h"



//  List of supported characters that have an ÒuppercaseÓ and ÒlowercaseÓ version, even
//  though they are not alphabetic (i.e. not A through Z).  Must be null-terminated.
//
//  Important:  All character values less than 256 are automatically converted to their
//              Unicode values on app launch, in the handleDisclaimerAgreed method --
//              except for negative values, which are only made positive.

int4  SpecialCaseCharsLower[]={
  'ˆ','','“','˜','','‡','Ž','’','—','œ','Š','‘','•','š','Ÿ','Ø','‰','','”','™','ž','‹','›','–','','Œ','¾','Ï','¹','¿',
  269,353,382,305,273,259,261,263,271,283,281,287,318,314,322,324,328,337,341,345,347,351,537,357,539,369,367,378,380,-240,-253,-254,0 }
;
int4  SpecialCaseCharsUpper[]={
  'Ë','é','í','ñ','ô','ç','ƒ','ê','î','ò','€','è','ì','…','†','Ù','å','æ','ë','ï','ó','Ì','Í','„','‚','','®','Î','¸','¯',
  268,352,381,304,272,258,260,262,270,282,280,286,317,313,321,323,327,336,340,344,346,350,536,356,538,368,366,377,379,-208,-221,-222,0 }
;



//  Table to convert the Mac OS Roman characters 0x80-0xFF to Unicode (not UTF-8).
//  Derived from the table at:  http://alanwood.net/demos/macroman.html
int4  MacRomanToUnicode[128]={
  196  ,  197,  199,  201,  209,  214,  220,  225,  224,  226,  228,  227,  229,  231,   233,   232,
  234  ,  235,  237,  236,  238,  239,  241,  243,  242,  244,  246,  245,  250,  249,   251,   252,
  8224 ,  176,  162,  163,  167, 8226,  182,  223,  174,  169, 8482,  180,  168, 8800,   198,   216,
  8734 ,  177, 8804, 8805,  165,  181, 8706, 8721, 8719,  960, 8747,  170,  186,  937,   230,   248,
  191  ,  161,  172, 8730,  402, 8776, 8710,  171,  187, 8230,  160,  192,  195,  213,   338,   339,
  8211 , 8212, 8220, 8221, 8216, 8217,  247, 9674,  255,  376, 8260, 8364, 8249, 8250, 64257, 64258,
  8225 ,  183, 8218, 8222, 8240,  194,  202,  193,  203,  200,  205,  206,  207,  204,   211,   212,
  63743,  210,  218,  219,  217,  305,  710,  732,  175,  728,  729,  730,  184,  733,   731,   711  }
;



//  Convert a character from Mac OS Roman text to Unicode, or change negative Unicode value to positive.

void convertMacCharToUnicode(int4  *c) {
  if      ((*c)>=128 && (*c)<256)  *c=MacRomanToUnicode[(*c)-128];
  else if ((*c)<   0            ) (*c)*=-1; }



//  Converts all paragraph-end characters to just RETURN_CHAR, even if they are RETURN_CHAR/NEWLINE_CHAR pairs.

void fixReturnAndNewlineChars(int4 *text) {

  int4  i=0, j=0 ;

  while (text[i]) {
    if      (text[i]== RETURN_CHAR) {
      text[j]=RETURN_CHAR; if (text[i+1]==NEWLINE_CHAR) i++; }
    else if (text[i]==NEWLINE_CHAR) {
      text[j]=RETURN_CHAR; if (text[i+1]== RETURN_CHAR) i++; }
    else {
      text[j]=text[i]; }
    i++; j++; }
  text[j]=0; }



//  Copies a null-terminated string, up to a specificed number of non-null characters in length.
//
//  Important:  dst should point to an array that has at least max+1 bytes.
//
//       Note:  If either dst or src is nil, this function will do nothing -- but if just src
//              is nil, this function will create an empty string at dst.

void copyText(BYTE *dst, BYTE *src, int4 max) {

  int4  i=0 ;

  //  Do nothing if the destination is nil.
  if (!dst) return;

  //  If the source is nil, create an empty string at the destination.
  if (!src) {
    *dst=(BYTE) 0;
    return; }

  //  Copy the string from the source to the destination.
  while (i<max && src[i]) {
    dst[i]=src[i]; i++; }
  dst  [i]=(BYTE) 0; }



//  This is a variant on copyText (see its comments), but this
//  one works with strings of int4s instead of strings of bytes.

void copyTextLong(int4 *dst, int4 *src, int4 max) {

  int4  i=0 ;

  //  Do nothing if the destination is nil.
  if (!dst) return;

  //  If the source is nil, create an empty string at the destination.
  if (!src) {
    *dst=0;
    return; }

  //  Copy the string from the source to the destination.
  while (i<max && src[i]) {
    dst[i]=src[i]; i++; }
  dst  [i]=0; }



//  Copies a null-terminated UTF-8 string to a string of int4 (4-byte) values, up to a specificed
//  number of non-null characters in length.
//
//  Since UTF-8 characters are at most 21 bits (when decoded), the signedness of a plain (signed)
//  int4 is not an issue.
//
//  This function looks carefully for invalid UTF-8 structure -- if it finds any, or if the UTF-8
//  string was excessively large, then an empty string will be returned in int4Str.
//
//  Warning:  int4Str must have enough space to hold max+1 int4s.

void copyTextUtf8ToLongStr(int4 *int4Str, BYTE *utf, int4 max) {

  int4  i, j, count ;
  BYTE  c, d ;

  //  Scan the UTF-8 string, verifying its integrity and discovering its character count.
  //  Note:  This integrity check does not detect characters encoded with more bytes than
  //         necessary.
  i=0; count=0; *int4Str=0; c=utf[i++];
  while (c) {
    if ((++count)>2000000000) return;   //  (more than 2 billion characters assumed to be an error)
    if     (c&128) {
      if (!(c& 64)) return;
      d    =utf[i++]; if (d<128 || (d&64)         ) return;
      if   (c& 32) {
        d  =utf[i++]; if (d<128 || (d&64)         ) return;
        if (c& 16) {
          d=utf[i++]; if (d<128 || (d&64) || (c&8)) return; }}}
    c=utf[i++]; }

  //  Translate the UTF-8 string to a string of int4s.  (This code plays fast-and-loose with UTF-8
  //  parsing, since the UTF-8 stringÕs validity and count already have been determined above.)
  i=0; j=0; if (count>max) count=max;
  while (count--) {
    c=utf[i++];
    if      (!(c&128)) {
      int4Str[j++]=          c                                                     ;       }    //  1-byte encoding
    else if (!(c& 32)) {
      int4Str[j++]=      64*(c&31)+      (utf[i]&63)                               ; i++ ; }    //  2-byte encoding
    else if (!(c& 16)) {
      int4Str[j++]=   64*64*(c&15)+   64*(utf[i]&63)+   (utf[i+1]&63)              ; i+=2; }    //  3-byte encoding
    else               {
      int4Str[j++]=64*64*64*(c& 7)+64*64*(utf[i]&63)+64*(utf[i+1]&63)+(utf[i+2]&63); i+=3; }}   //  4-byte encoding
  int4Str[j]=0; }



//  Converts a null-terminated Mac OS Roman C-string to a null-terminated string of Unicode int4s.

void convertMacStrToUnicodeLongStr(int4 *int4Str, BYTE *macStr, int4 max) {

  int4  i=0 ;

  while (i<max && macStr[i]) {
    int4Str[i]=   macStr[i]; convertMacCharToUnicode(&int4Str[i++]); }
  int4Str  [i]=0; }



//  Converts a null-terminated string of int4s to a null-terminated UTF-8 string.
//
//  Overwrites the same space that the string of int4s occupies, destroying that string of int4s.
//  Any int4 value outside the 21-bit range (0 to 2,097,151) will be converted to a character value of 1.
//  The UTF-8 string will be arbitrarily terminated if it reaches about 2 billion bytes (not characters).

void convertLongStrToUtf8(int4 *int4Str) {

  int4   i=0, j=0,  c=int4Str[i++] ;
  BYTE  *utf=(BYTE *) int4Str      ;

  while     (c && j<2000000000) {
    if      (c<    0 || c>=UTF8_CHARS) utf[j++]=1;
    else if (c<  128                 ) utf[j++]=c;
    else if (c< 2048                 ) {
      utf[j++]=  192+ c        /    64;
      utf[j++]=  128+(c&63    )       ; }
    else if (c<65536                 ) {
      utf[j++]=  224+ c        /  4096;
      utf[j++]=  128+(c&4095  )/    64;
      utf[j++]=  128+(c&63    )       ; }
    else                               {
      utf[j++]=  240+ c        /262144;
      utf[j++]=  128+(c&262143)/  4096;
      utf[j++]=  128+(c&4095  )/    64;
      utf[j++]=  128+(c&63    )       ; }
    c=int4Str[i++]; }
  utf[j]=0; }



//  Copy three strings into one string, using a delimeter character inbetween them.
//
//  Used by the Undo/Redo system, so that a single user action that changes three fields (Width, Height, and
//  Height Used) can be stored as a single Undo item.
//
//  Warning:  Before calling, ensure that there is at least max+1 bytes at dst.
//
//  Warning:  Do not call with max less than 2.
//
//  Note:  If the delimeter character is found in any of the three strings, it will be ignored (and therefore lost).
//
//  Note:  If the limit imposed by ÒmaxÓ is reached, strings will be arbitrarily truncated, with no special system
//         to spread the damage evenly across the three strings.

void copy3stringsTo1(int4 *dst, int4 *src1, int4 *src2, int4 *src3, int4 max) {

  int4  i=0, j ;

  //  Copy the first string.
  j=0;
  while (src1[j] && i<max) {
    if  (src1[j]!=MULTI_STRING_DELIMITER) dst[i++]=src1[j];
    j++; }
  if (i==max) {
    dst[i-2]=MULTI_STRING_DELIMITER;
    dst[i-1]=MULTI_STRING_DELIMITER;
    dst[i  ]=0;
    return; }
  dst[i++]=MULTI_STRING_DELIMITER;

  //  Copy the second string.
  j=0;
  while (src2[j] && i<max) {
    if  (src2[j]!=MULTI_STRING_DELIMITER) dst[i++]=src2[j];
    j++; }
  if (i==max) {
    dst[i-1]=MULTI_STRING_DELIMITER;
    dst[i  ]=0;
    return; }
  dst[i++]=MULTI_STRING_DELIMITER;

  //  Copy the third string.
  j=0;
  while (src3[j] && i<max) {
    if  (src3[j]!=MULTI_STRING_DELIMITER) dst[i++]=src3[j];
    j++; }
  dst[i]=0; }



//  This function is the reverse of copy3stringsTo1.
//
//  Used by the Undo/Redo system, so that a single user action that changes three fields (Width, Height, and
//  Height Used) can be stored as a single Undo item.
//
//  Warning:  Before calling this function, Call the function is3strings to test whether the string will work
//            correctly with this function.
//
//  Warning:  Before calling this function, ensure there is enough space at the three destination strings.

void copy1stringTo3(int4 *dst1, int4 *dst2, int4 *dst3, int4 *src, int4 max) {

  int4  i=0, j ;

  //  Copy the first string.
  j=0; while (src[i]!=MULTI_STRING_DELIMITER) dst1[j++]=src[i++];
  dst1[j]=0; i++;

  //  Copy the second string.
  j=0; while (src[i]!=MULTI_STRING_DELIMITER) dst2[j++]=src[i++];
  dst2[j]=0; i++;

  //  Copy the third string.
  j=0; while (src[i]                        ) dst3[j++]=src[i++];
  dst3[j]=0; }



//  Test whether a string will work with copy1stringTo3 without causing it to crash -- see comments at that function.
//
//  Returns YES if the string looks right, otherwise NO.

bool is3strings(int4 *text) {

  int4  i=0, count=1 ;

  while (text[i]) if (text[i++]==MULTI_STRING_DELIMITER) count++;

  return count==3L; }



//  Tells whether a character is considered uppercase, which can be true even if it is non-alphabetic -- see the
//  strings SpecialCaseCharsUpper and SpecialCaseCharsLower.
//
//  Closely related to the function isLowerCase.
//
//  Important:  This function is not an exact inverse of isLowerCase -- this returns NO if the character has
//              no case, e.g. '4'.

bool isUpperCase(int4 c) {

  short  i ;

  //  Handle unaccented English alphabet (which is assumed to be sequential in character set).
  if (c>='A' && c<='Z') return YES;

  //  Handle all other characters that can be capitalized.
  i=0;
  while (SpecialCaseCharsUpper[i]) {
    if  (SpecialCaseCharsUpper[i]==c) return YES;
    i++; }
  return NO; }



//  Tells whether a character is considered lowercase, which can be true even if it is non-alphabetic -- see the
//  SpecialCaseCharsUpper and SpecialCaseCharsLower strings.
//
//  Closely related to the function isUpperCase.
//
//  Important:  This function is not an exact inverse of isUpperCase -- this returns NO if the character has
//              no case, e.g. '4'.

bool isLowerCase(int4 c) {

  short  i ;

  //  Handle unaccented English alphabet (which is assumed to be sequential in character set).
  if (c>='a' && c<='z') return YES;

  //  Handle all other characters that can be capitalized.
  i=0;
  while (SpecialCaseCharsLower[i]) {
    if  (SpecialCaseCharsLower[i]==c) return YES;
    i++; }
  return NO; }



//  Convert a character to uppercase.  Changes only the English alphabetic characters from 'a' to 'z'.

void charToUpperCaseAlphaOnly(BYTE *c) {
  if ((*c)>='a' && (*c)<='z') (*c)+='A'-'a'; }



//  Convert a character to uppercase, if it has an uppercase version.
//
//  Note:  This may change the character even if it is non-alphabetic -- see the strings SpecialCaseCharsUpper
//         and SpecialCaseCharsLower.

void uniCharToUpperCase(int4 *c) {

  short  i ;

  //  Handle unaccented English alphabet (which is assumed to be sequential in character set).
  if ((*c)>='a' && (*c)<='z') {
    (*c)+='A'-'a'; return; }

  //  Handle all other characters that can be capitalized.
  i=0;
  while      (SpecialCaseCharsLower[i]) {
    if ((*c)==SpecialCaseCharsLower[i]) {
      (  *c) =SpecialCaseCharsUpper[i]; return; }
    i++; }}



//  Convert a string of int4s to uppercase.
//
//  Note:  This may change characters even if they area non-alphabetic --
//         see the strings SpecialCaseCharsUpper and SpecialCaseCharsLower.

void forceUpperCase(int4 *text) {

  int4  i=0, c=*text ;

  while (c) {
    uniCharToUpperCase(&c); text[i++]=c; c=text[i]; }}



//  Compare two strings of bytes.  Returns YES if they are identical, otherwise NO.
//
//  Note:  If the string pointers are both nil, this function returns YES, but if only one of them is nil,
//         it returns NO.
//
//  Closely related to int4StrEqual.

bool strEqual(BYTE *a, BYTE *b) {

  int4  i=0 ;

  //  Handle nil pointers.
  if (!a && !b) return YES;
  if (!a || !b) return  NO;

  //  Compare the string contents.
  while (a[i] && b[i]) {
    if  (a[i] != b[i]) return NO;
    i++; }

  //  Return the result.
  return a[i] == b[i]; }



//  Compare two strings of int4s.  Returns YES if they are identical, otherwise NO.
//
//  Note:  If the string pointers are both nil, this function returns YES, but if only one of them is nil,
//         it returns NO.
//
//  Closely related to strEqual.

bool int4StrEqual(int4 *a, int4 *b) {

  int4  i=0 ;

  //  Handle nil pointers.
  if (!a && !b) return YES;
  if (!a || !b) return  NO;

  //  Compare the string contents.
  while (a[i] && b[i]) {
    if  (a[i] != b[i]) return NO;
    i++; }

  //  Return the result.
  return a[i] == b[i]; }



//  This function returns YES if c is a numeric digit, or is a character that can be upper-or-lower case (even if
//  that character is not technically alphabetic -- see the strings SpecialCaseCharsUpper and SpecialCaseCharsLower).

bool isAlphaNumeric(int4 c) {

  //  Handle numeric digits (which are presumed to be sequential in the character set).
  if (c>='0' && c<='9') return YES;

  //  Handle characters that can be upper-or-lower case.
  return isLowerCase(c)
  ||     isUpperCase(c); }



//  In a string, converts all straight quotes to directional quotes.

void straightQuotesToDirectional(int4 *text) {

  int4  i=0, quoteL, quoteR ;

  while  (text[i]) {
    if   (text[i]=='\''
    ||    text[i]== '"') {
      if (text[i]=='\'') {
        quoteL='Ô';
        quoteR='Õ'; }
      else {
        quoteL='Ò';
        quoteR='Ó'; }
      convertMacCharToUnicode(&quoteL);
      convertMacCharToUnicode(&quoteR);
      if      (!text[i+1]) text[i]=quoteR;
      else if (!i        ) text[i]=quoteL;
      else if (!isAlphaNumeric(text[i-1]) &&  isAlphaNumeric(text[i+1])) text[i]=quoteL;
      else if ( isAlphaNumeric(text[i-1]) && !isAlphaNumeric(text[i+1])) text[i]=quoteR;
      else if (text[i-1]==' '             && text[i+1]!=' '            ) text[i]=quoteL;
      else                                                               text[i]=quoteR; }
    i++; }}



//  Removes leading and trailing spaces from a string, and reduces multiple consecutive spaces to one space each.

void stripExtraSpaces(int4 *text) {

  int4  i=0, j=0, c, lastChar=' ' ;
  bool  foundText=NO ;

  while (text[i]) {
    c=text[i]; if (c!=' ') foundText=YES;
    if (foundText && (c!=' ' || lastChar!=' ')) text[j++]=c;
    lastChar=c; i++; }
  if (j && text[j-1]==' ') j--;
  text[j]=0; }



//  Ensures that a string is at least one character in length.
//
//  WARNING:  There must be at least two int4s allocated at ÒtextÓ when calling this function.

void forceAtLeastOneChar(int4 *text) {
  if (!text[0]) {
    text[0]=1;
    text[1]=0; }}
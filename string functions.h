//  Star Wars TSG
//  Darel Rex Finley, 2006-2011

extern int4  SpecialCaseCharsUpper[] ;
extern int4  SpecialCaseCharsLower[] ;
extern int4  MacRomanToUnicode[128] ;

void convertMacCharToUnicode      (int4 *c);
void fixReturnAndNewlineChars     (int4 *text);
void copyText                     (BYTE *dst, BYTE *src, int4 max);
void copyTextLong                 (int4 *dst, int4 *src, int4 max);
void copyTextUtf8ToLongStr        (int4 *int4Str, BYTE *utf, int4 max);
void convertMacStrToUnicodeLongStr(int4 *int4Str, BYTE *macStr, int4 max);
void convertLongStrToUtf8         (int4 *int4Str);
bool isUpperCase                  (int4 c);
bool isLowerCase                  (int4 c);
void charToUpperCaseAlphaOnly     (BYTE *c);
void uniCharToUpperCase           (int4 *c);
void forceUpperCase               (int4 *text);
bool strEqual                     (BYTE *a, BYTE *b);
bool int4StrEqual                 (int4 *a, int4 *b);
bool isAlphaNumeric               (int4 c);
void straightQuotesToDirectional  (int4 *text);
void copy3stringsTo1              (int4 *dst, int4 *src1, int4 *src2, int4 *src3, int4 max);
void copy1stringTo3               (int4 *dst1, int4 *dst2, int4 *dst3, int4 *src, int4 max);
bool is3strings                   (int4 *text);
void stripExtraSpaces             (int4 *text);
void forceAtLeastOneChar          (int4 *text);
//  Star Wars TSG
//  Darel Rex Finley, 2006-2011

extern DoubleRect_  IntroChar[MAX_CHARS_INTRO  ] ;
extern int4         IntroText[MAX_CHARS_INTRO+1] ;
extern int4         IntroChars ;

extern bool  IntroImageReady ;

extern BYTE  *CbfIntro ;

extern double  IntroFade ;

void  forceRasterIntro       (bool lo, bool hi);
BYTE *prepareToDrawIntroImage(BYTE **rgb, int4 rowBytes);
void  drawIntroPixel         (BYTE *r, BYTE *g, BYTE *b, int4 pixelX, int4 pixelY, int4 *cbfChar, BYTE *cbfFill, short *cbfX, short *cbfY, bool *cbfRaw, BYTE **cbfData);
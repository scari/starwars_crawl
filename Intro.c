//  Star Wars TSG
//  Darel Rex Finley, 2006-2011



#include                    "nil.h"
#include                   "bool.h"
#include            "definitions.h"
#include      "Shared Structures.h"
#include          "cbf functions.h"
#include "image buffer functions.h"
#include     "geometry functions.h"
#include       "string functions.h"

#include                  "Intro.h"



//  Globals used during rendering of Intro image.  (Populated primarily by
//  prepareToDrawIntroTextImage; used primarily by drawIntroPixel.)
DoubleRect_  IntroChar[MAX_CHARS_INTRO  ] ;
int4         IntroText[MAX_CHARS_INTRO+1] ;
int4         IntroChars ;

bool  IntroImageReady ;

//  Intro cbf (font).
BYTE  *CbfIntro=nil ;

double  IntroFade ;



//  Forces the area of the image in which the Intro text resides to be re-rendered.
//
//  Call this function immediately before and again immediately after the intro text changes.
//
//  Note:  The controller never calls this function with render threads running.

void forceRasterIntro(bool lo, bool hi) {

  double  left, top, right, bottom ;
  int4    i ;

  //  Exit if there are no intro characters to render.
  if (!IntroChars) return;

  //  Determine the area that needs to be rendered.
  left  =IntroChar[0].L; right=left;
  top   =IntroChar[0].T;
  bottom=IntroChar[IntroChars-1].B;
  for (i=0; i<IntroChars; i++) if (IntroChar[i].R>right) right=IntroChar[i].R;

  //  Force the needed area to be rendered.
  forceRasterRect(lo,hi,
  screenToPixelX(left  )-1,
  screenToPixelY(top   )-1,
  screenToPixelX(right )+2,
  screenToPixelY(bottom)+2); }



//  Perform the necessary preparations for rendering the Intro image.
//
//  Returns an error message, or nil (not an empty string) if there was no error.

BYTE *prepareToDrawIntroImage(BYTE **rgb, int4 rowBytes) {

  double  charWid, rightEdge=-INTRO_WIDTH_MAX*.5 ;
  int4    i=0, j, rowStart=0, afterSpace ;

  //  Error check.
  if (!CbfIntro) return "prepareToDrawIntroImage called with nil CbfIntro.";

  straightQuotesToDirectional(&IntroText[0]);

  //  Store the passed parameters in globals, for use during image drawing.
  PreviewRgb     =rgb     ;
  PreviewRowBytes=rowBytes;

  //  Parse the intro text into a set of on-screen character positions.
  while (IntroText[i]) {
    charWid=(double) cbfWidth(CbfIntro,&IntroText[i])*INTRO_LINE_HEI/(double) cbfHeight(CbfIntro);
    if (i==rowStart) {
      if (!i) {
        IntroChar[i].T= INTRO_LINE_HEI *.5+INTRO_Y_OFFSET;
        IntroChar[i].B= IntroChar[i  ].T  -INTRO_LINE_HEI; }
      else {
        IntroChar[i].T= IntroChar[i-1].B  -INTRO_LINE_SEP;
        IntroChar[i].B= IntroChar[i  ].T  -INTRO_LINE_HEI; }
      IntroChar  [i].L=-INTRO_WIDTH_MAX*.5               ;
      IntroChar  [i].R= IntroChar[i  ].L  +       charWid; }
    else {
      IntroChar  [i].T= IntroChar[i-1].T                 ;
      IntroChar  [i].B= IntroChar[i-1].B                 ;
      IntroChar  [i].L= IntroChar[i-1].R  +INTRO_CHAR_SEP;
      IntroChar  [i].R= IntroChar[i  ].L  +       charWid; }
    if (IntroChar[i].R> INTRO_WIDTH_MAX*.5 && IntroText[i]!=' ') {
      //  (word-wrap -- backtrack characters out of the current row)
      afterSpace=i; while (afterSpace>rowStart && IntroText[afterSpace-1]!=' ') afterSpace--;
      if (afterSpace>rowStart) i=afterSpace;
      rowStart=i;
      //  (shove all rows up)
      for (j=0; j<i; j++) {
        IntroChar[j].T+=(INTRO_LINE_HEI+INTRO_LINE_SEP)*.5;
        IntroChar[j].B+=(INTRO_LINE_HEI+INTRO_LINE_SEP)*.5; }
      i--; }
    i++; }
  IntroChars=i;
  //  (center the whole block text horizontally, while still keeping it left-justified)
  for (i=0; i<IntroChars; i++) if (IntroText[i]!=' ' && rightEdge<IntroChar[i].R) rightEdge=IntroChar[i].R;
  for (i=0; i<IntroChars; i++) {
    IntroChar[i].L+=(INTRO_WIDTH_MAX*.5-rightEdge)*.5;
    IntroChar[i].R+=(INTRO_WIDTH_MAX*.5-rightEdge)*.5; }

  //  Success.
  return nil; }



//  Renders one pixel of the Intro image.

void drawIntroPixel(BYTE *r, BYTE *g, BYTE *b, int4 pixelX, int4 pixelY,
int4 *cbfChar, BYTE *cbfFill, short *cbfX, short *cbfY, bool *cbfRaw, BYTE **cbfData) {

  double  subPixelPart=1./((double) RasterSubPixels*(double) RasterSubPixels) ;
  double  pixelFrac, screenX, screenY, subPixelPartOver255=subPixelPart/255.  ;
  int4    i, c, ii, jj, introPixelX, introPixelY, *ptrToLong ;
  BYTE    glyphLevel ;

  //  Letterbox to black, if appropriate for this pixel.
  if (pixelY< ImageHeiLetterBox
  ||  pixelY>=ImageHeiLetterBox+ImageHeiUsed) {
    *r=(BYTE) 0;
    *g=(BYTE) 0;
    *b=(BYTE) 0; }
  else {

    //  Construct the image pixel.
    pixelFrac=0.;
    for   (jj=0L; jj<(int4) RasterSubPixels; jj++) {   //    vertical subpixel loop
      screenY  =pixelToScreenY((double) pixelY+((double) jj+.5)/(double) RasterSubPixels);
      for (ii=0L; ii<(int4) RasterSubPixels; ii++) {   //  horizontal subpixel loop
        screenX=pixelToScreenX((double) pixelX+((double) ii+.5)/(double) RasterSubPixels);

        //  Intro text.
        for (i=0; i<IntroChars; i++) {
          if (inDoubleRect(IntroChar[i],screenX,screenY)) {
            c=IntroText[i];
            introPixelX=(int4) ((double) cbfWidth (CbfIntro,&c)
            *(screenX-IntroChar[i].L)/(IntroChar[i].R-IntroChar[i].L));
            introPixelY=(int4) ((double) cbfHeight(CbfIntro)*(IntroChar[i].T-screenY)
            /(IntroChar[i].T-IntroChar[i].B));
            //  Re-populate the cbf (font) decompression variables, if they’re not in a state to use as-is.
            if ((int4) cbfY[jj]!=introPixelY || cbfChar[jj]!=c || (int4) cbfX[jj]>introPixelX) {
              ptrToLong=(int4 *) &CbfIntro[8L+4L*(int4) c];
              cbfChar[jj]=c; cbfFill[jj]=(BYTE) 0; cbfX[jj]=0; cbfY[jj]=(short) introPixelY; cbfRaw[jj]=NO;
              ptrToLong=(int4 *) (CbfIntro+(*ptrToLong)+4L+4L*cbfY[jj]);
              cbfData[jj]=CbfIntro+(*ptrToLong); }
            //  First, find the region (“fill” or “raw”) that includes the desired pixel.
            while (cbfX[jj]+ (short) cbfData[jj][0]<=introPixelX) {
              cbfX     [jj]+=(short) cbfData[jj][0];
              if (cbfRaw[jj]) {   //   “raw” mode
                if (cbfData[jj][0]) {
                  cbfData[jj]+=cbfData[jj][0];
                  if (cbfData[jj][0]<(BYTE) 128) cbfFill[jj]=(BYTE)   0;
                  else                           cbfFill[jj]=(BYTE) 255; }
                else {
                  cbfFill[jj]=(BYTE) 255-cbfFill[jj]; }
                cbfData[jj]++; cbfRaw[jj]=NO; }
              else {              //  “fill” mode
                if (cbfData[jj][0]==(BYTE) 255) {
                  cbfData[jj]++; }
                else {
                  cbfData[jj]++; cbfRaw[jj]=YES; }}}
            //  Then, determine the greyscale level (0-255) of that pixel.
            if (cbfRaw[jj]) {     //   “raw” mode
              glyphLevel=cbfData[jj][1+introPixelX-cbfX[jj]]; }
            else {                //  “fill” mode
              glyphLevel=cbfFill[jj]; }
            //  Use the pixel value that was found.
            pixelFrac+=(double) glyphLevel*subPixelPartOver255; }}}}

    //  Put the calculated pixel value into the displayable bitmap.
    *r=(BYTE) (pixelFrac*(double) INTRO_COLOR_R+.5);
    *g=(BYTE) (pixelFrac*(double) INTRO_COLOR_G+.5);
    *b=(BYTE) (pixelFrac*(double) INTRO_COLOR_B+.5); }}
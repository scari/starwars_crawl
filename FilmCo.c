//  Star Wars TSG
//  Darel Rex Finley, 2006-2011



#include                    "nil.h"
#include                   "bool.h"
#include            "definitions.h"
#include             "FilmCoFont.h"
#include          "SequenceTimes.h"
#include      "Shared Structures.h"
#include "image buffer functions.h"
#include     "geometry functions.h"
#include      "numeric functions.h"
#include       "string functions.h"
#include                   "math.h"
#include                 "stdlib.h"

#include                 "FilmCo.h"



//  Bell-curve data used to create white sparkles over Film Company logo.
double   FilmCoBellDimensionH=(int4) (IMAGE_WID_MAX/FILMCO_BELL_DIVISOR_H*2.+2.) ;
double   FilmCoBellDimensionV=(int4) (IMAGE_HEI_MAX/FILMCO_BELL_DIVISOR_V*2.+2.) ;
double  *FilmCoBellH         =nil ;   //  (dynamically allocated, and dimensioned with FilmCoBellDimensionH)
double  *FilmCoBellV         =nil ;   //  (dynamically allocated, and dimensioned with FilmCoBellDimensionV)
double   FilmCoBellRadHmax        ;
double   FilmCoBellRadVmax        ;
double   FilmCoBellRadH           ;
double   FilmCoBellRadV           ;



//  Array of information extracted from the Film Company font at app launch.
//
//  Having this data in this array helps to improve rendering speed.
//
FilmCoGlyph_  *FilmCoGlyph=NULL ;
int4           FilmCoGlyphs     ;



//  Globals used during the rendering of the Film Company image.  (Populated primarily by
//  prepareToDrawFilmCoImage; used primarily by drawFilmCoPixel and drawFilmCoPreviewPixel.)
double          FilmCoFade ;   //  (ranges from 0 to 1, and represents the fade from one Film Co image to another, or the fade from black to/from a Film Co image)
double          FilmCoProgress, FilmCoSparkleDur ;
FilmCoChar_     FilmCoChar   [MAX_CHARS_FILM_CO    ] ;   //  Information about the placement of each character of the Film Company logo’s large (main) text.
FilmCoIncChar_  FilmCoIncChar[MAX_CHARS_FILM_CO_INC] ;   //  Information about the placement of each character of the Film Company logo’s small (Inc) text.
double          BaseArcFrillInvSlope, BaseArcFrillRad, BaseArcFrillRad2, BaseArcFrillCapX, BaseArcFrillCapY ;
double          BaseArcRad, BaseArcRad2, BaseArcLimitL, BaseArcLimitR, BaseArcIncRad, BaseArcIncRad2 ;
int4            FilmCoText   [MAX_CHARS_FILM_CO    +1] ;
int4            FilmCoIncText[MAX_CHARS_FILM_CO_INC+1] ;
int4            FilmCoChars, FilmCoIncChars ;



//  FilmCo sparkles -- built by the constructSparkles function.
Sparkle_  Sparkle[FILMCO_SPARKLES] ;



//  Array of random numbers used to make the brushed-metal look of the Film Company’s core.
BYTE  MetalRnd[5000] ;



//  Build the Sparkle array.  (Called only once, on app launch.)
//
//  Note:  minStartI and maxStartI are initialized only to prevent compiler warnings -- they shouldn’t need to be.

void constructSparkles() {

  double  windowStart, windowEnd, windowSize, filmCoTimeStart=0., minStart=1., maxStart=0. ;
  int4    i, minStartI=0, maxStartI=0 ;

  //  Seed the random number generator.
  srandomdev();

  //  Built an array of sparkles that start at random times and occur at random locations over the Film Co logo.
  for (i=0; i<FILMCO_FADE_UP; i++) filmCoTimeStart+=getSequenceTime(i);
  setFilmCoProgressByMovieTime(FILMCO_SPARKLES_BEGIN                  ); windowStart     =FilmCoProgress;
  setFilmCoProgressByMovieTime(FILMCO_SPARKLES_END                    ); windowEnd       =FilmCoProgress;
  setFilmCoProgressByMovieTime(filmCoTimeStart+FILMCO_SPARKLE_DURATION); FilmCoSparkleDur=FilmCoProgress;
  windowSize=windowEnd-FilmCoSparkleDur-windowStart;
  for (i=0; i<FILMCO_SPARKLES; i++) {
    Sparkle[i].x    =FILMCO_TEXT_WIDTH *(rnd0to1()-.5);
    Sparkle[i].y    =FILMCO_TEXT_BOTTOM+(FILMCO_TEXT_TOP_OVERPEAK-FILMCO_TEXT_BOTTOM)*rnd0to1();
    Sparkle[i].start=windowStart+rnd0to1()*.5*windowSize+rnd0to1()*.5*windowSize;
    if (Sparkle[i].start<minStart) {
      minStart=Sparkle[i].start; minStartI=i; }
    if (Sparkle[i].start>maxStart) {
      maxStart=Sparkle[i].start; maxStartI=i; }}

  //  Ensure that at least one sparkle hits the very start and very end of the sparkle time-window.
  Sparkle[minStartI].start=windowStart           ;
  Sparkle[maxStartI].start=windowStart+windowSize; }



//  Build the MetalRnd array.  (Called only once, on app launch.)

void generateMetalScratches() {

  int4  i ;

  //  Seed the random number generator.
  srandomdev();

  //  Generate a set of random numbers that will be used to generate the brushed-metal look
  //  in a consistent way (i.e., the randomness will not change when each frame is rendered).
  for (i=0L; i<sizeof(MetalRnd)/sizeof(MetalRnd[0]); i++) {
    MetalRnd[i]=(BYTE) (
    (double)  METAL_SCRATCH_LEVEL_LO+
    (double) (METAL_SCRATCH_LEVEL_HI-METAL_SCRATCH_LEVEL_LO)*rnd0to1()+.5); }}



//  Convert a movie-time value (how many movie seconds have elapsed since the entire opening sequence
//  began) to a fraction representing progress through the Film Company segment of the sequence.
//  FilmCoProgress ranges from 0 to 1, and represents the progress through the entire Film Co segment.
//
//  Note:  This should not happen during the execution of this app, but if for some reason this
//         function is passed a movieTime value outside the Film Company segment of the full opening
//         sequence, FilmCoProgress will be set to a value outside the range 0-1.

void setFilmCoProgressByMovieTime(double movieTime) {

  double  filmCoTimeStart=0., filmCoTimeEnd=0. ;
  int4    i ;

  for (i=0; i< FILMCO_FADE_UP  ; i++) filmCoTimeStart+=getSequenceTime(i);
  for (i=0; i<=FILMCO_FADE_DOWN; i++) filmCoTimeEnd  +=getSequenceTime(i);
  FilmCoProgress=(movieTime-filmCoTimeStart)/(filmCoTimeEnd-filmCoTimeStart); }



//  Forces the area of the image in which the Film Company logo (and its sparkles) resides to be re-rendered.
//
//  Note:  This function assumes that the distance-based bands whose reach is defined by FILMCO_BORDER_REACH
//         will not reach further than the gaussian aura blur.
//
//  Note:  The controller never calls this function with render threads running.

void forceRasterFilmCo(bool lo, bool hi) {
  forceRasterRect(lo,hi,
  screenToPixelX(-FILMCO_TEXT_WIDTH    /2.)-FilmCoBellRadHmax-2,
  screenToPixelY( FILMCO_TEXT_TOP_OVERPEAK)-FilmCoBellRadVmax-2,
  screenToPixelX( FILMCO_TEXT_WIDTH    /2.)+FilmCoBellRadHmax+3,
  screenToPixelY( FILMCO_TEXT_BOTTOM      )+FilmCoBellRadVmax+3); }



//  Compresses the Film Company font serifs (vertically) so they look normal even though the character may be
//  stretched vertically to fit in the logo.  See the file “FilmCo Glyph Constants”.
//
//  Note:  “y” must be in movie-screen coordinates, not glyph coordinates -- see the file “Coordinate Systems”.

void compressFilmCoGlyphSections(
double *y, double charL, double charT, double charR, double charB, double glyphWid,
double sectLo, double sectHi, double yExpand) {

  double  mid, yCompression=(charR-charL)/(charT-charB)/glyphWid*(2.*FILMCO_GLYPH_SCALE)*yExpand ;

  if (yCompression>=1.) return;   //  (sections may only compress, not expand)

  //  Convert sectLo and sectHi from the glyph-definition range (-FILMCO_GLYPH_SCALE
  //  to FILMCO_GLYPH_SCALE) to the movie-screen range (charB to charT).
  sectLo=charB+(charT-charB)*(.5+sectLo/(2.*FILMCO_GLYPH_SCALE));
  sectHi=charB+(charT-charB)*(.5+sectHi/(2.*FILMCO_GLYPH_SCALE));

  if      ((*y)<=sectLo) {   //  bottom section
    (*y)=((*y)-charB)*yCompression+charB; }

  else if ((*y)> sectHi) {   //     top section
    (*y)=((*y)-charT)*yCompression+charT; }

  else                   {   //  center section
    mid=(sectLo+sectHi)*.5;
    (*y)=((*y)-mid  )*yCompression+mid  ; }}



//  Call this function for all Film Company line segments (not for splines) -- it will move only those endpoints that need to
//  be moved.  If the line segment also needs to be turned into an arc, this function will return YES instead of NO, and will
//  set points bX,bY and cX,cY so they are ready to be used with the ARC_AUTO technique.  (Points bX,bY and cX,cY will be
//  almost the same as the new points sX,sY and eX,eY, respectively.  The purpose of bX,bY and cX,cY is to assign direction
//  at the start and end of the arc.)
//
//  Set only points sX,sY and eX,eY when calling this function.  (Points sX,sY and eX,eY must be in movie-screen coordinates,
//  not glyph coordinates -- see the file “Coordinate Systems”.)
//
//  This function should be called after, not before, calling compressFilmCoGlyphSections on the line segment’s endpoints.
//  See the file “FilmCo Glyph Constants”.

bool wrapLineAroundBaseArc(
double *sX, double *sY, double *bX, double *bY, double *cX, double *cY, double *eX, double *eY,
double charL, double charR, double charB, double sectLo) {

  double  dX, newBottom, tinySegment=.0001 ;
  bool    newPoints=NO ;

  sectLo=charB+(FILMCO_TEXT_TOP-charB)*(.5+sectLo/(2.*FILMCO_GLYPH_SCALE));

  if ( (*sY)<sectLo && (*sY)==(*eY)
  &&  ((*sX)<BaseArcLimitR || (*eX)<BaseArcLimitR)
  &&  ((*sX)>BaseArcLimitL || (*eX)>BaseArcLimitL)) {   //  Create points bX,bY and cX,cY.

    newPoints=YES;

    //  Shift sX and/or eX inward, if necessary to keep the arc above the text bottom.  (This shift creates a horizontal
    //  gap, but that’s OK since horizontal line segments don’t affect the point-in-polygon algorithm being used here.)
    if ((*sX)<BaseArcLimitL && (*eX)>BaseArcLimitL) (*sX)=BaseArcLimitL;
    if ((*eX)<BaseArcLimitL && (*sX)>BaseArcLimitL) (*eX)=BaseArcLimitL;
    if ((*eX)>BaseArcLimitR && (*sX)<BaseArcLimitR) (*eX)=BaseArcLimitR;
    if ((*sX)>BaseArcLimitR && (*eX)<BaseArcLimitR) (*sX)=BaseArcLimitR;

    if ((*sX)>(*eX)) tinySegment*=-1.;
    (*bX)=(*sX)+tinySegment; (*bY)=(*sY);
    (*cX)=(*eX)-tinySegment; (*cY)=(*eY);
    dX=-(*bX); newBottom=FILMCO_ARC_CENTER_Y+sqrt(BaseArcRad2-dX*dX); (*bY)-=charB-newBottom;
    dX=-(*cX); newBottom=FILMCO_ARC_CENTER_Y+sqrt(BaseArcRad2-dX*dX); (*cY)-=charB-newBottom; }

  if ((*sY)<sectLo) {
    dX=-(*sX); newBottom=FILMCO_ARC_CENTER_Y+sqrt(BaseArcRad2-dX*dX);
    if (newBottom<FILMCO_TEXT_BOTTOM) newBottom=FILMCO_TEXT_BOTTOM;
    (*sY)-=charB-newBottom; }

  if ((*eY)<sectLo) {
    dX=-(*eX); newBottom=FILMCO_ARC_CENTER_Y+sqrt(BaseArcRad2-dX*dX);
    if (newBottom<FILMCO_TEXT_BOTTOM) newBottom=FILMCO_TEXT_BOTTOM;
    (*eY)-=charB-newBottom; }

  if (newPoints && (*sY)==(*eY) && (*sY)==(*bY)) newPoints=NO;

  return newPoints; }



//  This function is very closely related to wrapLineAroundBaseArc.
//
//  Call this function for all Film Company line segments (not for splines) -- it will move only those endpoints that need to
//  be moved.  If the line segment also needs to be turned into an arc, this function will return YES instead of NO, and will
//  set points bX,bY and cX,cY so they are ready to be used with the ARC_AUTO technique.  (Points bX,bY and cX,cY will be
//  almost the same as the new points sX,sY and eX,eY, respectively.  The purpose of bX,bY and cX,cY is to assign direction
//  at the start and end of the arc.)
//
//  Set only points sX,sY and eX,eY when calling this function.  (Points sX,sY and eX,eY must be in movie-screen coordinates,
//  not glyph coordinates -- see the file “Coordinate Systems”.)
//
//  This function should be called after, not before, calling compressFilmCoGlyphSections on the line segment’s endpoints.
//  See the file “FilmCo Glyph Constants”.

bool wrapLineAroundBaseArcInc(
double *sX, double *sY, double *bX, double *bY, double *cX, double *cY, double *eX, double *eY,
double charL, double charT, double charR, double charB, double sectHi) {

  double  dX, newTop, tinySegment=.0001 ;
  bool    newPoints=NO ;

  sectHi=charB+(charT-charB)*(.5+sectHi/(2.*FILMCO_GLYPH_SCALE));

  if ((*sY)>sectHi && (*sY)==(*eY)) {   //  Create points bX,bY and cX,cY.

    newPoints=YES;

    if ((*sX)>(*eX)) tinySegment*=-1.;
    (*bX)=(*sX)+tinySegment; (*bY)=(*sY);
    (*cX)=(*eX)-tinySegment; (*cY)=(*eY);
    dX=-(*bX); newTop=FILMCO_ARC_CENTER_Y+sqrt(BaseArcIncRad2-dX*dX); (*bY)+=newTop-charT;
    dX=-(*cX); newTop=FILMCO_ARC_CENTER_Y+sqrt(BaseArcIncRad2-dX*dX); (*cY)+=newTop-charT; }

  if ((*sY)>sectHi) {
    dX=-(*sX); newTop=FILMCO_ARC_CENTER_Y+sqrt(BaseArcIncRad2-dX*dX);
    (*sY)+=newTop-charT; }

  if ((*eY)>sectHi) {
    dX=-(*eX); newTop=FILMCO_ARC_CENTER_Y+sqrt(BaseArcIncRad2-dX*dX);
    (*eY)+=newTop-charT; }

  if (newPoints && (*sY)==(*eY) && (*sY)==(*bY)) newPoints=NO;

  return newPoints; }



//  Call this function for all Film Company splines -- it will move only those that need to be moved.
//
//  All points must be in movie-screen coordinates, not local font glyph coordinates -- see the file
//  “Coordinate Systems”.
//
//  This function should be called after, not before, calling compressFilmCoGlyphSections on the line
//  segment’s endpoints.  See the file “FilmCo Glyph Constants”.

void wrapSplineAroundBaseArc(double sX, double *sY, double a, double *b, double eX, double *eY,
double charL, double charR, double charB, double sectLo) {

  double  dX, newBottom ;

  sectLo=charB+(FILMCO_TEXT_TOP-charB)*(.5+sectLo/(2.*FILMCO_GLYPH_SCALE));

  if ((*sY)<sectLo) {
    dX=-sX; newBottom=FILMCO_ARC_CENTER_Y+sqrt(BaseArcRad2   -dX*dX);
    if (newBottom<FILMCO_TEXT_BOTTOM) newBottom=FILMCO_TEXT_BOTTOM;
    (*sY)-=charB-newBottom; }

  if ((* b)<sectLo) {
    dX=- a; newBottom=FILMCO_ARC_CENTER_Y+sqrt(BaseArcRad2   -dX*dX);
    if (newBottom<FILMCO_TEXT_BOTTOM) newBottom=FILMCO_TEXT_BOTTOM;
    (* b)-=charB-newBottom; }

  if ((*eY)<sectLo) {
    dX=-eX; newBottom=FILMCO_ARC_CENTER_Y+sqrt(BaseArcRad2   -dX*dX);
    if (newBottom<FILMCO_TEXT_BOTTOM) newBottom=FILMCO_TEXT_BOTTOM;
    (*eY)-=charB-newBottom; }}



//  Call this function for all Film Company Inc. splines -- it will move only those that need to be moved.
//
//  All points must be in movie-screen coordinates, not local font glyph coordinates -- see the file
//  “Coordinate Systems”.
//
//  This function should be called after, not before, calling compressFilmCoGlyphSections on the line
//  segment’s endpoints.  See the file “FilmCo Glyph Constants”.
//
//  This function is very closely related to wrapSplineAroundBaseArc.

void wrapSplineAroundBaseArcInc(double sX, double *sY, double a, double *b, double eX, double *eY,
double charL, double charT, double charR, double charB, double sectHi) {

  double  dX, newTop ;

  sectHi=charB+(charT-charB)*(.5+sectHi/(2.*FILMCO_GLYPH_SCALE));

  if ((*sY)>sectHi) {
    dX=-sX; newTop=FILMCO_ARC_CENTER_Y+sqrt(BaseArcIncRad2-dX*dX);
    (*sY)+=newTop-charT; }

  if ((* b)>sectHi) {
    dX=- a; newTop=FILMCO_ARC_CENTER_Y+sqrt(BaseArcIncRad2-dX*dX);
    (* b)+=newTop-charT; }

  if ((*eY)>sectHi) {
    dX=-eX; newTop=FILMCO_ARC_CENTER_Y+sqrt(BaseArcIncRad2-dX*dX);
    (*eY)+=newTop-charT; }}



//  Pass in RGB values (as bytes), and get back a double value (0-1) that represents the overall (black & white)
//  brightness of the color.
//
//  See the HSP color model for more information about this transformation:  http://alienryderflex.com/hsp.html

double rgbTOp(BYTE r, BYTE g, BYTE b) {
  return sqrt(
  (double) r*(double) r*(.24117/255./255.)+
  (double) g*(double) g*(.69088/255./255.)+
  (double) b*(double) b*(.06795/255./255.)); }



//  Used by the functions pointInSplineArcPoly and handleArcAuto
//  to handle spline curves when processing a spline polygon.

void handleSplineCurve(double *minGap,
double x, double y, double sX, double sY, double a, double b, double eX, double eY,
double glyphWid, double sectLo, double sectHi, double yExpand, double charL, double charT, double charR, double charB,
bool wrapArc, bool doCompress, bool inc, bool sSpline, bool eSpline, int4 *nodeCount) {

  double  bottomPart, topPart, xPart, sRoot, f, dX, x1, y1, x2, y2, squareY, gap, rad2 ;

  //  Undo stretching of the content of each of the glyph‚Äôs sub-sections (top, middle, and
  //  bottom).  (See the file “FilmCo Glyph Constants”.)
  if (doCompress) {
    compressFilmCoGlyphSections(&sY,charL,charT,charR,charB,glyphWid,sectLo,sectHi,yExpand);
    compressFilmCoGlyphSections(& b,charL,charT,charR,charB,glyphWid,sectLo,sectHi,yExpand);
    compressFilmCoGlyphSections(&eY,charL,charT,charR,charB,glyphWid,sectLo,sectHi,yExpand); }

  //  Adjust to account for the base arc that curves under the bottom of the text.  (See the
  //  file “FilmCo Glyph Constants”.)
  if (wrapArc) {
    if (inc) wrapSplineAroundBaseArcInc(sX,&sY,a,&b,eX,&eY,charL,charT,charR,charB,sectHi);
    else     wrapSplineAroundBaseArc   (sX,&sY,a,&b,eX,&eY,charL,      charR,charB,sectLo); }

  //  Interpolate hard corners if needed.
  if (sSpline) {
    sX=(sX+a)*.5; sY=(sY+b)*.5; }
  if (eSpline) {
    eX=(eX+a)*.5; eY=(eY+b)*.5; }

  if (!minGap) {

    //  Do the spline-curve math.
    bottomPart=2.*(sY+eY-b-b);
    if (bottomPart==0.) {   //  prevent division-by-zero
      b+=.0001; bottomPart-=.0004; }
    sRoot=2.*(b-sY); sRoot*=sRoot; sRoot-=2.*bottomPart*(sY-y);
    if (sRoot>=0.) {
      sRoot=sqrt(sRoot); topPart=2.*(sY-b);
      f=(topPart+sRoot)/bottomPart;
      if (f>=0. && f<=1.) {
        xPart=sX+f*(a-sX); if (xPart+f*(a+f*(eX-a)-xPart)<x) (*nodeCount)++; }
      f=(topPart-sRoot)/bottomPart;
      if (f>=0. && f<=1.) {
        xPart=sX+f*(a-sX); if (xPart+f*(a+f*(eX-a)-xPart)<x) (*nodeCount)++; }}}

  else {

    if (inc) rad2=BaseArcIncRad2;
    else     rad2=BaseArcRad2   ;
    for (f=0.; f<=1.05; f+=.1) {   // Arbitrarily test 11 points.
      x1=sX+f*( a-sX);
      y1=sY+f*( b-sY);
      x2= a+f*(eX- a);
      y2= b+f*(eY- b);
      x =x1+f*(x2-x1);
      y =y1+f*(y2-y1); dX=-x; squareY=rad2-dX*dX;
      if (squareY>=0.) {
        gap=y-(FILMCO_ARC_CENTER_Y+sqrt(squareY)); if (inc) gap*=-1.;
        if (gap<(*minGap)) (*minGap)=gap; }}}}



//  Closely related to handleSplineCurve, but used for building a row of
//  nodes that can be used to dramatically accelerate rendering speed.

void handleSplineCurveForNodesRow(
double y, double sX, double sY, double a, double b, double eX, double eY,
double glyphWid, double sectLo, double sectHi, double yExpand, double charL, double charT, double charR, double charB,
bool wrapArc, bool doCompress, bool inc, bool sSpline, bool eSpline, int4 subPixelRow, int4 polyCount, int4 *nodes,
double *nodeX, int4 *polyTag) {

  double  bottomPart, topPart, xPart, sRoot, f ;
  int4    base ;

  //  Undo stretching of the content of each of the glyph’s sub-sections (top, middle, and
  //  bottom).  (See the file ‚ÄúFilmCo Glyph Constants‚Äù.)
  if (doCompress) {
    compressFilmCoGlyphSections(&sY,charL,charT,charR,charB,glyphWid,sectLo,sectHi,yExpand);
    compressFilmCoGlyphSections(& b,charL,charT,charR,charB,glyphWid,sectLo,sectHi,yExpand);
    compressFilmCoGlyphSections(&eY,charL,charT,charR,charB,glyphWid,sectLo,sectHi,yExpand); }

  //  Adjust to account for the base arc that curves under the bottom of the text.  (See the
  //  file ‚ÄúFilmCo Glyph Constants‚Äù.)
  if (wrapArc) {
    if (inc) wrapSplineAroundBaseArcInc(sX,&sY,a,&b,eX,&eY,charL,charT,charR,charB,sectHi);
    else     wrapSplineAroundBaseArc   (sX,&sY,a,&b,eX,&eY,charL,      charR,charB,sectLo); }

  //  Interpolate hard corners if needed.
  if (sSpline) {
    sX=(sX+a)*.5; sY=(sY+b)*.5; }
  if (eSpline) {
    eX=(eX+a)*.5; eY=(eY+b)*.5; }

  //  Do the spline-curve math.
  bottomPart=2.*(sY+eY-b-b);
  if (bottomPart==0.) {   //  prevent division-by-zero
    b+=.0001; bottomPart-=.0004; }
  sRoot=2.*(b-sY); sRoot*=sRoot; sRoot-=2.*bottomPart*(sY-y);
  if (sRoot>=0.) {
    sRoot=sqrt(sRoot); topPart=2.*(sY-b);
    f=(topPart+sRoot)/bottomPart;
    if (f>=0. && f<=1.) {
      xPart=sX+f*(a-sX);
      if (nodes[subPixelRow]<NODE_ROW_MAX) {
        base=subPixelRow*NODE_ROW_MAX;
        nodeX  [base+nodes[subPixelRow]]=xPart+f*(a+f*(eX-a)-xPart);
        polyTag[base+nodes[subPixelRow]]=polyCount; nodes[subPixelRow]++; }}
    f=(topPart-sRoot)/bottomPart;
    if (f>=0. && f<=1.) {
      xPart=sX+f*(a-sX);
      if (nodes[subPixelRow]<NODE_ROW_MAX) {
        base=subPixelRow*NODE_ROW_MAX;
        nodeX  [base+nodes[subPixelRow]]=xPart+f*(a+f*(eX-a)-xPart);
        polyTag[base+nodes[subPixelRow]]=polyCount; nodes[subPixelRow]++; }}}}



//  Closely related to handleSplineCurve, but used for determining the distance to a spline,
//  not whether the test point is inside/outside of a spline polygon.

void handleSplineCurveForDist(double *minDist, double x, double y,
double sX, double sY, double a, double b, double eX, double eY,
double glyphWid, double sectLo, double sectHi, double yExpand, double charL, double charT, double charR, double charB,
bool wrapArc, bool doCompress, bool inc, bool sSpline, bool eSpline) {

  double  dist ;

  //  Undo stretching of the content of each of the glyph’s sub-sections (top, middle, and
  //  bottom).  (See the file “FilmCo Glyph Constants”.)
  if (doCompress) {
    compressFilmCoGlyphSections(&sY,charL,charT,charR,charB,glyphWid,sectLo,sectHi,yExpand);
    compressFilmCoGlyphSections(& b,charL,charT,charR,charB,glyphWid,sectLo,sectHi,yExpand);
    compressFilmCoGlyphSections(&eY,charL,charT,charR,charB,glyphWid,sectLo,sectHi,yExpand); }

  //  Adjust to account for the base arc that curves under the bottom of the text.  (See the
  //  file “FilmCo Glyph Constants”.)
  if (wrapArc) {
    if (inc) wrapSplineAroundBaseArcInc(sX,&sY,a,&b,eX,&eY,charL,charT,charR,charB,sectHi);
    else     wrapSplineAroundBaseArc   (sX,&sY,a,&b,eX,&eY,charL,      charR,charB,sectLo); }

  //  Interpolate hard corners if needed.
  if (sSpline) {
    sX=(sX+a)*.5; sY=(sY+b)*.5; }
  if (eSpline) {
    eX=(eX+a)*.5; eY=(eY+b)*.5; }

  //  Replace the shortest distance if this one is shorter.
  dist=distPointToSpline_HoningMethod(x,y,sX,sY,a,b,eX,eY); if (dist<(*minDist)) (*minDist)=dist; }



//  Determines whether or not a point is inside a spline-arc-polygon.  (See the file “Polygon Constants”
//  for a detailed description of how the polygon tags work.
//
//  * If this function is called with a nil minGap, it returns 0 or 1 to indicate that the point x,y is
//    outside of or inside the polygon, respectively.
//
//  * If this function is called with a non-nil minGap, it ignores x,y and instead sets the variable
//    pointed to by minGap to the smallest vertical gap between the polygon and the base arc.  (The return
//    value is meaningless.)
//
//  This function is closely related to pointInSplinePoly, but with arc capability added.  Also note that
//  in this function (unlike in pointInSplinePoly) x and y are expected to be in movie-screen coordinates,
//  not glyph coordinates -- see the file “Coordinate Systems”.
//
//  See this webpage for an explanation of the point-in-spline-polygon technique:  http://alienryderflex.com/polyspline
//
//  See the files “ARC Points” and “Polygon Constants” for detail about how ARC and ARC_AUTO work.

int4 pointInSplineArcPoly(double *minGap, double *poly, double x, double y,
double charL, double charT, double charR, double charB,
double glyphWid, bool wrapArc, double sectLo, double sectHi, double yExpand, bool overPeak, bool inc) {

  double  sX, sY, eX, eY, a, b, rotBx, rotBy, rotCx, rotCy, rotDx, rotDy, theCos, theSin, centX, centY, f ;
  double  arcSplineBx, arcSplineBy, arcSplineCx, arcSplineCy, arcHardX, arcHardY, sectX, sectY, dist, gap ;
  double  arcAx, arcAy, arcBx, arcBy, arcCx, arcCy, arcDx, arcDy, abExtend, dcExtend, dX, dY, distAB      ;
  double  bX, bY, cX, cY, charWid=charR-charL, squareY, rad2, preCalc ;
  int4    i=0, j, k, start=0, nodeCount=0 ;
  bool    sSpline, eSpline ;
  bool    newPointsMade=NO ;   //  (Initialized only to prevent a compiler warning; it shouldn’t need to be.)

  y+=.000001;   //  (Prevent the need for special tests when f is exactly 0 or 1.)

  if (overPeak) {
    charT=charB+(charT-charB)*(FILMCO_TEXT_TOP_OVERPEAK-FILMCO_TEXT_BOTTOM)/(FILMCO_TEXT_TOP-FILMCO_TEXT_BOTTOM); }

  while (poly[i]!=END_POLY) {
    j=i+2; if (poly[i]==SPLINE) j++;
    if (poly[j]==END_POLY || poly[j]==NEW_LOOP || poly[j]==NEW_POLY) j=start;

    if   (poly[j]==ARC || poly[j]==ARC_AUTO) {       //  ARC
      if (poly[j]==ARC_AUTO) {
        //  Get the two line segments that will be connected by an arc.  Order:  A, B, arc, C, D
        arcAx=poly[i-2]; arcAy=poly[i-1];
        arcBx=poly[i  ]; arcBy=poly[i+1]; j++ ; if (poly[j]==END_POLY || poly[j]==NEW_LOOP || poly[j]==NEW_POLY) j=start;
        arcCx=poly[j  ]; arcCy=poly[j+1]; j+=2; if (poly[j]==END_POLY || poly[j]==NEW_LOOP || poly[j]==NEW_POLY) j=start;
        arcDx=poly[j  ]; arcDy=poly[j+1];
        //  Discover the length of line segment AB.
        dX=arcBx-arcAx; dY=arcBy-arcAy; distAB=sqrt(dX*dX+dY*dY);
        //  Translate the line segments so that point B is at the origin.
        rotCx=arcCx-arcBx; rotCy=arcCy-arcBy;
        rotDx=arcDx-arcBx; rotDy=arcDy-arcBy;
        //  Rotate the line segments so that point A is on the positive, X axis.
        theCos=(arcAx-arcBx)/distAB;
        theSin=(arcAy-arcBy)/distAB;
        rotatePointAroundOrigin(&rotCx,&rotCy,theCos,-theSin);
        rotatePointAroundOrigin(&rotDx,&rotDy,theCos,-theSin);
        //  Discover the extenders.
        dcExtend=rotCy/(rotDy-rotCy); sectX=rotCx+(rotCx-rotDx)*dcExtend;
        abExtend=-sectX/distAB;
        //  Discover the two spline corners created by the extenders.
        arcSplineBx=arcBx+(arcBx-arcAx)*abExtend*ARC_FRAC;
        arcSplineBy=arcBy+(arcBy-arcAy)*abExtend*ARC_FRAC;
        arcSplineCx=arcCx+(arcCx-arcDx)*dcExtend*ARC_FRAC;
        arcSplineCy=arcCy+(arcCy-arcDy)*dcExtend*ARC_FRAC; }
      else {
        //  Get the two endpoints (B and C), and the arc-center.
        arcBx=poly[i  ]; arcBy=poly[i+1];
        centX=poly[j+1]; centY=poly[j+2]; j+=3; if (poly[j]==END_POLY || poly[j]==NEW_LOOP || poly[j]==NEW_POLY) j=start;
        arcCx=poly[j  ]; arcCy=poly[j+1];
        //  Discover the tangent-intersection point.  (See the file “Finding Tangent Intersect”.)
        rotBx=arcBx-centX;
        rotBy=arcBy-centY; dist  =sqrt(rotBx*rotBx+rotBy*rotBy);
        rotCx=arcCx-centX; theCos=rotBx/dist;
        rotCy=arcCy-centY; theSin=rotBy/dist;
        rotatePointAroundOrigin(&rotCx,&rotCy,theCos,-theSin); sectX=dist; sectY=rotCy-(dist-rotCx)*rotCx/rotCy;
        rotatePointAroundOrigin(&sectX,&sectY,theCos, theSin);
        sectX+=centX;
        sectY+=centY;
        //  Discover the two spline corners.
        arcSplineBx=arcBx+(sectX-arcBx)*ARC_FRAC;
        arcSplineBy=arcBy+(sectY-arcBy)*ARC_FRAC;
        arcSplineCx=arcCx+(sectX-arcCx)*ARC_FRAC;
        arcSplineCy=arcCy+(sectY-arcCy)*ARC_FRAC; }
      //  Discover the new, hard corner inbetween the two spline corners.
      arcHardX=(arcSplineBx+arcSplineCx)*.5;
      arcHardY=(arcSplineBy+arcSplineCy)*.5;
      //  Convert the spline’s glyph coordinates to movie-screen coordinates -- see the file “Coordinate Systems”.
      preCalc=charWid/glyphWid;
      arcBx      =(arcBx      +16.)*preCalc+charL;
      arcSplineBx=(arcSplineBx+16.)*preCalc+charL;
      arcHardX   =(arcHardX   +16.)*preCalc+charL;
      arcSplineCx=(arcSplineCx+16.)*preCalc+charL;
      arcCx      =(arcCx      +16.)*preCalc+charL; preCalc=(charT-charB)/(2.*FILMCO_GLYPH_SCALE);
      arcBy      =(arcBy      +16.)*preCalc+charB;
      arcSplineBy=(arcSplineBy+16.)*preCalc+charB;
      arcHardY   =(arcHardY   +16.)*preCalc+charB;
      arcSplineCy=(arcSplineCy+16.)*preCalc+charB;
      arcCy      =(arcCy      +16.)*preCalc+charB;
      //  Process the two spline curves that simulate an arc.
      handleSplineCurve(minGap,x,y,arcBx   ,arcBy   ,arcSplineBx,arcSplineBy,arcHardX,arcHardY,
      glyphWid,sectLo,sectHi,yExpand,charL,charT,charR,charB,wrapArc,YES,inc,NO,NO,&nodeCount);
      handleSplineCurve(minGap,x,y,arcHardX,arcHardY,arcSplineCx,arcSplineCy,arcCx   ,arcCy   ,
      glyphWid,sectLo,sectHi,yExpand,charL,charT,charR,charB,wrapArc,YES,inc,NO,NO,&nodeCount); }

    else if (poly[i]!=SPLINE && poly[j]!=SPLINE) {   //  LINE SEGMENT
      sX=poly[i]; sY=poly[i+1];
      eX=poly[j]; eY=poly[j+1];
      //  Convert the line’s glyph coordinates to movie-screen coordinates -- see the file “Coordinate Systems”.
      preCalc=charWid/glyphWid;
      sX=(sX+16.)*preCalc+charL;
      eX=(eX+16.)*preCalc+charL; preCalc=(charT-charB)/(2.*FILMCO_GLYPH_SCALE);
      sY=(sY+16.)*preCalc+charB;
      eY=(eY+16.)*preCalc+charB;
      //  Undo stretching of the content of each of the glyph’s sub-sections (top, middle, and
      //  bottom).  (See the file “FilmCo Glyph Constants”.)
      compressFilmCoGlyphSections(&sY,charL,charT,charR,charB,glyphWid,sectLo,sectHi,yExpand);
      compressFilmCoGlyphSections(&eY,charL,charT,charR,charB,glyphWid,sectLo,sectHi,yExpand);
      //  Wrap the line around the base arc.
      if (wrapArc) {
        if (inc) newPointsMade=wrapLineAroundBaseArcInc(&sX,&sY,&bX,&bY,&cX,&cY,&eX,&eY,charL,charT,charR,charB,sectHi);
        else     newPointsMade=wrapLineAroundBaseArc   (&sX,&sY,&bX,&bY,&cX,&cY,&eX,&eY,charL,      charR,charB,sectLo);
        if (newPointsMade) {
          if (!minGap) {
            handleLineSegment(x,y,sX,sY,bX,bY,&nodeCount);
            handleLineSegment(x,y,cX,cY,eX,eY,&nodeCount); }
          handleArcAuto(
          minGap,x,y,sX,sY,bX,bY,cX,cY,eX,eY,charL,charT,charR,charB,charWid,glyphWid,yExpand,sectLo,sectHi,inc,&nodeCount); }}
      if (!wrapArc || !newPointsMade) {
        if (minGap) {
          if (inc) rad2=BaseArcIncRad2;
          else     rad2=BaseArcRad2   ;
          for (f=0.; f<=1.05; f+=.1) {   //  (Arbitrarily test 11 points across the straight line segment.)
            x=sX+f*(eX-sX);
            y=sY+f*(eY-sY); dX=-x; squareY=rad2-dX*dX;
            if (squareY>=0.) {
              gap=y-(FILMCO_ARC_CENTER_Y+sqrt(squareY)); if (inc) gap*=-1.;
              if (gap<(*minGap)) (*minGap)=gap; }}}
        else handleLineSegment(x,y,sX,sY,eX,eY,&nodeCount); }}

    else if (poly[j]==SPLINE) {                      //  SPLINE CURVE
      a=poly[j+1]; b=poly[j+2]; k=j+3; if (poly[k]==END_POLY || poly[k]==NEW_LOOP || poly[k]==NEW_POLY) k=start;
      if (poly[i]!=SPLINE) {
        sSpline= NO; sX=poly[i  ]; sY=poly[i+1]; }
      else {
        sSpline=YES; sX=poly[i+1]; sY=poly[i+2]; }
      if (poly[k]!=SPLINE) {
        eSpline= NO; eX=poly[k  ]; eY=poly[k+1]; }
      else {
        eSpline=YES; eX=poly[k+1]; eY=poly[k+2]; }
      //  Convert the spline’s glyph coordinates to movie-screen coordinates -- see the file “Coordinate Systems”.
      preCalc=charWid/glyphWid;
      sX=(sX+16.)*preCalc+charL;
      a =( a+16.)*preCalc+charL;
      eX=(eX+16.)*preCalc+charL; preCalc=(charT-charB)/(2.*FILMCO_GLYPH_SCALE);
      sY=(sY+16.)*preCalc+charB;
      b =( b+16.)*preCalc+charB;
      eY=(eY+16.)*preCalc+charB;
      //  Process the spline curve.
      handleSplineCurve(minGap,x,y,sX,sY,a,b,eX,eY,glyphWid,sectLo,sectHi,yExpand,charL,charT,charR,charB,wrapArc,YES,inc,
      sSpline,eSpline,&nodeCount); }

    //  Advance through the polygon data.  (See the file “Spline Corner Logic”.)
    if      (poly[i]==SPLINE  ) i++ ;
    i+=2;
    if      (poly[i]==ARC     ) i+=3;
    else if (poly[i]==ARC_AUTO) i++ ;
    if      (poly[i]==NEW_LOOP) {
      i++; start=i; }
    else if (poly[i]==NEW_POLY) {
      if (nodeCount&1) return 1;
      i++; start=i; }}

  return nodeCount&1; }



//  This function is very closely related to pointInSplineArcPoly (see comments at that function), but it adds a set of
//  nodes to a growing list of nodes for a specified Y threshold.  Used for accellerated (whole-row) image rendering.
//
//  See the file “Polygon Constants” for a detailed description of how the polygon tags work.
//
//  See this webpage for an explanation of whole-row node rendering:  http://alienryderflex.com/polygon_fill
//
//  See this webpage for an explanation of the point-in-spline-polygon technique:  http://alienryderflex.com/polyspline
//
//  See the files “ARC Points” and “Polygon Constants” for detail about how ARC and ARC_AUTO work.

void addNodesFromSplineArcPoly_FilmCo(double *poly, double y,
double charL, double charT, double charR, double charB,
double glyphWid, bool wrapArc, double sectLo, double sectHi, double yExpand, bool overPeak, bool inc, int4 subPixelRow,
int4 *polyCount, int4 *nodes, double *nodeX, int4 *polyTag) {

  double  sX, sY, eX, eY, a, b, rotBx, rotBy, rotCx, rotCy, rotDx, rotDy, theCos, theSin, centX, centY ;
  double  arcAx, arcAy, arcBx, arcBy, arcCx, arcCy, arcDx, arcDy, abExtend, dcExtend, dX, dY, distAB   ;
  double  arcSplineBx, arcSplineBy, arcSplineCx, arcSplineCy, arcHardX, arcHardY, sectX, sectY, dist   ;
  double  bX, bY, cX, cY, charWid=charR-charL, preCalc ;
  int4    i=0, j, k, start=0 ;
  bool    sSpline, eSpline   ;
  bool    newPointsMade=NO   ;   //  (Initialized only to prevent a compiler warning; it shouldn’t need to be.)

  y+=.000001;   //  (Prevent the need for special tests when f -- a variable used by functions this function calls -- is exactly 0 or 1.)

  if (overPeak) {
    charT=charB+(charT-charB)*(FILMCO_TEXT_TOP_OVERPEAK-FILMCO_TEXT_BOTTOM)/(FILMCO_TEXT_TOP-FILMCO_TEXT_BOTTOM); }

  while (poly[i]!=END_POLY) {
    j=i+2; if (poly[i]==SPLINE) j++;
    if (poly[j]==END_POLY || poly[j]==NEW_LOOP || poly[j]==NEW_POLY) j=start;

    if   (poly[j]==ARC || poly[j]==ARC_AUTO) {   //  ARC or ARC_AUTO
      if (poly[j]==ARC_AUTO) {
        //  ** Handle ARC_AUTO **
        //  Get the two line segments that will be connected by an arc.  Order:  A, B, arc, C, D
        arcAx=poly[i-2]; arcAy=poly[i-1];
        arcBx=poly[i  ]; arcBy=poly[i+1]; j++ ; if (poly[j]==END_POLY || poly[j]==NEW_LOOP || poly[j]==NEW_POLY) j=start;
        arcCx=poly[j  ]; arcCy=poly[j+1]; j+=2; if (poly[j]==END_POLY || poly[j]==NEW_LOOP || poly[j]==NEW_POLY) j=start;
        arcDx=poly[j  ]; arcDy=poly[j+1];
        //  Discover the length of line segment AB.
        dX=arcBx-arcAx; dY=arcBy-arcAy; distAB=sqrt(dX*dX+dY*dY);
        //  Translate the line segments so that point B is at the origin.
        rotCx=arcCx-arcBx; rotCy=arcCy-arcBy;
        rotDx=arcDx-arcBx; rotDy=arcDy-arcBy;
        //  Rotate the line segments so that point A is on the positive, X axis.
        theCos=(arcAx-arcBx)/distAB;
        theSin=(arcAy-arcBy)/distAB;
        rotatePointAroundOrigin(&rotCx,&rotCy,theCos,-theSin);
        rotatePointAroundOrigin(&rotDx,&rotDy,theCos,-theSin);
        //  Discover the extenders.
        dcExtend=rotCy/(rotDy-rotCy); sectX=rotCx+(rotCx-rotDx)*dcExtend;
        abExtend=-sectX/distAB;
        //  Discover the two spline corners created by the extenders.
        arcSplineBx=arcBx+(arcBx-arcAx)*abExtend*ARC_FRAC;
        arcSplineBy=arcBy+(arcBy-arcAy)*abExtend*ARC_FRAC;
        arcSplineCx=arcCx+(arcCx-arcDx)*dcExtend*ARC_FRAC;
        arcSplineCy=arcCy+(arcCy-arcDy)*dcExtend*ARC_FRAC; }
      else {
        //  ** Handle ARC **
        //  Get the two endpoints (B and C), and the arc-center.
        arcBx=poly[i  ]; arcBy=poly[i+1];
        centX=poly[j+1]; centY=poly[j+2]; j+=3; if (poly[j]==END_POLY || poly[j]==NEW_LOOP || poly[j]==NEW_POLY) j=start;
        arcCx=poly[j  ]; arcCy=poly[j+1];
        //  Discover the tangent-intersection point.  (See the file “Finding Tangent Intersect”.)
        rotBx=arcBx-centX;
        rotBy=arcBy-centY; dist=sqrt(rotBx*rotBx+rotBy*rotBy);
        rotCx=arcCx-centX; theCos=rotBx/dist;
        rotCy=arcCy-centY; theSin=rotBy/dist;
        rotatePointAroundOrigin(&rotCx,&rotCy,theCos,-theSin); sectX=dist; sectY=rotCy-(dist-rotCx)*rotCx/rotCy;
        rotatePointAroundOrigin(&sectX,&sectY,theCos, theSin);
        sectX+=centX;
        sectY+=centY;
        //  Discover the two spline corners.
        arcSplineBx=arcBx+(sectX-arcBx)*ARC_FRAC;
        arcSplineBy=arcBy+(sectY-arcBy)*ARC_FRAC;
        arcSplineCx=arcCx+(sectX-arcCx)*ARC_FRAC;
        arcSplineCy=arcCy+(sectY-arcCy)*ARC_FRAC; }
      //  Discover the new, hard corner inbetween the two spline corners.
      arcHardX=(arcSplineBx+arcSplineCx)*.5;
      arcHardY=(arcSplineBy+arcSplineCy)*.5;
      //  Convert the spline’s glyph coordinates to movie-screen coordinates -- see the file “Coordinate Systems”.
      preCalc=charWid/glyphWid;
      arcBx      =(arcBx      +16.)*preCalc+charL;
      arcSplineBx=(arcSplineBx+16.)*preCalc+charL;
      arcHardX   =(arcHardX   +16.)*preCalc+charL;
      arcSplineCx=(arcSplineCx+16.)*preCalc+charL;
      arcCx      =(arcCx      +16.)*preCalc+charL; preCalc=(charT-charB)/(2.*FILMCO_GLYPH_SCALE);
      arcBy      =(arcBy      +16.)*preCalc+charB;
      arcSplineBy=(arcSplineBy+16.)*preCalc+charB;
      arcHardY   =(arcHardY   +16.)*preCalc+charB;
      arcSplineCy=(arcSplineCy+16.)*preCalc+charB;
      arcCy      =(arcCy      +16.)*preCalc+charB;
      //  Process the two spline curves that simulate an arc.
      handleSplineCurveForNodesRow(y,arcBx   ,arcBy   ,arcSplineBx,arcSplineBy,arcHardX,arcHardY,glyphWid,
      sectLo,sectHi,yExpand,charL,charT,charR,charB,wrapArc,YES,inc,NO,NO,subPixelRow,*polyCount,nodes,nodeX,polyTag);
      handleSplineCurveForNodesRow(y,arcHardX,arcHardY,arcSplineCx,arcSplineCy,arcCx   ,arcCy   ,glyphWid,
      sectLo,sectHi,yExpand,charL,charT,charR,charB,wrapArc,YES,inc,NO,NO,subPixelRow,*polyCount,nodes,nodeX,polyTag); }

    else if (poly[i]!=SPLINE && poly[j]!=SPLINE) {   //  LINE SEGMENT
      sX=poly[i]; sY=poly[i+1];
      eX=poly[j]; eY=poly[j+1];
      //  Convert the line’s glyph coordinates to movie-screen coordinates -- see the file “Coordinate Systems”.
      preCalc=charWid/glyphWid;
      sX=(sX+16.)*preCalc+charL;
      eX=(eX+16.)*preCalc+charL; preCalc=(charT-charB)/(2.*FILMCO_GLYPH_SCALE);
      sY=(sY+16.)*preCalc+charB;
      eY=(eY+16.)*preCalc+charB;
      //  Prevent stretching of the content of each of the glyph’s sub-sections (top, middle, and bottom).
      compressFilmCoGlyphSections(&sY,charL,charT,charR,charB,glyphWid,sectLo,sectHi,yExpand);
      compressFilmCoGlyphSections(&eY,charL,charT,charR,charB,glyphWid,sectLo,sectHi,yExpand);
      //  Wrap the line around the base arc.
      if (wrapArc) {
        if (inc) newPointsMade=wrapLineAroundBaseArcInc(&sX,&sY,&bX,&bY,&cX,&cY,&eX,&eY,charL,charT,charR,charB,sectHi);
        else     newPointsMade=wrapLineAroundBaseArc   (&sX,&sY,&bX,&bY,&cX,&cY,&eX,&eY,charL,      charR,charB,sectLo);
        if (newPointsMade) {
          handleLineSegmentForNodesRow(y,sX,sY,bX,bY,subPixelRow,*polyCount,nodes,nodeX,polyTag);
          handleLineSegmentForNodesRow(y,cX,cY,eX,eY,subPixelRow,*polyCount,nodes,nodeX,polyTag);
          handleArcAutoForNodesRow    (y,sX,sY,bX,bY,cX,cY,eX,eY,charL,charT,charR,charB,charWid,
          glyphWid,yExpand,sectLo,sectHi,inc,        subPixelRow,*polyCount,nodes,nodeX,polyTag); }}
      if (!wrapArc || !newPointsMade) {
        handleLineSegmentForNodesRow  (y,sX,sY,eX,eY,subPixelRow,*polyCount,nodes,nodeX,polyTag); }}

    else if (poly[j]==SPLINE) {   //  SPLINE CURVE
      a=poly[j+1]; b=poly[j+2]; k=j+3; if (poly[k]==END_POLY || poly[k]==NEW_LOOP || poly[k]==NEW_POLY) k=start;
      if (poly[i]!=SPLINE) {
        sSpline= NO; sX=poly[i  ]; sY=poly[i+1]; }
      else {
        sSpline=YES; sX=poly[i+1]; sY=poly[i+2]; }
      if (poly[k]!=SPLINE) {
        eSpline= NO; eX=poly[k  ]; eY=poly[k+1]; }
      else {
        eSpline=YES; eX=poly[k+1]; eY=poly[k+2]; }
      //  Convert the spline’s glyph coordinates to movie-screen coordinates -- see the file “Coordinate Systems”.
      preCalc=charWid/glyphWid;
      sX=(sX+16.)*preCalc+charL;
      a =( a+16.)*preCalc+charL;
      eX=(eX+16.)*preCalc+charL; preCalc=(charT-charB)/(2.*FILMCO_GLYPH_SCALE);
      sY=(sY+16.)*preCalc+charB;
      b =( b+16.)*preCalc+charB;
      eY=(eY+16.)*preCalc+charB;
      //  Process the spline curve.
      handleSplineCurveForNodesRow(y,sX,sY,a,b,eX,eY,glyphWid,sectLo,sectHi,yExpand,charL,charT,charR,charB,wrapArc,YES,inc,
      sSpline,eSpline,subPixelRow,*polyCount,nodes,nodeX,polyTag); }

    //  Advance through the polygon data.  (See the file “Spline Corner Logic”.)
    if      (poly[i]==SPLINE  ) i++ ;
    i+=2;
    if      (poly[i]==ARC     ) i+=3;
    else if (poly[i]==ARC_AUTO) i++ ;
    if      (poly[i]==NEW_POLY) (*polyCount)++;
    if      (poly[i]==NEW_POLY
    ||       poly[i]==NEW_LOOP) {
      i++; start=i; }}}



//  Used by distToFilmCoChar to handle ARC_AUTO -- see that distToFilmCoChar’s comments.
//
//  Very closely related to handleArcAuto.

void handleArcAutoForDist(double *minDist, double x, double y, double aX, double aY, double bX, double bY,
double cX, double cY, double dX, double dY, double charL, double charT, double charR, double charB, double charWid,
double glyphWid, double yExpand, double sectLo, double sectHi, bool inc) {

  double  deltaX, deltaY, distAB, rotCx, rotCy, rotDx, rotDy, theCos, theSin, abExtend, dcExtend, sectX ;
  double  splineBx, splineBy, splineCx, splineCy, hardX, hardY ;

  //  Discover the length of line segment AB.
  deltaX=bX-aX; deltaY=bY-aY; distAB=sqrt(deltaX*deltaX+deltaY*deltaY);

  //  Translate the line segments so that point B is at the origin.
  rotCx=cX-bX; rotCy=cY-bY;
  rotDx=dX-bX; rotDy=dY-bY;

  //  Rotate the line segments so that point A is on the positive, X axis.
  theCos=(aX-bX)/distAB;
  theSin=(aY-bY)/distAB;
  rotatePointAroundOrigin(&rotCx,&rotCy,theCos,-theSin);
  rotatePointAroundOrigin(&rotDx,&rotDy,theCos,-theSin);

  //  Discover the extenders.
  dcExtend=rotCy/(rotDy-rotCy); sectX=rotCx+(rotCx-rotDx)*dcExtend;
  abExtend=-sectX/distAB;

  //  Discover the two spline corners created by the extenders.
  splineBx=bX+(bX-aX)*abExtend*ARC_FRAC;
  splineBy=bY+(bY-aY)*abExtend*ARC_FRAC;
  splineCx=cX+(cX-dX)*dcExtend*ARC_FRAC;
  splineCy=cY+(cY-dY)*dcExtend*ARC_FRAC;

  //  Discover the new, hard corner inbetween the two spline corners.
  hardX=(splineBx+splineCx)*.5;
  hardY=(splineBy+splineCy)*.5;

  //  Process the two spline curves that simulate an arc.
  handleSplineCurveForDist(minDist,x,y,bX   ,bY   ,splineBx,splineBy,hardX,hardY,glyphWid,sectLo,sectHi,yExpand,
  charL,charT,charR,charB,NO,NO,inc,NO,NO);
  handleSplineCurveForDist(minDist,x,y,hardX,hardY,splineCx,splineCy,cX   ,cY   ,glyphWid,sectLo,sectHi,yExpand,
  charL,charT,charR,charB,NO,NO,inc,NO,NO); }



//  Used by drawFilmCoPixel to determine the distance from a test point to the closest edge of any part of the
//  Film Company logo.  This is used to create the banded border around the logo.
//
//  Closely related to pointInSplineArcPoly, but returns the shortest distance from the testpoint
//  to any part of the edge of the polygon.
//
//  See the file “Polygon Constants” for a detailed description of how the polygon tags work.
//
//  See this webpage for an explanation of the point-in-spline-polygon technique:  http://alienryderflex.com/polyspline
//
//  See the files “ARC Points” and “Polygon Constants” for detail about how ARC and ARC_AUTO work.

double distToFilmCoChar(double *poly, double x, double y,
double charL, double charT, double charR, double charB, double glyphWid,
bool wrapArc, double sectLo, double sectHi, double yExpand, bool overPeak, bool inc) {

  double  sX, sY, eX, eY, a, b, rotBx, rotBy, rotCx, rotCy, rotDx, rotDy, theCos, theSin, centX, centY ;
  double  bX, bY, cX, cY, charWid=charR-charL, minDist=INF, newDist, originalSx, originalEx, preCalc   ;
  double  arcAx, arcAy, arcBx, arcBy, arcCx, arcCy, arcDx, arcDy, abExtend, dcExtend, dX, dY, distAB   ;
  double  arcSplineBx, arcSplineBy, arcSplineCx, arcSplineCy, arcHardX, arcHardY, sectX, sectY, dist   ;
  int4    i=0, j, k, start=0 ;
  bool    sSpline, eSpline   ;
  bool    newPointsMade=NO   ;   //  (Initialized only to prevent a compiler warning; it shouldn’t need to be.)

  y+=.000001;   //  (Prevent the need for special tests when F is exactly 0 or 1.)

  if (overPeak) {
    charT=charB+(charT-charB)*(FILMCO_TEXT_TOP_OVERPEAK-FILMCO_TEXT_BOTTOM)/(FILMCO_TEXT_TOP-FILMCO_TEXT_BOTTOM); }

  while (poly[i]!=END_POLY) {
    j=i+2; if (poly[i]==SPLINE) j++;
    if (poly[j]==END_POLY || poly[j]==NEW_LOOP || poly[j]==NEW_POLY) j=start;

    if   (poly[j]==ARC || poly[j]==ARC_AUTO) {   //  ARC
      if (poly[j]==ARC_AUTO) {
        //  Get the two line segments that will be connected by an arc.  Order:  A, B, arc, C, D
        arcAx=poly[i-2]; arcAy=poly[i-1];
        arcBx=poly[i  ]; arcBy=poly[i+1]; j++ ; if (poly[j]==END_POLY || poly[j]==NEW_LOOP || poly[j]==NEW_POLY) j=start;
        arcCx=poly[j  ]; arcCy=poly[j+1]; j+=2; if (poly[j]==END_POLY || poly[j]==NEW_LOOP || poly[j]==NEW_POLY) j=start;
        arcDx=poly[j  ]; arcDy=poly[j+1];
        //  Discover the length of line segment AB.
        dX=arcBx-arcAx; dY=arcBy-arcAy; distAB=sqrt(dX*dX+dY*dY);
        //  Translate the line segments so that point B is at the origin.
        rotCx=arcCx-arcBx; rotCy=arcCy-arcBy;
        rotDx=arcDx-arcBx; rotDy=arcDy-arcBy;
        //  Rotate the line segments so that point A is on the positive, X axis.
        theCos=(arcAx-arcBx)/distAB;
        theSin=(arcAy-arcBy)/distAB;
        rotatePointAroundOrigin(&rotCx,&rotCy,theCos,-theSin);
        rotatePointAroundOrigin(&rotDx,&rotDy,theCos,-theSin);
        //  Discover the extenders.
        dcExtend=rotCy/(rotDy-rotCy); sectX=rotCx+(rotCx-rotDx)*dcExtend;
        abExtend=-sectX/distAB;
        //  Discover the two spline corners created by the extenders.
        arcSplineBx=arcBx+(arcBx-arcAx)*abExtend*ARC_FRAC;
        arcSplineBy=arcBy+(arcBy-arcAy)*abExtend*ARC_FRAC;
        arcSplineCx=arcCx+(arcCx-arcDx)*dcExtend*ARC_FRAC;
        arcSplineCy=arcCy+(arcCy-arcDy)*dcExtend*ARC_FRAC; }
      else {
        //  Get the two endpoints (B and C), and the arc-center.
        arcBx=poly[i  ]; arcBy=poly[i+1];
        centX=poly[j+1]; centY=poly[j+2]; j+=3; if (poly[j]==END_POLY || poly[j]==NEW_LOOP || poly[j]==NEW_POLY) j=start;
        arcCx=poly[j  ]; arcCy=poly[j+1];
        //  Discover the tangent-intersection point.  (See the file “Finding Tangent Intersect”.)
        rotBx=arcBx-centX;
        rotBy=arcBy-centY; dist=sqrt(rotBx*rotBx+rotBy*rotBy);
        rotCx=arcCx-centX; theCos=rotBx/dist;
        rotCy=arcCy-centY; theSin=rotBy/dist;
        rotatePointAroundOrigin(&rotCx,&rotCy,theCos,-theSin); sectX=dist; sectY=rotCy-(dist-rotCx)*rotCx/rotCy;
        rotatePointAroundOrigin(&sectX,&sectY,theCos, theSin);
        sectX+=centX;
        sectY+=centY;
        //  Discover the two spline corners.
        arcSplineBx=arcBx+(sectX-arcBx)*ARC_FRAC;
        arcSplineBy=arcBy+(sectY-arcBy)*ARC_FRAC;
        arcSplineCx=arcCx+(sectX-arcCx)*ARC_FRAC;
        arcSplineCy=arcCy+(sectY-arcCy)*ARC_FRAC; }
      //  Discover the new, hard corner inbetween the two spline corners.
      arcHardX=(arcSplineBx+arcSplineCx)*.5;
      arcHardY=(arcSplineBy+arcSplineCy)*.5;
      //  Convert the spline’s glyph coordinates to movie-screen coordinates -- see the file “Coordinate Systems”.
      preCalc=charWid/glyphWid;
      arcBx      =(arcBx      +16.)*preCalc+charL;
      arcSplineBx=(arcSplineBx+16.)*preCalc+charL;
      arcHardX   =(arcHardX   +16.)*preCalc+charL;
      arcSplineCx=(arcSplineCx+16.)*preCalc+charL;
      arcCx      =(arcCx      +16.)*preCalc+charL; preCalc=(charT-charB)/(2.*FILMCO_GLYPH_SCALE);
      arcBy      =(arcBy      +16.)*preCalc+charB;
      arcSplineBy=(arcSplineBy+16.)*preCalc+charB;
      arcHardY   =(arcHardY   +16.)*preCalc+charB;
      arcSplineCy=(arcSplineCy+16.)*preCalc+charB;
      arcCy      =(arcCy      +16.)*preCalc+charB;
      //  Process the two spline curves that simulate an arc.
      handleSplineCurveForDist(&minDist,x,y,
      arcBx   ,arcBy   ,arcSplineBx,arcSplineBy,arcHardX,arcHardY,glyphWid,sectLo,sectHi,yExpand,charL,charT,charR,charB,
      wrapArc,YES,inc,NO,NO);
      handleSplineCurveForDist(&minDist,x,y,
      arcHardX,arcHardY,arcSplineCx,arcSplineCy,arcCx   ,arcCy   ,glyphWid,sectLo,sectHi,yExpand,charL,charT,charR,charB,
      wrapArc,YES,inc,NO,NO); }

    else if (poly[i]!=SPLINE && poly[j]!=SPLINE) {   //  LINE SEGMENT
      sX=poly[i]; sY=poly[i+1];
      eX=poly[j]; eY=poly[j+1];
      //  Convert the line’s glyph coordinates to movie-screen coordinates -- see the file “Coordinate Systems”.
      preCalc=charWid/glyphWid;
      sX=(sX+16.)*preCalc+charL;
      eX=(eX+16.)*preCalc+charL; preCalc=(charT-charB)/(2.*FILMCO_GLYPH_SCALE);
      sY=(sY+16.)*preCalc+charB;
      eY=(eY+16.)*preCalc+charB;
      //  Undo stretching of the content of each of the glyph’s sub-sections (top, middle, and
      //  bottom).  (See the file “FilmCo Glyph Constants”.)
      compressFilmCoGlyphSections(&sY,charL,charT,charR,charB,glyphWid,sectLo,sectHi,yExpand);
      compressFilmCoGlyphSections(&eY,charL,charT,charR,charB,glyphWid,sectLo,sectHi,yExpand);
      //  Wrap the line around the base arc.
      if (wrapArc) {
        originalSx=sX;
        originalEx=eX;
        if (inc) newPointsMade=wrapLineAroundBaseArcInc(&sX,&sY,&bX,&bY,&cX,&cY,&eX,&eY,charL,charT,charR,charB,sectHi);
        else     newPointsMade=wrapLineAroundBaseArc   (&sX,&sY,&bX,&bY,&cX,&cY,&eX,&eY,charL,      charR,charB,sectLo);
        if (newPointsMade) {
          newDist  =distPointToSeg(x,y,sX,sY,        bX,bY); if (newDist<minDist) minDist=newDist;
          newDist  =distPointToSeg(x,y,cX,cY,        eX,eY); if (newDist<minDist) minDist=newDist;
          if (sX!=originalSx) {
            newDist=distPointToSeg(x,y,sX,sY,originalSx,sY); if (newDist<minDist) minDist=newDist; }
          if (eX!=originalEx) {
            newDist=distPointToSeg(x,y,eX,eY,originalEx,eY); if (newDist<minDist) minDist=newDist; }
          handleArcAutoForDist(
          &minDist,x,y,sX,sY,bX,bY,cX,cY,eX,eY,charL,charT,charR,charB,charWid,glyphWid,yExpand,sectLo,sectHi,inc); }}
      if (!wrapArc || !newPointsMade) {
        newDist    =distPointToSeg(x,y,sX,sY,        eX,eY); if (newDist<minDist) minDist=newDist; }}

    else if (poly[j]==SPLINE) {                      //  SPLINE CURVE
      a=poly[j+1]; b=poly[j+2]; k=j+3; if (poly[k]==END_POLY || poly[k]==NEW_LOOP || poly[k]==NEW_POLY) k=start;
      if (poly[i]!=SPLINE) {
        sSpline= NO; sX=poly[i  ]; sY=poly[i+1]; }
      else {
        sSpline=YES; sX=poly[i+1]; sY=poly[i+2]; }
      if (poly[k]!=SPLINE) {
        eSpline= NO; eX=poly[k  ]; eY=poly[k+1]; }
      else {
        eSpline=YES; eX=poly[k+1]; eY=poly[k+2]; }
      //  Convert the spline’s glyph coordinates to movie-screen coordinates -- see the file “Coordinate Systems”.
      preCalc=charWid/glyphWid;
      sX=(sX+16.)*preCalc+charL;
      a =( a+16.)*preCalc+charL;
      eX=(eX+16.)*preCalc+charL; preCalc=(charT-charB)/(2.*FILMCO_GLYPH_SCALE);
      sY=(sY+16.)*preCalc+charB;
      b =( b+16.)*preCalc+charB;
      eY=(eY+16.)*preCalc+charB;
      //  Process the spline curve.
      handleSplineCurveForDist(
      &minDist,x,y,sX,sY,a,b,eX,eY,glyphWid,sectLo,sectHi,yExpand,charL,charT,charR,charB,wrapArc,YES,inc,sSpline,eSpline); }

    //  Advance through the polygon data.  (See the file “Spline Corner Logic”.)
    if      (poly[i]==SPLINE  ) i++ ;
    i+=2;
    if      (poly[i]==ARC     ) i+=3;
    else if (poly[i]==ARC_AUTO) i++ ;
    if      (poly[i]==NEW_LOOP
    ||       poly[i]==NEW_POLY) {
      i++; start=i; }}

  return minDist; }



//  Extract data about the Film Company font into an array.  (The availability of this data in this array
//  helps rendering speed.)
//
//  Returns nil if there is no error, or an error message if there is.
//
//  Called only once on app launch.
//
//  See the file “Polygon Constants” for a detailed description of how the polygon tags work.

BYTE *processFilmCoFont() {

  int4  i, j, chr, start ;

  //  Determine the range of glyphs covered by the FilmCo font, and fix non-Unicode character values.
  i=0; FilmCoGlyphs=0;
  while       (FilmCoFont[i]!=END_POLY) {
    chr=(int4) FilmCoFont[i]; if (chr<=-UTF8_CHARS || chr>=UTF8_CHARS) return "Invalid character found by processorFilmCoFont.";
    convertMacCharToUnicode(&chr); FilmCoFont[i]=chr; i+=6; if (chr>=FilmCoGlyphs) FilmCoGlyphs=chr+1;
    while     (FilmCoFont[i]!=END_POLY) {
      if      (FilmCoFont[i]==NEW_LOOP
      ||       FilmCoFont[i]==NEW_POLY) {
        i++; }
      if      (FilmCoFont[i]==ARC     ) i++;
      else if (FilmCoFont[i]==ARC_AUTO) i--;
      else if (FilmCoFont[i]==SPLINE  ) i++;
      i++; i++; }
    i++; }
  if (FilmCoGlyphs>MAX_INT4/sizeof(FilmCoGlyph_)) return "Range-of-int4 overrun found by processorFilmCoFont.";

  //  Allocate an array of FilmCo glyph data.
  if ( FilmCoGlyph) return "Duplicate allocation of FilmCoGlyph found by processorFilmCoFont.";
  FilmCoGlyph=(FilmCoGlyph_ *) malloc(FilmCoGlyphs*sizeof(FilmCoGlyph_));
  if (!FilmCoGlyph) return "Memory allocation failure in processorFilmCoFont.";

  //  Initialize the array with default values.
  for (i=0L; i<FilmCoGlyphs; i++) {
    FilmCoGlyph[i].width     = 0.;
    FilmCoGlyph[i].start     = 6 ;
    FilmCoGlyph[i].flatTop   =YES;
    FilmCoGlyph[i].flatBottom=YES;
    FilmCoGlyph[i].bottomSect=YES;
    FilmCoGlyph[i].hitsTop   =YES;
    FilmCoGlyph[i].sectionLo = 0.;
    FilmCoGlyph[i].sectionHi = 0.;
    FilmCoGlyph[i].yMult     = 1.;
    FilmCoGlyph[i].kern      = 0.; }

  //  Extract information about the glyphs that are supported by the font.  (This plays
  //  a little fast-and-loose, since some things have already been verified above.)
  i=0L;
  while       (FilmCoFont[i]!=END_POLY) {
    chr=(int4) FilmCoFont[i]; i+=6; start=i;
    FilmCoGlyph[chr].start     =           i   ;
    FilmCoGlyph[chr].flatTop   =           NO  ;
    FilmCoGlyph[chr].hitsTop   =           NO  ;
    FilmCoGlyph[chr].flatBottom=           NO  ;
    FilmCoGlyph[chr].bottomSect=           NO  ;
    FilmCoGlyph[chr].sectionLo =FilmCoFont[i-5];
    FilmCoGlyph[chr].sectionHi =FilmCoFont[i-4];
    FilmCoGlyph[chr].yMult     =FilmCoFont[i-3];
    FilmCoGlyph[chr].kern      =FilmCoFont[i-2];
    FilmCoGlyph[chr].overPeak  =FilmCoFont[i-1];
    while      (FilmCoFont[i  ]!=END_POLY) {

      //  Determine whether or not this glyph has a flat top and/or bottom, and whether it reaches the top at all.
      if       (FilmCoFont[i  ]< POLY_TAG_MIN
      ||        FilmCoFont[i  ]> POLY_TAG_MAX) {
        j=i+2;
        if     (FilmCoFont[j  ]==END_POLY
        ||      FilmCoFont[j  ]==NEW_LOOP
        ||      FilmCoFont[j  ]==NEW_POLY) {
          j=start; }
        if     (FilmCoFont[j  ]!=ARC
        &&      FilmCoFont[j  ]!=ARC_AUTO
        &&      FilmCoFont[j  ]!=SPLINE  ) {
          if   (FilmCoFont[i+1]>= FILMCO_GLYPH_SCALE
          ||    FilmCoFont[j+1]>= FILMCO_GLYPH_SCALE) {
            FilmCoGlyph[chr].hitsTop=YES; }
          if   (FilmCoFont[i+1]<FilmCoGlyph[chr].sectionLo
          ||    FilmCoFont[j+1]<FilmCoGlyph[chr].sectionLo) {
            FilmCoGlyph[chr].bottomSect=YES; }
          if   (FilmCoFont[i  ]-FilmCoFont[j]>.01
          ||    FilmCoFont[j  ]-FilmCoFont[i]>.01) {
            if (FilmCoFont[i+1]== FILMCO_GLYPH_SCALE
            &&  FilmCoFont[j+1]== FILMCO_GLYPH_SCALE) {
              FilmCoGlyph[chr].flatTop=YES; }
            if (FilmCoFont[i+1]==-FILMCO_GLYPH_SCALE
            &&  FilmCoFont[j+1]==-FILMCO_GLYPH_SCALE) {
              FilmCoGlyph[chr].flatBottom=YES; }}}}

      if       (FilmCoFont[i  ]==NEW_LOOP
      ||        FilmCoFont[i  ]==NEW_POLY) {
        start=--i+2; }
      else if  (FilmCoFont[i  ]==ARC     ) i++;
      else if  (FilmCoFont[i  ]==ARC_AUTO) i--;
      else if  (FilmCoFont[i  ]==SPLINE  ) i++;
      else if  (FilmCoFont[i  ]+16.>FilmCoGlyph[chr].width) FilmCoGlyph[chr].width=FilmCoFont[i]+FILMCO_GLYPH_SCALE;
      i++; i++; }
    i++; }

  //  Give all unsupported glyphs the width of a big square (-1. to 1.).
  for (i=0L; i<256L; i++) if (FilmCoGlyph[i].width==0.) FilmCoGlyph[i].width=(2.*FILMCO_GLYPH_SCALE);

  //  Success -- return no error message.
  return NULL; }



//  Prepares a null-terminated string for use as a Film Company logo (just the main text; not the Film Company Inc part below).

void prepFilmCoText(int4 *text) {
  straightQuotesToDirectional(text);
  stripExtraSpaces           (text);
  forceAtLeastOneChar        (text);
  forceUpperCase             (text); }



//  Prepares a null-terminated string for use as a Film Company Inc (limited-liability) tag.

void prepFilmCoIncText(int4 *text) {
  straightQuotesToDirectional(text);
  stripExtraSpaces           (text);
  forceAtLeastOneChar        (text); }



//  Used by the functions prepareToDrawFilmCoImage and shouldFilmCoCharArc to determine
//  whether a character is more than half off of the left edge of the Film Company
//  logo’s base arc, in which case it should not be wrapped around the arc at all, even
//  though it overlaps the arc somewhat.  (Notice that the leading “L” in the official
//  “Lucasfilm” logo overlaps the base arc but does not wrap around it.)

bool charMoreThanHalfOffLeftSideOfArc(int4 i) {
  return BaseArcLimitL-FilmCoChar[i].left>(FilmCoChar[i].right-FilmCoChar[i].left)*.5; }



//  Perform the necessary preparations for rendering the Film Company image.
//
//  Pass a value of 1.0 (in blurFrac) to do a full-range blur as would be applied to the main Film Company text by itself,
//  if no other kinds of blurring needed to be done.  Pass smaller values to do smaller blurs (that may add up to a larger
//  blur with multiple applications).  Do *not* pass a value larger than 1.0, as this will overrun the bell-curve arrays.
//
//  Note:  When performing multiple applications of the blur, use blurFrac values that *add* to the total amount of blur
//         you desire.  This function already uses sqrt on the passed value, to take into account the way that successive
//         gaussian blurs add up.

void prepareToDrawFilmCoImage(BYTE **rgb, int4 rowBytes, double blurFrac) {

  double  dX, dXb, dY, totalWidth, newTotalWidth, minGap, widthAdjuster ;
  int4    i ;

  //  Store the passed parameters in globals, for use during image drawing.
  PreviewRgb     =rgb     ;
  PreviewRowBytes=rowBytes;

  //  Build the gaussian-blur bell curves.
  FilmCoBellRadH   =(double) ImageWid    /FILMCO_BELL_DIVISOR_H*sqrt(blurFrac); buildFilmCoBellCurve(FilmCoBellRadH,&FilmCoBellH[0]);
  FilmCoBellRadV   =(double) ImageHeiUsed/FILMCO_BELL_DIVISOR_V*sqrt(blurFrac); buildFilmCoBellCurve(FilmCoBellRadV,&FilmCoBellV[0]);
  FilmCoBellRadHmax=(double) ImageWid    /FILMCO_BELL_DIVISOR_H;
  FilmCoBellRadVmax=(double) ImageHeiUsed/FILMCO_BELL_DIVISOR_V;

  //  Constrain all character values to the range of the FilmCo font.
  i=-1; while (FilmCoText[++i]) if (FilmCoText[i]>=FilmCoGlyphs) FilmCoText[i]=1;

  //  For the main (big) row of film-company text, calculate the horizontal positioning of each character.
  i=0; totalWidth=0.;
  while (FilmCoText[i  ]) {
    FilmCoChar[i  ].left =totalWidth;  totalWidth+=FilmCoGlyph[FilmCoText[i]].width/FILMCO_GLYPH_SCALE;
    FilmCoChar[i++].right=totalWidth;
    if  (FilmCoText[i  ]) totalWidth+=FILMCO_CHAR_SEP-FilmCoGlyph[FilmCoText[i-1]].kern/FILMCO_GLYPH_SCALE; }
  FilmCoChars=i; widthAdjuster=sqrt(totalWidth/FILMCO_DEFAULT_TOTALWIDTH); if (widthAdjuster>1.) widthAdjuster=1.;
  for (i=0; i<FilmCoChars; i++) {
    FilmCoChar[i  ].left -=totalWidth/2.; FilmCoChar[i].left *=FILMCO_TEXT_WIDTH/totalWidth*widthAdjuster;
    FilmCoChar[i  ].right-=totalWidth/2.; FilmCoChar[i].right*=FILMCO_TEXT_WIDTH/totalWidth*widthAdjuster; }

  //  Adjust the width of each character according to how it is affected by the base arc.  (But don’t start
  //  a circular loop of recalculation just because the new widths technically affect the heights too!)
  BaseArcRad          =FILMCO_TEXT_BOTTOM+(FILMCO_TEXT_TOP-FILMCO_TEXT_BOTTOM)*FILMCO_ARC_ENCROACH-FILMCO_ARC_CENTER_Y ;
  BaseArcRad2         =BaseArcRad*BaseArcRad;
  dY                  =FILMCO_TEXT_BOTTOM-FILMCO_ARC_CENTER_Y;
  BaseArcLimitR       =sqrt(BaseArcRad2-dY*dY);
  BaseArcLimitL       =-BaseArcLimitR ;
  BaseArcFrillRad     =BaseArcRad-.025;
  BaseArcIncRad       =BaseArcRad-.05 ;
  BaseArcFrillRad2    =BaseArcFrillRad*BaseArcFrillRad;
  BaseArcIncRad2      =BaseArcIncRad  *BaseArcIncRad  ;
  BaseArcFrillCapY    =FILMCO_TEXT_BOTTOM+.01667;
  dY                  =BaseArcFrillCapY-FILMCO_ARC_CENTER_Y;
  dX                  =sqrt(BaseArcFrillRad2-dY*dY);
  BaseArcFrillCapX    =-dX;
  BaseArcFrillInvSlope=dX/dY; newTotalWidth=0.; i=0;
  while (FilmCoText[i]) {
    //  Simple height setting for now, just to facilitate width resizing:
    dX=(FilmCoChar[i].left+FilmCoChar[i].right)*.5;
    if (charMoreThanHalfOffLeftSideOfArc(i)) {
      FilmCoChar[i].bottomHi=FILMCO_TEXT_BOTTOM; }
    else {
      FilmCoChar[i].bottomHi=FILMCO_ARC_CENTER_Y+sqrt(BaseArcRad2-dX*dX); }
    //  Calculate the new width.
    FilmCoChar[i].left =newTotalWidth; newTotalWidth+=FilmCoGlyph[FilmCoText[i]].width/FILMCO_GLYPH_SCALE
    *(FILMCO_TEXT_TOP-FilmCoChar[i].bottomHi)/(FILMCO_TEXT_TOP-FILMCO_TEXT_BOTTOM);
    FilmCoChar[i++].right=newTotalWidth;
    if (FilmCoText[i]) newTotalWidth+=FILMCO_CHAR_SEP-FilmCoGlyph[FilmCoText[i-1]].kern/FILMCO_GLYPH_SCALE; }
  FilmCoChars=i;
  for (i=0; i<FilmCoChars; i++) {
    FilmCoChar[i  ].left -=newTotalWidth/2.; FilmCoChar[i].left *=FILMCO_TEXT_WIDTH/newTotalWidth*widthAdjuster;
    FilmCoChar[i  ].right-=newTotalWidth/2.; FilmCoChar[i].right*=FILMCO_TEXT_WIDTH/newTotalWidth*widthAdjuster; }
  //  Determine the highest and lowest Y-coordinates across the base of the character’s enclosing rectangle (when
  //  it’s wrapped to the base arc).
  i=0;
  while (FilmCoText[i]) {
    dX  =-FilmCoChar[i].right;
    if (dX<0.) {
      dX=-FilmCoChar[i].left ; if (dX>0.) dX=0.; }
    if (charMoreThanHalfOffLeftSideOfArc(i)) {
      FilmCoChar[i].bottomHi=FILMCO_TEXT_BOTTOM; }
    else {
      FilmCoChar[i].bottomHi=FILMCO_ARC_CENTER_Y+sqrt(BaseArcRad2-dX*dX); }
    dX  =FilmCoChar[i].right; dX *=dX ;
    dXb =FilmCoChar[i].left ; dXb*=dXb; if (dXb>dX) dX=dXb;
    FilmCoChar[i++].bottomLo=FILMCO_ARC_CENTER_Y+sqrt(BaseArcRad2-dX   ); }
  //  Adjust the bottom of characters with non-flat bottoms so that their glyphs just touch the base arc.  This is
  //  tricky because the glyph can compress differently depending on its height (see the function
  //  compressFilmCoGlyphSections).  An iterative, honing-in solution is employed.
  i=0;
  while (FilmCoText[i]) {
    if (!FilmCoGlyph[FilmCoText[i]].flatBottom
    &&   FilmCoGlyph[FilmCoText[i]].bottomSect) {
      minGap  =INF;
      while (minGap<-.001 || minGap>.001) {
        minGap=INF;
        pointInSplineArcPoly(&minGap,&FilmCoFont[FilmCoGlyph[FilmCoText[i]].start],0.,0.,
        FilmCoChar[i].left,FILMCO_TEXT_TOP,FilmCoChar[i].right,FilmCoChar[i].bottomHi,FilmCoGlyph[FilmCoText[i]].width,
        NO,FilmCoGlyph[FilmCoText[i]].sectionLo,FilmCoGlyph[FilmCoText[i]].sectionHi,
        FilmCoGlyph[FilmCoText[i]].yMult,FilmCoGlyph[FilmCoText[i]].overPeak,NO);
        FilmCoChar[i].bottomHi-=minGap*.5;
        if (FilmCoChar[i].bottomHi<FILMCO_TEXT_BOTTOM) {
          FilmCoChar  [i].bottomHi=FILMCO_TEXT_BOTTOM; minGap=0.; }}
      FilmCoChar[i].bottomLo=FilmCoChar[i].bottomHi; }
    i++; }

  //  For the Film Company Inc (small) row of text, calculate the horizontal positioning of each character.
  i=0; totalWidth=0.;
  while (FilmCoIncText[i  ]) {
    FilmCoIncChar[i  ].left = totalWidth; totalWidth+=FilmCoGlyph[FilmCoIncText[i]].width/FILMCO_GLYPH_SCALE;
    FilmCoIncChar[i++].right= totalWidth;
    if  (FilmCoIncText[i  ]) totalWidth+=FILMCO_CHAR_SEP-FilmCoGlyph[FilmCoIncText[i-1]].kern/FILMCO_GLYPH_SCALE; }
  FilmCoIncChars=i; widthAdjuster=sqrt(totalWidth/FILMCO_INC_DEFAULT_TOTALWIDTH); if (widthAdjuster>1.) widthAdjuster=1.;
  for (i=0; i<FilmCoIncChars; i++) {
    FilmCoIncChar[i  ].left -=totalWidth/2.;
    FilmCoIncChar[i  ].left *=FILMCO_TEXT_WIDTH*FILMCO_INC_TEXT_WIDTH_FRAC/totalWidth*widthAdjuster;
    FilmCoIncChar[i  ].right-=totalWidth/2.;
    FilmCoIncChar[i  ].right*=FILMCO_TEXT_WIDTH*FILMCO_INC_TEXT_WIDTH_FRAC/totalWidth*widthAdjuster; }
  //  Determine the highest and lowest Y-coordinates across the top of the character’s enclosing rectangle (when it’s
  //  wrapped to the base arc).
  i=0;
  while (FilmCoIncText[i]) {
    dX = FilmCoIncChar[i].left ; dX *=dX ;
    dXb= FilmCoIncChar[i].right; dXb*=dXb; if (dXb>dX) dX=dXb;
    FilmCoIncChar[i++].topLo=FILMCO_ARC_CENTER_Y+sqrt(BaseArcIncRad2-dX); }
  //  Adjust the top of characters with non-flat tops so that their glyphs just touch the base arc.  This is tricky
  //  because the glyph can compress differently depending on its height (see the function
  //  compressFilmCoGlyphSections).  An iterative, honing-in solution is employed.
  i=0;
  while (FilmCoIncText[i]) {
    if (!FilmCoGlyph[FilmCoIncText[i]].flatTop && FilmCoGlyph[FilmCoIncText[i]].hitsTop) {
      minGap  =INF;
      while (minGap<-.001 || minGap>.001) {
        minGap=INF;
        pointInSplineArcPoly(&minGap,&FilmCoFont[FilmCoGlyph[FilmCoIncText[i]].start],0.,0.,
        FilmCoIncChar[i].left,FilmCoIncChar[i].topLo,FilmCoIncChar[i].right,FILMCO_TEXT_BOTTOM,FilmCoGlyph[FilmCoIncText[i]].width,
        NO,FilmCoGlyph[FilmCoIncText[i]].sectionLo,FilmCoGlyph[FilmCoIncText[i]].sectionHi,
        FilmCoGlyph[FilmCoIncText[i]].yMult,FilmCoGlyph[FilmCoIncText[i]].overPeak,YES);
        FilmCoIncChar[i].topLo+=minGap*.5; }}
    i++; }}



//  Builds an array of bell-curve values, to speed up generation of Film Company sparkles.
//
//  Warning:  Before calling this function, be sure “array” will be big enough to hold the results.
//
//     Note:  The standard formula for a bell curve is:   y  =  e ^ (-(x^2)/2)

void buildFilmCoBellCurve(double rad, double *array) {

  int4    i, radL=(int4) (rad+.999) ;
  double  x, sum=0. ;

  //  Build a bell curve.
  for (i=0; i<=radL*2; i++) {
    x=(double) (i-radL)/(double) radL*BELL_REACH;
    array[i]=pow(MATH_E,(-x*x)/2.); sum+=array[i]; }

  //  Normalize the bell curve so it adds up to 1.
  for (i=0; i<=radL*2; i++) array[i]/=sum; }



//  Decides whether a Film Company character should be wrapped around the base arc.

bool shouldFilmCoCharArc(int4 i) {
  return !charMoreThanHalfOffLeftSideOfArc(i) && FilmCoGlyph[FilmCoText[i]].flatBottom; }



//  Convenience function used by the functions addNodesForCurlyFrill and addNodesForBow to reduce duplicate
//  coding, by exploiting the fact that the curly frills and the bow are horizontally symmetrical.

void makeMirrorNodes(double x, int4 setNum, int4 subPixelRow, int4 *nodes, double *nodeX, int4 *polyTag) {

  int4  base=subPixelRow*NODE_ROW_MAX ;

  if (nodes[subPixelRow]<NODE_ROW_MAX) {
    nodeX[base+nodes[subPixelRow]]= x; polyTag[base+nodes[subPixelRow]]=setNum; nodes[subPixelRow]++; }
  if (nodes[subPixelRow]<NODE_ROW_MAX) {
    nodeX[base+nodes[subPixelRow]]=-x; polyTag[base+nodes[subPixelRow]]=setNum; nodes[subPixelRow]++; }}



//  Used by the functions drawFilmCoPixel and drawFilmCoPreviewPixel to generate the curly
//  frills that appear to the left and right of the Film Company Inc text.
//
//  Pass the value 1.0 in the “pad” parameter to go out one frill thickness from edge of frill.
//  Pass 1.5 to got 1.5 frill thicknesses out, etc.
//
//  Note:  This function contains repeated numeric constants which could be #defined, but since
//         the shape is fixed by the logo we are imitating, there is little reason to do that.
//
//  See this webpage for an explanation of whole-row node rendering:  http://alienryderflex.com/polygon_fill

void addNodesForCurlyFrill(
double pad, double y, double flexEdge, int4 setNum, int4 subPixelRow, int4 *nodes, double *nodeX, int4 *polyTag) {

  double  dX, dY, dX2 ;
  bool    lineCore=NO ;

  pad*=.00167*2.;

  if (y<=-.09333+pad
  &&  y>=-.17667-pad) {

    //  Central line.
    dY =-.135-y;
    dX2=(.00167   +pad)*(.00167   +pad)-dY*dY;
    if (dX2>=0.) {
      lineCore=YES; dX=sqrt(dX2);  makeMirrorNodes(           -.31-dX,setNum,subPixelRow,nodes,nodeX,polyTag); }
    dX2=(.00167*3.+pad)*(.00167*3.+pad)-dY*dY;
    if (dX2>=0.) {
      dX=sqrt(dX2); if (!lineCore) makeMirrorNodes(flexEdge-.00833-dX,setNum,subPixelRow,nodes,nodeX,polyTag);
      makeMirrorNodes                             (flexEdge-.00833+dX,setNum,subPixelRow,nodes,nodeX,polyTag); }

    //  Big circle.
    dX2=(.00167*25.+pad)*(.00167*25.+pad)-dY*dY;
    if (dX2>=0.) {
      dX=sqrt(dX2);
      if (y>-.135+.00167*13.5+pad
      ||  y<-.135-.00167*13.5-pad) {
        makeMirrorNodes(flexEdge-.00833-dX,setNum,subPixelRow,nodes,nodeX,polyTag); }
      makeMirrorNodes  (flexEdge-.00833+dX,setNum,subPixelRow,nodes,nodeX,polyTag); }
    dX2=(.00167*23.-pad)*(.00167*23.-pad)-dY*dY;
    if (dX2>=0.) {
      dX=sqrt(dX2);
      if (y>-.135+.00167*11.5-pad
      ||  y<-.135-.00167*11.5+pad) {
        makeMirrorNodes(flexEdge-.00833-dX,setNum,subPixelRow,nodes,nodeX,polyTag); } 
      makeMirrorNodes  (flexEdge-.00833+dX,setNum,subPixelRow,nodes,nodeX,polyTag); }

    //  Curly end of upper line.
    dY=y-(-.135+.00167*16.5);
    dX2=(.00167*5.+pad)*(.00167*5.+pad)-dY*dY;
    if (dX2>=0.) makeMirrorNodes(-.23333-sqrt(dX2),setNum,subPixelRow,nodes,nodeX,polyTag);
    dX2=(.00167*3.-pad)*(.00167*3.-pad)-dY*dY;
    if (dX2>=0.) makeMirrorNodes(-.23333-sqrt(dX2),setNum,subPixelRow,nodes,nodeX,polyTag);
    dY=y-(-.135+.00167*20.5);
    dX2=(.00167   +pad)*(.00167   +pad)-dY*dY;
    if (dX2>=0.) makeMirrorNodes(-.23333+sqrt(dX2),setNum,subPixelRow,nodes,nodeX,polyTag);

    //  Curly end of lower line.
    dY=y-(-.135-.00167*16.5);
    dX2=(.00167*5.+pad)*(.00167*5.+pad)-dY*dY;
    if (dX2>=0.) makeMirrorNodes(-.35   -sqrt(dX2),setNum,subPixelRow,nodes,nodeX,polyTag);
    dX2=(.00167*3.-pad)*(.00167*3.-pad)-dY*dY;
    if (dX2>=0.) makeMirrorNodes(-.35   -sqrt(dX2),setNum,subPixelRow,nodes,nodeX,polyTag);
    dY=y-(-.135-.00167*20.5);
    dX2=(.00167   +pad)*(.00167   +pad)-dY*dY;
    if (dX2>=0.) makeMirrorNodes(-.35   +sqrt(dX2),setNum,subPixelRow,nodes,nodeX,polyTag); }}



//  Used by the functions drawFilmCoPixel and drawFilmCoPreviewPixel to generate the bow (arc)
//  that appears directly below the Film Company large (main) text.
//
//  Closely related to addNodesForCurlyFrill.
//
//  Note:  This function contains repeated numeric constants which could be #defined, but since
//         the shape is fixed by the logo we are imitating, there is little reason to do that.
//
//  See this webpage for an explanation of whole-row node rendering:  http://alienryderflex.com/polygon_fill
//
//  See this webpage for an explanation of the point-in-spline-polygon technique:  http://alienryderflex.com/polyspline

void addNodesForBow(double pad, double y, int4 setNum, int4 subPixelRow, int4 *nodes, double *nodeX, int4 *polyTag) {

  bool    upperNode=NO, lowerNode=NO ;
  double  dX, dY, dX2 ;

  pad*=.00167*2.;

  if (y< FILMCO_ARC_CENTER_Y+BaseArcFrillRad +.00167+pad
  &&  y>=                    BaseArcFrillCapY-.00167-pad) {

    //  Big arc (bow).
    dY =y-FILMCO_ARC_CENTER_Y;
    dX2=(BaseArcFrillRad+.00167+pad)*(BaseArcFrillRad+.00167+pad)-dY*dY;
    if (dX2>0.) {
      dX=sqrt(dX2);
      if (dX/dY<BaseArcFrillInvSlope) {
        upperNode=YES;  makeMirrorNodes(-dX,setNum,subPixelRow,nodes,nodeX,polyTag); }}
    dX2=(BaseArcFrillRad-.00167-pad)*(BaseArcFrillRad-.00167-pad)-dY*dY;
    if (dX2>0.) {
      dX=sqrt(dX2);
      if (dX/dY<BaseArcFrillInvSlope) {
        lowerNode=YES;  makeMirrorNodes(-dX,setNum,subPixelRow,nodes,nodeX,polyTag); }}
    dY=y-BaseArcFrillCapY; dX2=(.00167+pad)*(.00167+pad)-dY*dY;
    if   (dX2> 0.) {
      dX=sqrt(dX2);
      if (dY >=0.) {
        if (!upperNode) makeMirrorNodes(BaseArcFrillCapX-dX,setNum,subPixelRow,nodes,nodeX,polyTag); }
      else {
        makeMirrorNodes                (BaseArcFrillCapX-dX,setNum,subPixelRow,nodes,nodeX,polyTag);
        if (!lowerNode) makeMirrorNodes(BaseArcFrillCapX+dX,setNum,subPixelRow,nodes,nodeX,polyTag); }}}}



//  Renders one pixel of the Film Company image, but makes just a simple sillhouette of the logo for the
//  user to preview while entering the text.
//
//  Related to drawFilmCoPixel.

void drawFilmCoPreviewPixel(BYTE *r, BYTE *g, BYTE *b, int4 pixelX, int4 pixelY,
bool *nodeRowsReady, int4 *nodes, double *nodeX, int4 *polyTag, int4 *nodeNext, bool *inPoly) {

  double  pixelFrac, screenX, screenY, subPixelPart=1./((double) RasterSubPixels*(double) RasterSubPixels) ;
  int4    i, ii, jj, polyCount ;

  //  Letterbox to black, if appropriate for this pixel.
  if (pixelY< ImageHeiLetterBox
  ||  pixelY>=ImageHeiLetterBox+ImageHeiUsed) {
    *r=(BYTE) 0;
    *g=(BYTE) 0;
    *b=(BYTE) 0; }
  else {

    //  Build the whole-row node arrays, if they need to be built.
    //  See this webpage for an explanation of whole-row node rendering:  http://alienryderflex.com/polygon_fill
    if (!(*nodeRowsReady)) {
      *nodeRowsReady=YES; polyCount=0L;
      for (jj=0L; jj<(int4) RasterSubPixels; jj++) {   //  (vertical subpixel loop)
        nodes[jj]=0; screenY=pixelToScreenY((double) pixelY+((double) jj+.5)/(double) RasterSubPixels);
        for (i=0; i<FilmCoChars; i++) {
          if (FilmCoText[i]!=' ') {
            addNodesFromSplineArcPoly_FilmCo(&FilmCoFont[FilmCoGlyph[FilmCoText[i]].start],screenY,
            FilmCoChar[i].left,FILMCO_TEXT_TOP,FilmCoChar[i].right,FilmCoChar[i].bottomHi,FilmCoGlyph[FilmCoText[i]].width,
            shouldFilmCoCharArc(i),FilmCoGlyph[FilmCoText[i]].sectionLo,FilmCoGlyph[FilmCoText[i]].sectionHi,
            FilmCoGlyph[FilmCoText[i]].yMult,FilmCoGlyph[FilmCoText[i]].overPeak,NO,
            jj,&polyCount,nodes,nodeX,polyTag); }
          polyCount++; }
        for (i=0; i<FilmCoIncChars; i++) {
          if (FilmCoIncText[i]!=' ') {
            addNodesFromSplineArcPoly_FilmCo(&FilmCoFont[FilmCoGlyph[FilmCoIncText[i]].start],screenY,
            FilmCoIncChar[i].left,FilmCoIncChar[i].topLo,FilmCoIncChar[i].right,FILMCO_TEXT_BOTTOM,
            FilmCoGlyph[FilmCoIncText[i]].width,FilmCoGlyph[FilmCoIncText[i]].flatTop,FilmCoGlyph[FilmCoIncText[i]].sectionLo,
            FilmCoGlyph[FilmCoIncText[i]].sectionHi,FilmCoGlyph[FilmCoIncText[i]].yMult,FilmCoGlyph[FilmCoIncText[i]].overPeak,YES,
            jj,&polyCount,nodes,nodeX,polyTag); }
          polyCount++; }
        addNodesForCurlyFrill(0.,screenY,FilmCoIncChar[0].left-.04167,polyCount,jj,nodes,nodeX,polyTag);
        addNodesForBow       (0.,screenY,                             polyCount,jj,nodes,nodeX,polyTag);
        sortNodeRow(jj,nodes,nodeX,polyTag); deOverlapNodeRow(jj,nodes,nodeX,polyTag); nodeNext[jj]=0; inPoly[jj]=NO; }}

    //  Construct the image pixel.
    pixelFrac=0.;
    for   (jj=0L; jj<(int4) RasterSubPixels; jj++) {   //    vertical subpixel loop
      for (ii=0L; ii<(int4) RasterSubPixels; ii++) {   //  horizontal subpixel loop
        screenX=pixelToScreenX((double) pixelX+((double) ii+.5)/(double) RasterSubPixels);

        //  Preview logo (simple, green sillhouette).
        while (nodeNext[jj]<nodes[jj] && nodeX[jj*NODE_ROW_MAX+nodeNext[jj]]<screenX) {
          nodeNext[jj]++; inPoly[jj]=!inPoly[jj]; }
        if (inPoly[jj]) pixelFrac+=subPixelPart; }}

    //  Put the calculated pixel value into the displayable bitmap.
    *r=(BYTE) (pixelFrac*(double) FILMCO_PREVIEW_R);
    *g=(BYTE) (pixelFrac*(double) FILMCO_PREVIEW_G);
    *b=(BYTE) (pixelFrac*(double) FILMCO_PREVIEW_B); }}



//  Renders one pixel of the Film Company image.  This function is for use during full-sequence rendering, not
//  for generating preview images when the user is inputting the Film Company or Film Company Inc texts.
//
//  Related to drawFilmCoPreviewPixel.

void drawFilmCoPixel(BYTE *r, BYTE *g, BYTE *b,
int4 pixelX, int4 pixelY, int4 mode, int4 stage, BYTE *bufGreen, BYTE *bufBlue, BYTE *bufOrange, BYTE *bufYellow,
bool *nodeRowsReady, int4 *nodes, double *nodeX, int4 *polyTag, int4 *nodeNext, bool *inPoly) {

  double  silhouetteFrac, justAuraFrac, greenBandFrac, tweenBandFrac, metalFrac, screenX, screenY, sparkSize ;
  double  minDist, newDist, red, green, blue, greyVal, invFade, dX, dY, dist2, sparkVal, distDiv ;
  double  subPixelPart=1./((double) RasterSubPixels*(double) RasterSubPixels) ;
  int4    i, ii, jj, polyCount, bufI ;

  //  Letterbox to black, if appropriate for this pixel.
  if (pixelY< ImageHeiLetterBox
  ||  pixelY>=ImageHeiLetterBox+ImageHeiUsed) {
    *r=(BYTE) 0;
    *g=(BYTE) 0;
    *b=(BYTE) 0; }
  else {

    //  Build the whole-row node arrays, if they need to be built.
    //  See this webpage for an explanation of whole-row node rendering:  http://alienryderflex.com/polygon_fill
    if (!(*nodeRowsReady)) {
      *nodeRowsReady=YES; polyCount=0L;
      for (jj=0L; jj<(int4) RasterSubPixels; jj++) {   //  (vertical subpixel loop)
        nodes[jj]=0; screenY=pixelToScreenY((double) pixelY+((double) jj+.5)/(double) RasterSubPixels);
        if (mode==    SILHOUETTE_A
        ||  mode==BORDERED_OVERLAY
        ||  mode==       BLUE_CORE
        ||  mode==     ORANGE_TINT) {
          for (i=0; i<FilmCoChars; i++) {
            if (FilmCoText[i]!=' ') {
              addNodesFromSplineArcPoly_FilmCo(&FilmCoFont[FilmCoGlyph[FilmCoText[i]].start],screenY,
              FilmCoChar[i].left,FILMCO_TEXT_TOP,FilmCoChar[i].right,FilmCoChar[i].bottomHi,FilmCoGlyph[FilmCoText[i]].width,
              shouldFilmCoCharArc(i),FilmCoGlyph[FilmCoText[i]].sectionLo,FilmCoGlyph[FilmCoText[i]].sectionHi,
              FilmCoGlyph[FilmCoText[i]].yMult,FilmCoGlyph[FilmCoText[i]].overPeak,NO,
              jj,&polyCount,nodes,nodeX,polyTag); }
            polyCount++; }}
        if (mode==    SILHOUETTE_B
        ||  mode==    SILHOUETTE_C
        ||  mode==BORDERED_OVERLAY
        ||  mode==       BLUE_CORE
        ||  mode==     ORANGE_TINT) {
          for (i=0; i<FilmCoIncChars; i++) {
            if (FilmCoIncText[i]!=' ') {
              addNodesFromSplineArcPoly_FilmCo(&FilmCoFont[FilmCoGlyph[FilmCoIncText[i]].start],screenY,
              FilmCoIncChar[i].left,FilmCoIncChar[i].topLo,FilmCoIncChar[i].right,FILMCO_TEXT_BOTTOM,
              FilmCoGlyph[FilmCoIncText[i]].width,FilmCoGlyph[FilmCoIncText[i]].flatTop,FilmCoGlyph[FilmCoIncText[i]].sectionLo,
              FilmCoGlyph[FilmCoIncText[i]].sectionHi,FilmCoGlyph[FilmCoIncText[i]].yMult,FilmCoGlyph[FilmCoIncText[i]].overPeak,YES,
              jj,&polyCount,nodes,nodeX,polyTag); }
            polyCount++; }}
        if      (mode==SILHOUETTE_A
        ||       mode==   BLUE_CORE
        ||       mode== ORANGE_TINT) {
          addNodesForCurlyFrill(0.,screenY,FilmCoIncChar[0].left-.04167,polyCount,jj,nodes,nodeX,polyTag);
          addNodesForBow       (0.,screenY,                             polyCount,jj,nodes,nodeX,polyTag); }
        else if (mode==FRILL_BORDERS) {
          addNodesForCurlyFrill(1.,screenY,FilmCoIncChar[0].left-.04167,polyCount,jj,nodes,nodeX,polyTag);
          addNodesForBow       (0.,screenY,                             polyCount,jj,nodes,nodeX,polyTag);
          addNodesForBow       (1.,screenY,                             polyCount,jj,nodes,nodeX,polyTag);
          addNodesForBow       (2.,screenY,                             polyCount,jj,nodes,nodeX,polyTag); }
        else if (mode==BORDERED_OVERLAY) {
          addNodesForCurlyFrill(0.,screenY,FilmCoIncChar[0].left-.04167,polyCount,jj,nodes,nodeX,polyTag); }
        sortNodeRow(jj,nodes,nodeX,polyTag); deOverlapNodeRow(jj,nodes,nodeX,polyTag); nodeNext[jj]=0; inPoly[jj]=NO; }}

    //  Construct the image pixel.
    silhouetteFrac=0.;
    greenBandFrac =0.;
    tweenBandFrac =0.;
    metalFrac     =0.;
    for   (jj=0L; jj<(int4) RasterSubPixels; jj++) {   //    vertical subpixel loop
      screenY  =pixelToScreenY((double) pixelY+((double) jj+.5)/(double) RasterSubPixels);
      for (ii=0L; ii<(int4) RasterSubPixels; ii++) {   //  horizontal subpixel loop
        screenX=pixelToScreenX((double) pixelX+((double) ii+.5)/(double) RasterSubPixels);

        //  Film Company logo.
        while (nodeNext[jj]<nodes[jj] && nodeX[jj*NODE_ROW_MAX+nodeNext[jj]]<screenX) {
          nodeNext[jj]++; inPoly[jj]=!inPoly[jj]; }
        if (inPoly[jj]) {
          silhouetteFrac+=subPixelPart;
          if (mode==  BLUE_CORE
          ||  mode==ORANGE_TINT) {
            metalFrac+=subPixelPart*MetalRnd[(int4) (
            ((double) MOVIE_SCREEN_WID/2.+screenX+
            ((double) MOVIE_SCREEN_HEI/2.-screenY)/2.)*METAL_SCRATCH_WIDTH_ADJUSTER)]; }}
        else if (mode==BORDERED_OVERLAY) {
          minDist=INF;
          for (i=0; i<FilmCoChars; i++) {
            if (screenX>=FilmCoChar[i].left    -FILMCO_BORDER_REACH
            &&  screenX< FilmCoChar[i].right   +FILMCO_BORDER_REACH
            &&  screenY>=FilmCoChar[i].bottomLo-FILMCO_BORDER_REACH && FilmCoText[i]!=' ') {
              newDist=distToFilmCoChar(&FilmCoFont[FilmCoGlyph[FilmCoText[i]].start],screenX,screenY,
              FilmCoChar[i].left,FILMCO_TEXT_TOP,FilmCoChar[i].right,FilmCoChar[i].bottomHi,FilmCoGlyph[FilmCoText[i]].width,
              shouldFilmCoCharArc(i),FilmCoGlyph[FilmCoText[i]].sectionLo,FilmCoGlyph[FilmCoText[i]].sectionHi,
              FilmCoGlyph[FilmCoText[i]].yMult,FilmCoGlyph[FilmCoText[i]].overPeak,NO);
              if (newDist<minDist) minDist=newDist; }}
          if   (minDist< FILMCO_BORDER_REACH) {
            if (minDist>=FILMCO_BORDER_REACH*FILMCO_BORDER_OUTER
            ||  minDist< FILMCO_BORDER_REACH*FILMCO_BORDER_INNER) {
              greenBandFrac+=subPixelPart; }
            else {
              tweenBandFrac+=subPixelPart; }}}}}
    metalFrac/=255.;

    //  Put the calculated pixel value into the displayable bitmap.
    if      (mode==SILHOUETTE_A) {
      *r=(BYTE) (silhouetteFrac*(double) FILMCO_0_AURA_R+.5);
      *g=(BYTE) (silhouetteFrac*(double) FILMCO_0_AURA_G+.5);
      *b=(BYTE) (silhouetteFrac*(double) FILMCO_0_AURA_B+.5); }
    else if (mode==SILHOUETTE_B) {
      red  =(double) (*r)+silhouetteFrac*(double) FILMCO_0_AURA_R; if (red  >255.) red  =255.;
      green=(double) (*g)+silhouetteFrac*(double) FILMCO_0_AURA_G; if (green>255.) green=255.;
      blue =(double) (*b)+silhouetteFrac*(double) FILMCO_0_AURA_B; if (blue >255.) blue =255.;
      *r=(BYTE) (red  +.5);
      *g=(BYTE) (green+.5);
      *b=(BYTE) (blue +.5); }
    else if (mode==SILHOUETTE_C) {
      justAuraFrac=1.-silhouetteFrac;
      red  =(double) (*r)*justAuraFrac+silhouetteFrac*(double) FILMCO_0_AURA2_R;
      green=(double) (*g)*justAuraFrac+silhouetteFrac*(double) FILMCO_0_AURA2_G;
      blue =(double) (*b)*justAuraFrac+silhouetteFrac*(double) FILMCO_0_AURA2_B;
      *r=(BYTE) (red  +.5);
      *g=(BYTE) (green+.5);
      *b=(BYTE) (blue +.5); }
    else if (mode==FRILL_BORDERS) {
      justAuraFrac=1.-silhouetteFrac;
      red  =(double) (*r)*justAuraFrac+silhouetteFrac*(double) FILMCO_0_BAND_R;
      green=(double) (*g)*justAuraFrac+silhouetteFrac*(double) FILMCO_0_BAND_G;
      blue =(double) (*b)*justAuraFrac+silhouetteFrac*(double) FILMCO_0_BAND_B;
      *r=(BYTE) (red  +.5);
      *g=(BYTE) (green+.5);
      *b=(BYTE) (blue +.5); }
    else if (mode==BORDERED_OVERLAY) {
      justAuraFrac=1.-silhouetteFrac-greenBandFrac-tweenBandFrac; tweenBandFrac*=.5;
      *r=(BYTE) (greenBandFrac*(double) FILMCO_0_BAND_R
      +          tweenBandFrac*(double) (*r)
      +           justAuraFrac*(double) (*r)+.5);
      *g=(BYTE) (greenBandFrac*(double) FILMCO_0_BAND_G
      +          tweenBandFrac*(double) (*g)
      +           justAuraFrac*(double) (*g)+.5);
      *b=(BYTE) (greenBandFrac*(double) FILMCO_0_BAND_B
      +          tweenBandFrac*(double) (*b)
      +           justAuraFrac*(double) (*b)+.5); }
    else if (mode==BLUE_CORE) {
      justAuraFrac=1.-silhouetteFrac;
      *r=(BYTE) ((double) (*r)*FILMCO_1_KNOCKDOWN*justAuraFrac+metalFrac*FILMCO_1_CORE_R);
      *g=(BYTE) ((double) (*g)*FILMCO_1_KNOCKDOWN*justAuraFrac+metalFrac*FILMCO_1_CORE_G);
      *b=(BYTE) ((double) (*b)*FILMCO_1_KNOCKDOWN*justAuraFrac+metalFrac*FILMCO_1_CORE_B); }
    else if (mode==ORANGE_TINT) {
      greyVal=rgbTOp(*r,*g,*b); justAuraFrac=1.-silhouetteFrac;
      *r=(BYTE) ((greyVal*justAuraFrac*FILMCO_2_RAMPUP+metalFrac)*FILMCO_2_CORE_R);
      *g=(BYTE) ((greyVal*justAuraFrac*FILMCO_2_RAMPUP+metalFrac)*FILMCO_2_CORE_G);
      *b=(BYTE) ((greyVal*justAuraFrac*FILMCO_2_RAMPUP+metalFrac)*FILMCO_2_CORE_B); }
    else if (mode==YELLOW_TINT) {
      greyVal=rgbTOp(*r,*g,*b);
      *r=(BYTE) (greyVal*FILMCO_3_CORE_R*FILMCO_3_RAMPUP);
      *g=(BYTE) (greyVal*FILMCO_3_CORE_G*FILMCO_3_RAMPUP);
      *b=(BYTE) (greyVal*FILMCO_3_CORE_B*FILMCO_3_RAMPUP); }
    else if (mode==BUFFERS_READY) {
      if      (stage==FILMCO_FADE_UP) {
        *r=(BYTE) ((double) bufGreen[(0L*ImageHei+pixelY)*ImageWid+pixelX]*FilmCoFade+.5);
        *g=(BYTE) ((double) bufGreen[(1L*ImageHei+pixelY)*ImageWid+pixelX]*FilmCoFade+.5);
        *b=(BYTE) ((double) bufGreen[(2L*ImageHei+pixelY)*ImageWid+pixelX]*FilmCoFade+.5); }
      else if (stage==FILMCO_SPARKLE_GREEN) {
        *r=(BYTE) ((double) bufGreen[(0L*ImageHei+pixelY)*ImageWid+pixelX]           +.5);
        *g=(BYTE) ((double) bufGreen[(1L*ImageHei+pixelY)*ImageWid+pixelX]           +.5);
        *b=(BYTE) ((double) bufGreen[(2L*ImageHei+pixelY)*ImageWid+pixelX]           +.5); }
      else if (stage==FILMCO_SPARKLE_GREEN_TO_BLUE) {
        bufI=pixelY*ImageWid+pixelX; invFade=1.-FilmCoFade;
        *r=(BYTE) ((double) bufGreen [bufI]*invFade+(double) bufBlue  [bufI]*FilmCoFade+.5);
        bufI+=ImageHei*ImageWid;
        *g=(BYTE) ((double) bufGreen [bufI]*invFade+(double) bufBlue  [bufI]*FilmCoFade+.5);
        bufI+=ImageHei*ImageWid;
        *b=(BYTE) ((double) bufGreen [bufI]*invFade+(double) bufBlue  [bufI]*FilmCoFade+.5); }
      else if (stage==FILMCO_SPARKLE_BLUE_TO_ORANGE) {
        bufI=pixelY*ImageWid+pixelX; invFade=1.-FilmCoFade;
        *r=(BYTE) ((double) bufBlue  [bufI]*invFade+(double) bufOrange[bufI]*FilmCoFade+.5);
        bufI+=ImageHei*ImageWid;
        *g=(BYTE) ((double) bufBlue  [bufI]*invFade+(double) bufOrange[bufI]*FilmCoFade+.5);
        bufI+=ImageHei*ImageWid;
        *b=(BYTE) ((double) bufBlue  [bufI]*invFade+(double) bufOrange[bufI]*FilmCoFade+.5); }
      else if (stage==FILMCO_ORANGE_TO_YELLOW) {
        bufI=pixelY*ImageWid+pixelX; invFade=1.-FilmCoFade;
        *r=(BYTE) ((double) bufOrange[bufI]*invFade+(double) bufYellow[bufI]*FilmCoFade+.5);
        bufI+=ImageHei*ImageWid;
        *g=(BYTE) ((double) bufOrange[bufI]*invFade+(double) bufYellow[bufI]*FilmCoFade+.5);
        bufI+=ImageHei*ImageWid;
        *b=(BYTE) ((double) bufOrange[bufI]*invFade+(double) bufYellow[bufI]*FilmCoFade+.5); }
      else if (stage==FILMCO_FADE_DOWN) {
        bufI=pixelY*ImageWid+pixelX; invFade=1.-FilmCoFade;
        *r=(BYTE) ((double) bufYellow[bufI]*invFade+.5);
        bufI+=ImageHei*ImageWid;
        *g=(BYTE) ((double) bufYellow[bufI]*invFade+.5);
        bufI+=ImageHei*ImageWid;
        *b=(BYTE) ((double) bufYellow[bufI]*invFade+.5); }
      if (stage>=FILMCO_FADE_UP
      &&  stage<=FILMCO_FADE_DOWN) {
        screenX=pixelToScreenX((double) pixelX+.5);
        screenY=pixelToScreenY((double) pixelY+.5);
        red  =(double) (*r);
        green=(double) (*g);
        blue =(double) (*b);
        for (i=0; i<FILMCO_SPARKLES; i++) {
          if (FilmCoProgress>=Sparkle[i].start
          &&  FilmCoProgress<=Sparkle[i].start+FilmCoSparkleDur) {
            dX=screenX-Sparkle[i].x;
            dY=screenY-Sparkle[i].y;
            sparkSize=(FilmCoProgress-(Sparkle[i].start+FilmCoSparkleDur*.5))/(FilmCoSparkleDur*.5);
            if (sparkSize<0.) sparkSize*=-1.;
            sparkSize=pow(1.-sparkSize,FILMCO_SPARKLE_POWER);
            distDiv  =.0001+sparkSize;
            dist2    =(dX*dX+dY*dY)*BELL_REACH*BELL_REACH/FILMCO_SPARKLE_MAX_RADIUS/FILMCO_SPARKLE_MAX_RADIUS/distDiv/distDiv;
            if (dist2<BELL_REACH*BELL_REACH) {   //  y = exp(-x*x/2)  --  standard bell-curve formula
              sparkVal=255.*exp(-dist2*.5);
              red  +=sparkVal;
              green+=sparkVal;
              blue +=sparkVal; }}}
        if (red  >255.) red  =255.;
        if (green>255.) green=255.;
        if (blue >255.) blue =255.;
        *r=(BYTE) (red  +.5);
        *g=(BYTE) (green+.5);
        *b=(BYTE) (blue +.5); }}}}



//  Perform a linear gaussian blur on one row or column of image pixels.
//
//  Note:  Performing a horizontal and a then a vertical (or vice versa) gaussian blur has the
//         same effect as performing a 2-D gaussian blur, but takes much less time to execute.
//
//  Closely related to blurFilmCoRow().

void blurFilmCoPixels(BYTE *r, BYTE *g, BYTE *b, int4 mode, double rad) {

  int4     i, j, segI, pixels, radL=(int4) (rad+.999) ;
  double  *blurSegR, sumR, sumG, sumB ;
  double  *blurSegG ;
  double  *blurSegB ;

  //  Determine number of pixels in row/column.
  if (mode==BLUR_A_H
  ||  mode==BLUR_B_H
  ||  mode==BLUR_C_H) {
    pixels=ImageWid; }
  else {
    pixels=ImageHei; }

  //  Dynamically allocate the blur arrays.
  blurSegR=(double *) malloc(maxLong(FilmCoBellDimensionH,FilmCoBellDimensionV)*sizeof(double));
  blurSegG=(double *) malloc(maxLong(FilmCoBellDimensionH,FilmCoBellDimensionV)*sizeof(double));
  blurSegB=(double *) malloc(maxLong(FilmCoBellDimensionH,FilmCoBellDimensionV)*sizeof(double));
  if (!blurSegR || !blurSegG || !blurSegB) return;

  //  Gaussian-blur one row/column of pixels.
  for (i=0L; i<=radL*2L; i++) {
    if      (i< radL) {
      blurSegR[i]=(double) r[i];
      blurSegG[i]=(double) g[i];
      blurSegB[i]=(double) b[i]; }
    else {
      blurSegR[i]=0.;
      blurSegG[i]=0.;
      blurSegB[i]=0.; }}
  i=radL;
  for  (j=0; j<pixels; j++) {
    if (j+radL<pixels) {
      blurSegR[i]=(double) r[j+radL];
      blurSegG[i]=(double) g[j+radL];
      blurSegB[i]=(double) b[j+radL]; }
    else {
      blurSegR[i]=0.;
      blurSegG[i]=0.;
      blurSegB[i]=0.; }
    if (++i>radL*2) i=0;
    sumR=0.; sumG=0.; sumB=0.;
    if (mode==BLUR_A_H
    ||  mode==BLUR_B_H
    ||  mode==BLUR_C_H) {
      for (segI=0; segI<=radL*2; segI++) {
        sumR+=blurSegR[i]*FilmCoBellH[segI];
        sumG+=blurSegG[i]*FilmCoBellH[segI];
        sumB+=blurSegB[i]*FilmCoBellH[segI]; if (++i>radL*2) i=0; }}
    else {
      for (segI=0; segI<=radL*2; segI++) {
        sumR+=blurSegR[i]*FilmCoBellV[segI];
        sumG+=blurSegG[i]*FilmCoBellV[segI];
        sumB+=blurSegB[i]*FilmCoBellV[segI]; if (++i>radL*2) i=0; }}

    //  Brighten up the blur (aura) so it can be seen more easily.
    if (mode==BLUR_C_V) {
      sumR*=FILMCO_0_BLUR_BRIGHTEN; if (sumR>255.) sumR=255.;
      sumG*=FILMCO_0_BLUR_BRIGHTEN; if (sumG>255.) sumG=255.;
      sumB*=FILMCO_0_BLUR_BRIGHTEN; if (sumB>255.) sumB=255.; }

    r[j]=(BYTE) (sumR+.5);
    g[j]=(BYTE) (sumG+.5);
    b[j]=(BYTE) (sumB+.5); }

  //  Release the blur arrays.
  free(blurSegR);
  free(blurSegG);
  free(blurSegB); }



//  Used by pointInSplineArcPoly to handle ARC_AUTO.  See pointInSplineArcPoly’s comments.

void handleArcAuto(double *minGap, double x, double y, double aX, double aY, double bX, double bY,
double cX, double cY, double dX, double dY, double charL, double charT, double charR, double charB, double charWid,
double glyphWid, double yExpand, double sectLo, double sectHi, bool inc, int4 *nodeCount) {

  double  deltaX, deltaY, distAB, rotCx, rotCy, rotDx, rotDy, theCos, theSin, abExtend, dcExtend, sectX ;
  double  splineBx, splineBy, splineCx, splineCy, hardX, hardY ;

  //  Discover the length of line segment AB.
  deltaX=bX-aX; deltaY=bY-aY; distAB=sqrt(deltaX*deltaX+deltaY*deltaY);

  //  Translate the line segments so that point B is at the origin.
  rotCx=cX-bX; rotCy=cY-bY;
  rotDx=dX-bX; rotDy=dY-bY;

  //  Rotate the line segments so that point A is on the positive, X axis.
  theCos=(aX-bX)/distAB;
  theSin=(aY-bY)/distAB;
  rotatePointAroundOrigin(&rotCx,&rotCy,theCos,-theSin);
  rotatePointAroundOrigin(&rotDx,&rotDy,theCos,-theSin);

  //  Discover the extenders.
  dcExtend=rotCy/(rotDy-rotCy); sectX=rotCx+(rotCx-rotDx)*dcExtend;
  abExtend=-sectX/distAB;

  //  Discover the two spline corners created by the extenders.
  splineBx=bX+(bX-aX)*abExtend*ARC_FRAC;
  splineBy=bY+(bY-aY)*abExtend*ARC_FRAC;
  splineCx=cX+(cX-dX)*dcExtend*ARC_FRAC;
  splineCy=cY+(cY-dY)*dcExtend*ARC_FRAC;

  //  Discover the new, hard corner inbetween the two spline corners.
  hardX=(splineBx+splineCx)*.5;
  hardY=(splineBy+splineCy)*.5;

  //  Process the two spline curves that simulate an arc.
  handleSplineCurve(minGap,x,y,bX   ,bY   ,splineBx,splineBy,hardX,hardY,
  glyphWid,sectLo,sectHi,yExpand,charL,charT,charR,charB,NO,NO,inc,NO,NO,nodeCount);
  handleSplineCurve(minGap,x,y,hardX,hardY,splineCx,splineCy,cX   ,cY   ,
  glyphWid,sectLo,sectHi,yExpand,charL,charT,charR,charB,NO,NO,inc,NO,NO,nodeCount); }



//  Used by addNodesFromSplineArcPoly_FilmCo to handle ARC_AUTO.  See addNodesFromSplineArcPoly_FilmCo’s comments.

void handleArcAutoForNodesRow(double y, double aX, double aY, double bX, double bY,
double cX, double cY, double dX, double dY, double charL, double charT, double charR, double charB, double charWid,
double glyphWid, double yExpand, double sectLo, double sectHi, bool inc, int4 subPixelRow, int4 polyCount,
int4 *nodes, double *nodeX, int4 *polyTag) {

  double  deltaX, deltaY, distAB, rotCx, rotCy, rotDx, rotDy, theCos, theSin, abExtend, dcExtend, sectX ;
  double  splineBx, splineBy, splineCx, splineCy, hardX, hardY ;

  //  Discover the length of line segment AB.
  deltaX=bX-aX; deltaY=bY-aY; distAB=sqrt(deltaX*deltaX+deltaY*deltaY);

  //  Translate the line segments so that point B is at the origin.
  rotCx=cX-bX; rotCy=cY-bY;
  rotDx=dX-bX; rotDy=dY-bY;

  //  Rotate the line segments so that point A is on the positive, X axis.
  theCos=(aX-bX)/distAB;
  theSin=(aY-bY)/distAB;
  rotatePointAroundOrigin(&rotCx,&rotCy,theCos,-theSin);
  rotatePointAroundOrigin(&rotDx,&rotDy,theCos,-theSin);

  //  Discover the extenders.
  dcExtend=rotCy/(rotDy-rotCy); sectX=rotCx+(rotCx-rotDx)*dcExtend;
  abExtend=-sectX/distAB;

  //  Discover the two spline corners created by the extenders.
  splineBx=bX+(bX-aX)*abExtend*ARC_FRAC;
  splineBy=bY+(bY-aY)*abExtend*ARC_FRAC;
  splineCx=cX+(cX-dX)*dcExtend*ARC_FRAC;
  splineCy=cY+(cY-dY)*dcExtend*ARC_FRAC;

  //  Discover the new, hard corner inbetween the two spline corners.
  hardX=(splineBx+splineCx)*.5;
  hardY=(splineBy+splineCy)*.5;

  //  Process the two spline curves that simulate an arc.
  handleSplineCurveForNodesRow(y,bX   ,bY   ,splineBx,splineBy,hardX,hardY,
  glyphWid,sectLo,sectHi,yExpand,charL,charT,charR,charB,NO,NO,inc,NO,NO,subPixelRow,polyCount,nodes,nodeX,polyTag);
  handleSplineCurveForNodesRow(y,hardX,hardY,splineCx,splineCy,cX   ,cY   ,
  glyphWid,sectLo,sectHi,yExpand,charL,charT,charR,charB,NO,NO,inc,NO,NO,subPixelRow,polyCount,nodes,nodeX,polyTag); }



//  Used by deOverlapNodeRow to destroy a node.  See deOverlapNodeRow’s comments.

void killNode(int4 theNode, int4 subPixelRow, int4 *nodes, double *nodeX, int4 *polyTag) {

  int4  base=subPixelRow*NODE_ROW_MAX ;
  int4  i ;

  nodes[subPixelRow]--;
  for (i=theNode; i<nodes[subPixelRow]; i++) {
    nodeX  [base+i]=nodeX  [base+i+1];
    polyTag[base+i]=polyTag[base+i+1]; }}



//  Used by the functions drawFilmCoPixel and drawFilmCoPreviewPixel to eliminate
//  polygon overlap from a node row.
//
//  This function makes the fast, whole-row, node-list rendering method work with
//  overlapping polygons that are not intended to cancel each other, such as those used
//  by the letters 'A', 'K', and 'X'.  (See the files “Film Company Font Design” and
//  “Polygon Constants”.)
//
//  This function assumes that the nodes are already sorted left-to-right -- so be sure
//  to call the function sortNodeRow before calling this function.

void deOverlapNodeRow(int4 subPixelRow, int4 *nodes, double *nodeX, int4 *polyTag) {

  int4  tagA=-1L, tagB, i=0L, extraA, base=subPixelRow*NODE_ROW_MAX ;

  while   (i<nodes[subPixelRow]) {
    if (tagA<0L) {
      tagA     =polyTag[base+(i++)]; }
    else {
      if (tagA==polyTag[base+ i   ]) {
        tagA=-1L; i++; }
      else {
        tagB=   polyTag[base+i]; killNode(i,subPixelRow,nodes,nodeX,polyTag);
        while (i<nodes[subPixelRow]
        &&     polyTag[base+i]!=tagA
        &&     polyTag[base+i]!=tagB) {
          i++; }
        if    (i<nodes[subPixelRow]) {
          if (polyTag[base+i]==tagB) {
            killNode(i,subPixelRow,nodes,nodeX,polyTag); }
          else {
            killNode(i,subPixelRow,nodes,nodeX,polyTag); extraA=0L;
            while (i<nodes[subPixelRow]
            &&     polyTag[base+i]!=tagB) {
              if  (polyTag[base+i]==tagA) {
                extraA++; killNode(i,subPixelRow,nodes,nodeX,polyTag); }
              else {
                i++; }}
            if (i<nodes[subPixelRow]) {
              if (extraA&1L) killNode(i,subPixelRow,nodes,nodeX,polyTag);
              else           polyTag[base+i]=tagA; }}
          i=0L; tagA=-1L; }}}}}
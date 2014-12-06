//  Star Wars TSG
//  Darel Rex Finley, 2006-2011



#include                     "nil.h"
#include                    "bool.h"
#include             "definitions.h"
#include "DistributorSearchlights.h"
#include         "DistributorFont.h"
#include       "Shared Structures.h"
#include           "cbf functions.h"
#include  "image buffer functions.h"
#include      "geometry functions.h"
#include       "numeric functions.h"
#include        "string functions.h"
#include                    "math.h"
#include                  "stdlib.h"

#include             "Distributor.h"



//  Globals used during rendering of Distributor image.  (Populated primarily by
//  prepareToDrawDistributorImage; used primarily by drawDistributorPixel.)
int4               DistributorBackdropWid, DistributorBackdropHei ;
DistributorRow_    DistributorRow[DISTRIBUTOR_MAX_ROWS] ;
double             VanX, VanY ;
BYTE             **DistributorTowerMaskLev, **DistributorTowerRgb, **DistributorBackdrop, **DistributorSearchlightMaskLev ;
BYTE               DistributorFade ;   //  (Do not set directly -- use setDistributorFade.)
int4               DistributorTowerMaskRowBytes, DistributorTowerRowBytes, DistributorSearchlightMaskRowBytes ;
int4               DistributorTowerWid, DistributorTowerHei, DistributorBackdropRowBytes ;
BYTE              *CbfDistInfo=nil ;   //  Distributor Info cbf (font).
int4               DistributorRows ;
bool               DistributorImageReady ;
int4               DistributorText[MAX_CHARS_DISTRIBUTOR+1] ;
int4               DistInfoText   [MAX_CHARS_DIST_INFO  +1] ;
double             StackPlate[DISTRIBUTOR_MAX_ROWS*3][9]    ;
double             DistributorMovieScreenSlopeMax ;

//  Globals populated by prepareDistInfoChars().
DistInfoChar_  DistInfoChar[MAX_CHARS_DIST_INFO] ;
int4           DistInfoChars ;

double  SearchlightAngle[SEARCHLIGHT_COUNT] ;

DistributorGlyph_  *DistributorGlyph=nil ;
int4                DistributorGlyphs    ;



//  Calculates the angles of the generated searchlights as a function of the time during which
//  they have been onscreen.
//
//  In the “time” parameter, pass the amount of movie-time (seconds) that has elapsed since the
//  distributor image first started fading up -- or pass a constant if this is a preview image.

void setSearchlightAngles(double time) {

  int4  i ;

  for (i=0; i<SEARCHLIGHT_COUNT; i++) {
    SearchlightAngle[i]=
    SearchlightCenterAngle[i]+SearchlightAmplitude[i]*sin(CIRCLE_RADIANS*(time/SearchlightPeriod[i]+SearchlightPhase[i])); }}



void setDistributorFade(double frac) {
  DistributorFade=(BYTE) (int4) (255.*(1.-(DISTRIBUTOR_FADE_BASE+frac*(1.-DISTRIBUTOR_FADE_BASE)))+.5); }



//  Extracts various information from the Distributor font.  Called once on app launch.
//
//  Returns nil if there is no error, or an error message if there is.
//
//  See the file “Polygon Constants” for a detailed description of how the polygon tags work.
//
//  Important:  Keep in mind that every spline-polygon font character in this project is assumed to have its left edge
//              lined up with the leftmost X position (-FILMCO_GLYPH_SCALE, -1, or 0 depending on the font -- see the
//              file “Coordinate Systems”), and that both the left-edge alignment and the calculated width of the
//              glyph are based on hard corners only, not spline corners, arc centers, or the horizontal reach
//              (extent) of spline curves.

BYTE *processDistributorFont() {

  int4  i, chr ;

  //  Determine the range of glyphs covered by the Distributor font, and fix non-Unicode character values.
  i=0; DistributorGlyphs=0;
  while       (DistributorFont[i]!=END_POLY) {
    chr=(int4) DistributorFont[i]; if (chr<=-UTF8_CHARS || chr>=UTF8_CHARS) return "Invalid character found by processorDistributorFont.";
    convertMacCharToUnicode(&chr); DistributorFont[i++]=chr; if (chr>=DistributorGlyphs) DistributorGlyphs=chr+1;
    while     (DistributorFont[i]!=END_POLY) {
      if      (DistributorFont[i]==NEW_LOOP) i++;
      if      (DistributorFont[i]==SPLINE  ) i++;
      i++; i++; }
    i++; }
  if (DistributorGlyphs>MAX_INT4/sizeof(DistributorGlyph_)) return "Range-of-int4 overrun found by processorDistributorFont.";

  //  Allocate an array of distributor glyph data.
  if ( DistributorGlyph) return "Duplicate allocation of DistributorGlyph found by processorDistributorFont.";
  DistributorGlyph=(DistributorGlyph_ *) malloc(DistributorGlyphs*sizeof(DistributorGlyph_));
  if (!DistributorGlyph) return "Memory allocation failure in processorDistributorFont.";

  //  Initialize array with default values.
  for (i=0; i<DistributorGlyphs; i++) {
    DistributorGlyph[i].width=0.;
    DistributorGlyph[i].start=1 ; }

  //  Extract information about the glyphs that are supported by the font.  (This plays
  //  a little fast-and-loose, since some things have already been verified above.)
  i=0;
  while       (DistributorFont[i  ]!=END_POLY) {
    chr=(int4) DistributorFont[i++]; DistributorGlyph[chr].start=i;
    while     (DistributorFont[i  ]!=END_POLY) {
      if      (DistributorFont[i  ]==NEW_LOOP) i++;
      if      (DistributorFont[i  ]==SPLINE  ) i++;
      if      (DistributorFont[i  ]+1.>DistributorGlyph[chr].width) DistributorGlyph[chr].width=DistributorFont[i]+1.;
      i++; i++; }
    i++; }

  //  Give all unsupported glyphs the width of a big square (-1. to 1.).
  for (i=0; i<DistributorGlyphs; i++) if (DistributorGlyph[i].width==0.) DistributorGlyph[i].width=2.;

  //  Success -- return no error message.
  return NULL; }



//  Converts a point from the Distributor logo construction coordinate system to the movie-screen
//  coordinate system, applying the appropriate perspective.  See the file “Coordinate Systems”.

void distributorFwdPerspective(double *x, double *y) {

  double  div ;

  *y+=DISTRIBUTOR_Y_POST_OFFSET;
  *x*=DISTRIBUTOR_MAG;
  *y*=DISTRIBUTOR_MAG;

  div=DISTRIBUTOR_VIEW_DIST-DISTRIBUTOR_TEXT_SLOPE*(*x);
  *x/=div;
  *y/=div;

  *x/=DISTRIBUTOR_X_PRE_MULT  ;
  *y/=DISTRIBUTOR_Y_PRE_MULT  ;

  *x-=DISTRIBUTOR_X_PRE_OFFSET;
  *y-=DISTRIBUTOR_Y_PRE_OFFSET; }



//  This function does the reverse of distributorFwdPerspective.

void distributorRevPerspective(double *x, double *y) {

  *x+=DISTRIBUTOR_X_PRE_OFFSET;
  *y+=DISTRIBUTOR_Y_PRE_OFFSET;

  *x*=DISTRIBUTOR_X_PRE_MULT;
  *y*=DISTRIBUTOR_Y_PRE_MULT;

  *x =DISTRIBUTOR_VIEW_DIST*(*x)/(1.+DISTRIBUTOR_TEXT_SLOPE*(*x));
  *y*=DISTRIBUTOR_VIEW_DIST         -DISTRIBUTOR_TEXT_SLOPE*(*x) ;

  *x/=DISTRIBUTOR_MAG;
  *y/=DISTRIBUTOR_MAG; *y-=DISTRIBUTOR_Y_POST_OFFSET; }



//  This function generally makes the distributor text all uppercase, but it has
//  special exceptions for certain letter combinations, such as “20th” or “Mc”.

void fixDistributorCase(int4 *text) {

  bool  inNumber=NO, lastWasM=NO, foundMc ;
  int4  c=*text ;
  int4  i=0     ;

  while (c) {
    foundMc=(c=='c' && lastWasM); lastWasM=(c=='M');
    uniCharToUpperCase(&c);
    if (c>='0' && c<='9') inNumber=YES;
    else {
      if (c!='D' && c!='E' && c!='H' && c!='N' && c!='R' && c!='S' && c!='T') inNumber=NO;
      else if (inNumber) c-='A'-'a'; }
    if (foundMc) c='c';
    text[i]=c; c=text[++i]; }}



//  Prepares a null-terminated string for use as the text of the Distributor logo.

void prepDistributorText(int4 *text) {

  int4  c, lastChar=' ' ;
  bool  foundText=NO    ;
  int4  i=0L, j=0L      ;

  straightQuotesToDirectional(text);

  //  Strip out undesirable spaces, and ensure that the string is at least one character in length.
  while (text[i]) {
    c=text[i]; if (c!=' ') foundText=YES;
    if (foundText && (c!=' ' || lastChar!=' ')) text[j++]=c;
    lastChar=c; i++; }
  if (j && text[j-1]==' ') j--;
  if (!j) text[j++]=1;   //  (ensure at least one character)
  text[j]=0;

  //  Set best upper/lower case usage.
  fixDistributorCase(text); }



//  Prepares a null-terminated string for use as the text of the Distributor Info tagline.

void prepDistInfoText(int4 *text) {

  int4  i, j, c, lastChar=' ' ;
  bool  foundText=NO ;

  straightQuotesToDirectional(text);

  //  Strip out undesirable, extra spaces.
  i=0; j=0;
  while (text[i]) {
    c=text[i]; if (c!=' ') foundText=YES;
    if (foundText && (c!=' ' || lastChar!=' ')) text[j++]=c;
    lastChar=c; i++; }
  if (j && text[j-1]==' ') j--;
  text[j]=0; }



//  Perform the necessary preparations for rendering the Distributor image.
//
//  See the file “Polygon Constants” for a detailed description of how the polygon tags work.

void prepareToDrawDistributorImage(
BYTE **imageRgb, BYTE **backdrop, int4 rowBytes,
BYTE **towerRgb, BYTE **towerMaskLev, BYTE **searchlightMaskLev,
int4 towerWid, int4 towerHei,
int4 towerRowBytes, int4 towerMaskRowBytes, int4 searchlightMaskRowBytes,
int4 backdropRowBytes) {

  double       plateExtendLR, plateExtendFB, extraRowSep, rowSep, totalHei=0., rowY=1., x, y ;
  int4         charI, row, polyI ;
  bool         extraPlateCreated ;
  DoubleRect_  glyphBounds ;
  int4         i ;

  //  Store the passed parameters in globals, for use during image rendering.
  PreviewRowBytes                   =rowBytes               ;
  DistributorTowerWid               =towerWid               ;
  DistributorTowerHei               =towerHei               ;
  DistributorTowerMaskLev           =towerMaskLev           ;
  DistributorTowerMaskRowBytes      =towerMaskRowBytes      ;
  DistributorTowerRowBytes          =towerRowBytes          ;
  DistributorTowerRgb               =towerRgb               ;
  PreviewRgb                        =imageRgb               ;
  DistributorBackdrop               =backdrop               ;
  DistributorSearchlightMaskRowBytes=searchlightMaskRowBytes;
  DistributorBackdropRowBytes       =backdropRowBytes       ;
  DistributorSearchlightMaskLev     =searchlightMaskLev     ;

  //  Default to full brightness image.
  setDistributorFade(1.);

  //  Constrain all character values to the range of the Distributor font.
  i=-1; while (DistributorText[++i]) if (DistributorText[i]>=DistributorGlyphs) DistributorText[i]=1;

  //  Parse the Distributor text into rows that can be stacked in the logo.
  DistributorRows=0; charI=0;
  while       (DistributorText[charI]) {
    while     (DistributorText[charI]==' ') charI++;
    if        (DistributorText[charI]) {
      DistributorRow[DistributorRows].start=charI;
      while   (DistributorText[charI]!=' ' && DistributorText[charI]) charI++;
      if (charI-DistributorRow[DistributorRows].start==1) {   //  prevent single-character row
        while (DistributorText[charI]==' '                          ) charI++;
        while (DistributorText[charI]!=' ' && DistributorText[charI]) charI++; }
      DistributorRow[DistributorRows].end=charI; if (DistributorRows<DISTRIBUTOR_MAX_ROWS) DistributorRows++; }}
  if (DistributorRows>1 && DistributorRow[DistributorRows-1].end-DistributorRow[DistributorRows-1].start==1) {   //  prevent single-character row
    DistributorRow[DistributorRows-2].end=DistributorRow[DistributorRows-1].end; DistributorRows--; }
  for  (row=1; row<DistributorRows; row++) {   //  Move single-character words to previous row if that will make the rows more equal.
    if (DistributorRow[row-1].end  -    DistributorRow[row-1].start+2
    <   DistributorRow[row  ].end  -    DistributorRow[row  ].start
    &&  DistributorRow[row  ].end  -    DistributorRow[row  ].start>3
    &&  DistributorRow[row-1].end  + 1==DistributorRow[row  ].start
    &&  DistributorText[DistributorRow[row-1].end    ]==' '
    &&  DistributorText[DistributorRow[row  ].start+1]==' ') {
      DistributorRow  [row-1].end  +=2;
      DistributorRow  [row  ].start+=2; }}

  //  Set the maximum extrusion slope (used to optimize rendering of extrusion sides).
  x=1.; y=1.; distributorFwdPerspective(&x,&y); DistributorMovieScreenSlopeMax=(y-SCREEN_VAN_Y)/(SCREEN_VAN_X-x);

  //  For the Distributor text, calculate the X scale of each row independently.
  for (row=0; row<DistributorRows; row++) {
    DistributorRow[row].wid=-DISTRIBUTOR_CHAR_SEP;
    for (charI=DistributorRow[row].start; charI<DistributorRow[row].end; charI++) {
      DistributorRow[row].wid   +=DISTRIBUTOR_CHAR_SEP+DistributorGlyph[DistributorText[charI]].width; }
    DistributorRow  [row].scaleX =2./DistributorRow[row].wid; if (DistributorRow[row].scaleX>1.) DistributorRow[row].scaleX=1.; }

  //  For the distributor text, calculate the height and Y position (top) of each row.
  for (row=0; row<DistributorRows; row++) {
    DistributorRow[row].hei=2.*DistributorRow[row].scaleX;
    if (DistributorRows>1) DistributorRow[row].hei*=(DISTRIBUTOR_ROW_ADJUST_TOP+(double) row/(double) (DistributorRows-1)*(DISTRIBUTOR_ROW_ADJUST_BOT-DISTRIBUTOR_ROW_ADJUST_TOP));
    totalHei+=DistributorRow[row].hei; }
  for (row=0; row<DistributorRows; row++) {
    DistributorRow[row].hei*=(2.-DISTRIBUTOR_ROW_SEP*(DistributorRows-1)-DISTRIBUTOR_EXTRA_ROW_SEP)/totalHei;
    DistributorRow[row].top =rowY; rowY-=DistributorRow[row].hei+DISTRIBUTOR_ROW_SEP;
    if (!row) rowY-=1.5*DISTRIBUTOR_ROW_SEP; }

  //  For the Distributor text, calculate the amount of rise (overpeak) and desc (undertow) caused by special characters.
  for (row=0; row<DistributorRows; row++) {
    DistributorRow[row].rise= 1.;
    DistributorRow[row].desc=-1.;
    for (charI=DistributorRow[row].start; charI<DistributorRow[row].end; charI++) {
      extractDistributorGlyphBounds(DistributorText[charI],&glyphBounds);
      if (glyphBounds.T>DistributorRow[row].rise) DistributorRow[row].rise=glyphBounds.T;
      if (glyphBounds.B<DistributorRow[row].desc) DistributorRow[row].desc=glyphBounds.B; }}

  //  For the Distributor text, calculate the vanishing point that will be used for doing the text’s dark, extrusion sides, and the stack plates.
  VanX=SCREEN_VAN_X;
  VanY=SCREEN_VAN_Y; distributorRevPerspective(&VanX,&VanY);

  //  Calculate the polygonal data for the stack plates.
  extraPlateCreated=NO; extraRowSep=0.; rowSep=DISTRIBUTOR_EXTRA_ROW_SEP;
  plateExtendLR=EXTRA_PLATE_EXTEND_LR;
  plateExtendFB=EXTRA_PLATE_EXTEND_FB; row=0;
  while (row<DistributorRows-1 || row==0 && DistributorRows==1) {
    //  Plate’s expansive underbelly.
    polyI=row*3; if (extraPlateCreated) polyI+=3;
    StackPlate[polyI][0]=-1.-plateExtendLR; StackPlate[polyI][1]=DistributorRow[row].top-DistributorRow[row].hei-extraRowSep-rowSep;
    StackPlate[polyI][2]= 1.+plateExtendLR; StackPlate[polyI][3]=DistributorRow[row].top-DistributorRow[row].hei-extraRowSep-rowSep;
    StackPlate[polyI][4]=VanX+(StackPlate[polyI][2]-VanX)/(1.+EXTRUSION_LENGTH);
    StackPlate[polyI][5]=VanY+(StackPlate[polyI][3]-VanY)/(1.+EXTRUSION_LENGTH);
    StackPlate[polyI][6]=VanX+(StackPlate[polyI][0]-VanX)/(1.+EXTRUSION_LENGTH);
    StackPlate[polyI][7]=VanY+(StackPlate[polyI][1]-VanY)/(1.+EXTRUSION_LENGTH);
    for (i=0; i<4; i++) {
      distributorFwdPerspective(&StackPlate[polyI][i*2],&StackPlate[polyI][i*2+1]);
      if (i<2) {
        StackPlate[polyI][i*2  ]=SCREEN_VAN_X+(StackPlate[polyI][i*2  ]-SCREEN_VAN_X)*(1.+plateExtendFB);
        StackPlate[polyI][i*2+1]=SCREEN_VAN_Y+(StackPlate[polyI][i*2+1]-SCREEN_VAN_Y)*(1.+plateExtendFB); }
      else {
        StackPlate[polyI][i*2  ]=SCREEN_VAN_X+(StackPlate[polyI][i*2  ]-SCREEN_VAN_X)/(1.+plateExtendFB);
        StackPlate[polyI][i*2+1]=SCREEN_VAN_Y+(StackPlate[polyI][i*2+1]-SCREEN_VAN_Y)/(1.+plateExtendFB); }}
    StackPlate[polyI][8]=END_POLY;
    //  Plate’s front-facing edge.
    polyI++;
    StackPlate[polyI][0]=-1.-plateExtendLR; StackPlate[polyI][1]=DistributorRow[row].top-DistributorRow[row].hei-extraRowSep-rowSep;
    StackPlate[polyI][2]= 1.+plateExtendLR; StackPlate[polyI][3]=DistributorRow[row].top-DistributorRow[row].hei-extraRowSep-rowSep;
    StackPlate[polyI][4]= 1.+plateExtendLR; StackPlate[polyI][5]=DistributorRow[row].top-DistributorRow[row].hei-extraRowSep       ;
    StackPlate[polyI][6]=-1.-plateExtendLR; StackPlate[polyI][7]=DistributorRow[row].top-DistributorRow[row].hei-extraRowSep       ;
    for (i=0; i<4; i++) {
      distributorFwdPerspective(&StackPlate[polyI][i*2],&StackPlate[polyI][i*2+1]);
      StackPlate[polyI][i*2  ]=SCREEN_VAN_X+(StackPlate[polyI][i*2  ]-SCREEN_VAN_X)*(1.+plateExtendFB);
      StackPlate[polyI][i*2+1]=SCREEN_VAN_Y+(StackPlate[polyI][i*2+1]-SCREEN_VAN_Y)*(1.+plateExtendFB); }
    StackPlate[polyI][8]=END_POLY;
    //  Plate’s side-facing edge.
    polyI++;
    StackPlate[polyI][0]= 1.+plateExtendLR; StackPlate[polyI][1]=DistributorRow[row].top-DistributorRow[row].hei-extraRowSep-rowSep;
    StackPlate[polyI][2]= 1.+plateExtendLR; StackPlate[polyI][3]=DistributorRow[row].top-DistributorRow[row].hei-extraRowSep       ;
    StackPlate[polyI][4]=VanX+(StackPlate[polyI][2]-VanX)/(1.+EXTRUSION_LENGTH);
    StackPlate[polyI][5]=VanY+(StackPlate[polyI][3]-VanY)/(1.+EXTRUSION_LENGTH);
    StackPlate[polyI][6]=VanX+(StackPlate[polyI][0]-VanX)/(1.+EXTRUSION_LENGTH);
    StackPlate[polyI][7]=VanY+(StackPlate[polyI][1]-VanY)/(1.+EXTRUSION_LENGTH);
    for (i=0; i<4; i++) {
      distributorFwdPerspective(&StackPlate[polyI][i*2],&StackPlate[polyI][i*2+1]);
      if (i<2) {
        StackPlate[polyI][i*2  ]=SCREEN_VAN_X+(StackPlate[polyI][i*2  ]-SCREEN_VAN_X)*(1.+plateExtendFB);
        StackPlate[polyI][i*2+1]=SCREEN_VAN_Y+(StackPlate[polyI][i*2+1]-SCREEN_VAN_Y)*(1.+plateExtendFB); }
      else {
        StackPlate[polyI][i*2  ]=SCREEN_VAN_X+(StackPlate[polyI][i*2  ]-SCREEN_VAN_X)/(1.+plateExtendFB);
        StackPlate[polyI][i*2+1]=SCREEN_VAN_Y+(StackPlate[polyI][i*2+1]-SCREEN_VAN_Y)/(1.+plateExtendFB); }}
    StackPlate[polyI][8]=END_POLY;
    if (!extraPlateCreated) {
      plateExtendLR=PLATE_EXTEND_LR; row--;
      plateExtendFB=PLATE_EXTEND_FB; extraPlateCreated=YES; extraRowSep=DISTRIBUTOR_EXTRA_ROW_SEP;
      rowSep=DISTRIBUTOR_ROW_SEP; }
    else {
      extraRowSep=0.; }
    row++; }}



//  Obtain the bounds of a single (specified) glyph from the Distributor font.
//
//  See the file “Polygon Constants” for a detailed description of how the polygon tags work.

void extractDistributorGlyphBounds(int4 c, DoubleRect_ *rect) {

  bool  found=NO ;
  int4  i=0L     ;

  //  Find the specified glyph in the font.
  while (!found) {
    if      (DistributorFont[i]==END_POLY) {
      i=1L; found=YES; }
    else if (DistributorFont[i]==c       ) {
      i++ ; found=YES; }
    else {
      while (DistributorFont[i]!=END_POLY) i++;
      i++; }}

  //  Determine the bounds of the glyph.
  rect->L= 1.;
  rect->B= 1.;
  rect->R=-1.;
  rect->T=-1.;
  while  (DistributorFont[i   ]!=END_POLY) {
    if   (DistributorFont[i   ]==NEW_LOOP) i++;
    else {
      if (DistributorFont[i   ]==SPLINE  ) i++;
      if (DistributorFont[i   ]< rect->L ) rect->L=DistributorFont[i   ];
      if (DistributorFont[i+1L]< rect->B ) rect->B=DistributorFont[i+1L];
      if (DistributorFont[i   ]> rect->R ) rect->R=DistributorFont[i   ];
      if (DistributorFont[i+1L]> rect->T ) rect->T=DistributorFont[i+1L];
      i+=2L; }}}



//  Calculate the character positions of the distributor-info text.
//
//  Returns an error string, or nil (not an empty string) if there was no error.

BYTE *prepareDistInfoChars() {

  double  xPos=0., caseFrac ;
  int4    i=0L ;

  //  Error check.
  if (!CbfDistInfo) return "prepareDistInfoChars called with nil CbfDistInfo.";

  //  First, determine the positions of all characters with text left-justified from center of movie screen.
  while (DistInfoText[i]) {
    //  Make the character smaller if it’s lowercase (but still use the uppercase glyph).
    DistInfoChar[i].L=xPos; caseFrac=1.; if (isLowerCase(DistInfoText[i])) caseFrac=DIST_INFO_LCASE_FRAC;
    //  Determine the width of the character.
    xPos+=(double) cbfWidth (CbfDistInfo,&DistInfoText[i])*DIST_INFO_WIDTH_MULT
    *caseFrac     /cbfHeight(CbfDistInfo)*(DIST_INFO_TOP-DIST_INFO_BOT);
    DistInfoChar[i].R=xPos;
    if (DistInfoText[++i]) xPos+=(DIST_INFO_TOP-DIST_INFO_BOT)*DIST_INFO_SPACER_FRAC*DIST_INFO_WIDTH_MULT; }
  DistInfoChars=i;

  //  Then, move all the characters to the left so that the text is centered on the movie screen.
  for (i=0; i<DistInfoChars; i++) {
    DistInfoChar[i].L-=xPos/2.;
    DistInfoChar[i].R-=xPos/2.; }

  //  Success.
  return nil; }



//  Renders one pixel of the Distributor image.
//
//  Note:  screenX, screenY, pR, pG, and pB are initialized only to prevent compiler warnings -- they shouldn’t need to be.

void drawDistributorPixel(BYTE *r, BYTE *g, BYTE *b, int4 pixelX, int4 pixelY,
int4 *cbfChar, BYTE *cbfFill, short *cbfX, short *cbfY, bool *cbfRaw, BYTE **cbfData,
BYTE *distributorImage, bool inRender) {

  double  charPosL, charPosR, textFill, bevelFill, sideFill, plateFill, distInfoFill, rotScreenY, rotVanY ;
  double  distInfoCharTop, distInfoCharBot, distInfoCharBaseline, x, y, dX, dY, screenX=0, screenY=0      ;
  double  towerFill, towerR, towerG, towerB, towerCount, towerX, towerY, towerMaskFrac, logoFrac          ;
  double  plateR, plateG, plateB, pR=0, pG=0, pB=0, plateCount, searchlightMaskFrac, slope ;
  double  closeness, sine, sineCount, sideR, sideG, sideB, glowFrac, snowFrac ;
  double  subPixelPart=1./((double) RasterSubPixels*(double) RasterSubPixels) ;
  int4    k, ii, jj, towerBytePos, distInfoPixelX, distInfoPixelY, *ptrToLong ;
  double  searchlightMaskAccum=0., distributorRowRiseT, distributorRowRiseB   ;
  double  searchlightNearR, searchlightNearG, searchlightNearB ;
  double  searchlightFarR , searchlightFarG , searchlightFarB  ;
  double  backdropR=0., backdropG=0., backdropB=0. ;
  double  subPixelPartOver255=subPixelPart/255.    ;
  BYTE    searchlightMaskLevel, glyphLevel         ;
  double  bevelR=0., bevelG=0., bevelB=0.          ;
  double  bevSx, bevSy, bevEx, bevEy               ;
  int4    charI, row, inPlate, c                   ;
  bool    inText ;

  //  Letterbox to black, if appropriate for this pixel.
  if (pixelY< ImageHeiLetterBox
  ||  pixelY>=ImageHeiLetterBox+ImageHeiUsed) {
    *r=(BYTE) 0;
    *g=(BYTE) 0;
    *b=(BYTE) 0; }
  else {

    //  Construct the image pixel.
    textFill=0.; bevelFill=0.; sideFill=0.; sideR=0.; sideG=0.; sideB=0.; sineCount=0.;
    plateFill=0.; plateR=0.; plateG=0.; plateB=0.; plateCount=0.;
    towerFill=0.; towerR=0.; towerG=0.; towerB=0.; towerCount=0.; distInfoFill=0.;
    searchlightFarR =0.; searchlightFarG =0.; searchlightFarB =0.;
    searchlightNearR=0.; searchlightNearG=0.; searchlightNearB=0.;
    for   (jj=0L; jj<(int4) RasterSubPixels; jj++) {   //    vertical subpixel loop
      screenY  =pixelToScreenY((double) pixelY+((double) jj+.5)/(double) RasterSubPixels);
      for (ii=0L; ii<(int4) RasterSubPixels; ii++) {   //  horizontal subpixel loop
        screenX=pixelToScreenX((double) pixelX+((double) ii+.5)/(double) RasterSubPixels);
        x=screenX;
        y=screenY; distributorRevPerspective(&x,&y);

        //  Accumulate the background image for smooth resizing.
        k=DistributorBackdropRowBytes
        *(int4) (((double) (pixelY-ImageHeiLetterBox)+(.5+(double) jj)/(double) RasterSubPixels)/(double) ImageHeiUsed
        *(double) DistributorBackdropHei)
        +(int4) (((double)  pixelX                   +(.5+(double) ii)/(double) RasterSubPixels)/(double) ImageWid
        *(double) DistributorBackdropWid);
        backdropR+=(double) DistributorBackdrop[0][k];
        backdropG+=(double) DistributorBackdrop[1][k];
        backdropB+=(double) DistributorBackdrop[2][k];

        //  Accumulate the searchlight mask image for smooth resizing.
        searchlightMaskAccum+=DistributorSearchlightMaskLev[0][DistributorSearchlightMaskRowBytes
        *(int4) (((double) (pixelY-ImageHeiLetterBox)+(.5+(double) jj)/(double) RasterSubPixels)/(double) ImageHeiUsed
        *(double) DistributorBackdropHei)
        +(int4) (((double)  pixelX                   +(.5+(double) ii)/(double) RasterSubPixels)/(double) ImageWid
        *(double) DistributorBackdropWid)];

        if (y>DAIS_BOTTOM || (VanY-y)/(VanX-x)<(VanY-DAIS_BOTTOM)/(VanX-1.)) {   //  mask the dais

          if (screenX>=DISTRIBUTOR_LOGO_RECT_L
          &&  screenY<=DISTRIBUTOR_LOGO_RECT_T
          &&  screenX< DISTRIBUTOR_LOGO_RECT_R
          &&  screenY> DISTRIBUTOR_LOGO_RECT_B) {   //  Save time by excluding large portions of the image where the logo can never be.

            //  Stack-plates that separate rows of text in the logo (part 1).
            inPlate=-1;
            for (row=DistributorRows-1; row>=0; row--) {
              if (pointInSplinePoly(&StackPlate[row*3][0],screenX,screenY)) {
                dX=StackPlate[row*3][0]  -StackPlate[row*3][2];
                dY=StackPlate[row*3][1]  -StackPlate[row*3][3];
                rotScreenY =(screenY     -StackPlate[row*3][1])*dX-(screenX     -StackPlate[row*3][0])*dY;
                rotVanY    =(SCREEN_VAN_Y-StackPlate[row*3][1])*dX-(SCREEN_VAN_X-StackPlate[row*3][0])*dY;
                glowFrac=1.5-50.*rotScreenY/rotVanY;
                if (glowFrac> 1.) glowFrac=1.;
                if (glowFrac>=0.) {
                  pR=PLATE_BOTTOM_LIGHT_R*glowFrac+PLATE_BOTTOM_DARK_R*(1.-glowFrac);
                  pG=PLATE_BOTTOM_LIGHT_G*glowFrac+PLATE_BOTTOM_DARK_G*(1.-glowFrac);
                  pB=PLATE_BOTTOM_LIGHT_B*glowFrac+PLATE_BOTTOM_DARK_B*(1.-glowFrac); }
                else {
                  glowFrac=1./(1.-glowFrac/5.);
                  pR=PLATE_BOTTOM_DARK_R*glowFrac+SIDE_DARK_R*(1.-glowFrac);
                  pG=PLATE_BOTTOM_DARK_G*glowFrac+SIDE_DARK_G*(1.-glowFrac);
                  pB=PLATE_BOTTOM_DARK_B*glowFrac+SIDE_DARK_B*(1.-glowFrac); }
                inPlate=row; if (inPlate>0) inPlate--;
                break; }
              else if (pointInSplinePoly(&StackPlate[row*3+1][0],screenX,screenY)) {
                pR=distributorTextColorR(screenX,screenY);
                pG=distributorTextColorG(screenX,screenY);
                pB=distributorTextColorB(screenX,screenY); inPlate=row; if (inPlate>0) inPlate--;
                break; }
              else if (pointInSplinePoly(&StackPlate[row*3+2][0],screenX,screenY)) {
                pR=PLATE_SIDE_R;
                pG=PLATE_SIDE_G;
                pB=PLATE_SIDE_B; inPlate=row; if (inPlate>0) inPlate--;
                break; }}

            //  This loop determines whether the test point is in any characters of the logo’s text.
            inText=NO;
            for (row=inPlate+1; row<DistributorRows; row++) {
              if (y<=DistributorRow[row].top && y>=DistributorRow[row].top-DistributorRow[row].hei) {
                charPosL=-1.; if (DistributorRow[row].wid<2.) charPosL=-DistributorRow[row].wid/2.;
                for (charI=DistributorRow[row].start; charI<DistributorRow[row].end; charI++) {
                  charPosR=charPosL+DistributorGlyph[DistributorText[charI]].width*DistributorRow[row].scaleX;
                  if (x>=charPosL && x<=charPosR) {
                    if   (pointInSplinePoly     (&DistributorFont[DistributorGlyph[DistributorText[charI]].start],
                    (  x-DistributorRow[row].scaleX-charPosL)/DistributorRow[row].scaleX,(y-DistributorRow[row].top)*(DistributorRow[row].rise-DistributorRow[row].desc)/DistributorRow[row].hei+DistributorRow[row].rise)) {
                      if (pointInSplinePolyBevel(&DistributorFont[DistributorGlyph[DistributorText[charI]].start],
                      (x-DistributorRow[row].scaleX-charPosL)/DistributorRow[row].scaleX,(y-DistributorRow[row].top)*(DistributorRow[row].rise-DistributorRow[row].desc)/DistributorRow[row].hei+DistributorRow[row].rise,
                      DistributorGlyph[DistributorText[charI]].width/(charPosR-charPosL),2./DistributorRow[row].hei,&bevSx,&bevSy,&bevEx,&bevEy)) {
                        bevelFill+=subPixelPart;
                        accumulateBevelColor(screenX,screenY,bevSx,bevSy,bevEx,bevEy,&bevelR,&bevelG,&bevelB,subPixelPart); }
                      else {
                        textFill +=subPixelPart; }
                      inText=YES; inPlate=-1; }
                    break; }
                  if (x>charPosR && x<charPosR+DISTRIBUTOR_CHAR_SEP*DistributorRow[row].scaleX) break;
                  charPosL=charPosR+DISTRIBUTOR_CHAR_SEP*DistributorRow[row].scaleX; }
                break; }}

            //  This loop determines whether the test point is in the text’s dark, extrusion sides.
            if (!inText && y<1. && (screenY-SCREEN_VAN_Y)/(SCREEN_VAN_X-screenX)<DistributorMovieScreenSlopeMax) {
              row=DistributorRows-1; closeness=NOT_EVEN_CLOSE;
              if (x<=1.) while (row &&  y               > DistributorRow[row].top                ) row--;
              else       while (row && (y-VanY)/(VanX-x)>(DistributorRow[row].top-VanY)/(VanX-1.)) row--;
              while (row>=0 && row>=inPlate+1) {
                distributorRowRiseT=(DistributorRow[row].top                        -VanY);
                distributorRowRiseB=(DistributorRow[row].top-DistributorRow[row].hei-VanY);
                charPosR=1.; if (DistributorRow[row].wid<2.) charPosR=DistributorRow[row].wid/2.;
                for (charI=DistributorRow[row].end-1; charI>=DistributorRow[row].start; charI--) {
                  charPosL=charPosR-DistributorGlyph[DistributorText[charI]].width*DistributorRow[row].scaleX;
                  slope=(y-VanY)/(VanX-x);
                  if (slope<=distributorRowRiseT/(VanX-charPosR)
                  &&  slope>=distributorRowRiseB/(VanX-charPosL)) {
                    closeness=polyExtrusionCloseness(&DistributorFont[DistributorGlyph[DistributorText[charI]].start],
                    (   x-DistributorRow[row].scaleX-charPosL)/DistributorRow[row].scaleX,(   y-DistributorRow[row].top)*(DistributorRow[row].rise-DistributorRow[row].desc)/DistributorRow[row].hei+DistributorRow[row].rise,
                    (VanX-DistributorRow[row].scaleX-charPosL)/DistributorRow[row].scaleX,(VanY-DistributorRow[row].top)*(DistributorRow[row].rise-DistributorRow[row].desc)/DistributorRow[row].hei+DistributorRow[row].rise,&sine);
                    if (closeness>NOT_EVEN_CLOSE) {
                      row=0; break; }}
                  charPosR=charPosL-DISTRIBUTOR_CHAR_SEP*DistributorRow[row].scaleX; }
                row--; }
              if (closeness> NOT_EVEN_CLOSE
              &&  closeness>-EXTRUSION_LENGTH) {
                sideFill+=subPixelPart; sineCount++; inPlate=-1;
                //  Normalize the sine so it runs from 0-1, bottom edge of characters to top edge.
                if (sine>0.) sine=   .5*sine*sine;
                else         sine=1.-.5*sine*sine;
                //  Use the sine to decide how to color the extrusion side at this test point.
                if (sine<.8) {   //  (orange glow cast up from the hidden, front spotlight)
                  if      (sine<.45) glowFrac=1.;
                  else if (sine<.6 ) glowFrac=(.6-sine)/(.6-.45);
                  else               glowFrac=0.;
                  if (y<0.) glowFrac/=1.+10.*y*y;   //  make the glow fade off toward the bottom of the logo
                  if (x>0.) glowFrac/=1.+10.*x*x;   //  make the glow fade off toward the right side of the logo
                  sideR+=SIDE_GLOW_R*glowFrac+SIDE_DARK_R*(1.-glowFrac);
                  sideG+=SIDE_GLOW_G*glowFrac+SIDE_DARK_G*(1.-glowFrac);
                  sideB+=SIDE_GLOW_B*glowFrac+SIDE_DARK_B*(1.-glowFrac); }
                else {   //  (ghostly, “snow” fringe at the top edge of the logo’s characters)
                  snowFrac=(sine-.8)/.2; snowFrac=1.-(1.-snowFrac)*(1.-snowFrac);
                  if (y<0.) snowFrac/=1.+10.*y*y;   //  make the “snow” fade off toward the bottom of the logo
                  sideR+=SIDE_SNOW_R*snowFrac+SIDE_DARK_R*(1.-snowFrac);
                  sideG+=SIDE_SNOW_G*snowFrac+SIDE_DARK_G*(1.-snowFrac);
                  sideB+=SIDE_SNOW_B*snowFrac+SIDE_DARK_B*(1.-snowFrac); }}}

            //  Stack-plates that separate rows of text in the logo (part 2).
            if (inPlate>=0) {
              plateFill+=subPixelPart; plateCount++; plateR+=pR; plateG+=pG; plateB+=pB; }}}

        //  Distributor Info text.  (Warning:  This code assumes that every glyph is supported in the compressed bitmap font.
        //  Unsupported glyphs must be “fixed” (when then font is loaded into RAM) to use an uppercase version, or just the
        //  tilde glyph or any other supported glyph.)
        if (DistInfoChars
        &&  screenY<=DIST_INFO_TOP
        &&  screenY> DIST_INFO_BOT
        &&  screenX>=DistInfoChar[              0].L
        &&  screenX< DistInfoChar[DistInfoChars-1].R) {
          for (charI=0; charI<DistInfoChars; charI++) {
            if (screenX>=DistInfoChar[charI].L
            &&  screenX< DistInfoChar[charI].R) {
              c=DistInfoText[charI];
              if (isLowerCase(c)) {
                distInfoCharBaseline=DIST_INFO_BOT+(DIST_INFO_TOP-DIST_INFO_BOT)*DIST_INFO_BASELINE_FRAC;
                distInfoCharTop     =distInfoCharBaseline+(DIST_INFO_TOP-distInfoCharBaseline)*DIST_INFO_LCASE_FRAC;
                distInfoCharBot     =distInfoCharBaseline-(distInfoCharBaseline-DIST_INFO_BOT)*DIST_INFO_LCASE_FRAC; }
              else {
                distInfoCharTop     =DIST_INFO_TOP;
                distInfoCharBot     =DIST_INFO_BOT; }
              if (screenY<=distInfoCharTop && screenY>distInfoCharBot) {
                distInfoPixelX=(int4) ((double) cbfWidth (CbfDistInfo,&c)
                *(screenX              -DistInfoChar[charI].L)
                /(DistInfoChar[charI].R-DistInfoChar[charI].L));
                distInfoPixelY=(int4) ((double) cbfHeight(CbfDistInfo   )
                *(distInfoCharTop-screenY        )
                /(distInfoCharTop-distInfoCharBot));
                //  Repopulate the font decompression variables, if they’re not in a state to use as-is.
                if ((int4) cbfY[jj]!=distInfoPixelY || cbfChar[jj]!=c || (int4) cbfX[jj]>distInfoPixelX) {
                  ptrToLong=(int4 *) &CbfDistInfo[8L+4L*(int4) c];
                  cbfChar[jj]=c; cbfFill[jj]=(BYTE) 0; cbfX[jj]=0; cbfY[jj]=(short) distInfoPixelY; cbfRaw[jj]=NO;
                  ptrToLong=(int4 *) (CbfDistInfo+(*ptrToLong)+4L+4L*cbfY[jj]);
                  cbfData[jj]=CbfDistInfo+(*ptrToLong); }
                //  First, find the region (fill or raw) that includes the desired pixel.
                while (cbfX[jj]+ (short) cbfData[jj][0]<=distInfoPixelX) {
                  cbfX     [jj]+=(short) cbfData[jj][0];
                  if (cbfRaw[jj]) {   //   raw mode
                    if (cbfData[jj][0]) {
                      cbfData[jj]+=cbfData[jj][0];
                      if (cbfData[jj][0]<(BYTE) 128) cbfFill[jj]=(BYTE)   0;
                      else                           cbfFill[jj]=(BYTE) 255; }
                    else {
                      cbfFill[jj]=(BYTE) 255-cbfFill[jj]; }
                    cbfData[jj]++; cbfRaw[jj]=NO; }
                  else {              //  fill mode
                    if (cbfData[jj][0]==(BYTE) 255) {
                      cbfData[jj]++; }
                    else {
                      cbfData[jj]++; cbfRaw[jj]=YES; }}}
                //  Then determine the greyscale level (0-255) of that pixel.
                if (cbfRaw[jj]) {     //   raw mode
                  glyphLevel=cbfData[jj][1+distInfoPixelX-cbfX[jj]]; }
                else {                //  fill mode
                  glyphLevel=cbfFill[jj]; }
                //  Use the pixel value that was found.
                distInfoFill+=(double) glyphLevel*subPixelPartOver255; }}}}

        //  Searchlight beams.
        searchlightCalc(screenX,screenY,&searchlightFarR ,&searchlightFarG ,&searchlightFarB ,subPixelPart, NO,!inRender || DistributorImageReady);
        searchlightCalc(screenX,screenY,&searchlightNearR,&searchlightNearG,&searchlightNearB,subPixelPart,YES,!inRender || DistributorImageReady);

        //  Front searchlight tower that overlaps the logo extrusion sides (and overlaps its own searchlight beam).
        if (screenX>=FRONT_TOWER_L
        &&  screenX< FRONT_TOWER_R
        &&  screenY<=FRONT_TOWER_T
        &&  screenY> FRONT_TOWER_B) {
          towerX=                             (screenX-FRONT_TOWER_L)/(FRONT_TOWER_R-FRONT_TOWER_L)*(double) DistributorTowerWid;
          towerY=(double) DistributorTowerHei-(screenY-FRONT_TOWER_B)/(FRONT_TOWER_T-FRONT_TOWER_B)*(double) DistributorTowerHei;
          if ((int4) towerX<DistributorTowerWid && (int4) towerY<DistributorTowerHei) {
            //  (Really, this should perform a weighted average instead of just using one pixel -- but it seems to look fine the way it is, perhaps due to overscan rendering.)
            towerMaskFrac=(double) DistributorTowerMaskLev[0][DistributorTowerMaskRowBytes*(int4) towerY+(int4) towerX]/255.;
            towerBytePos=DistributorTowerRowBytes*(int4) towerY+(int4) towerX;
            towerR+=(double) DistributorTowerRgb[0][towerBytePos]*towerMaskFrac; towerFill +=towerMaskFrac*subPixelPart;
            towerG+=(double) DistributorTowerRgb[1][towerBytePos]*towerMaskFrac; towerCount+=towerMaskFrac             ;
            towerB+=(double) DistributorTowerRgb[2][towerBytePos]*towerMaskFrac; }}}}

    if ( sineCount==0.)  sineCount=.000001;   //  prevent division-by-zero
    if (plateCount==0.) plateCount=.000001;

    logoFrac=(1.-textFill-bevelFill-sideFill-plateFill);

    *r=(BYTE) (
    backdropR*subPixelPart*logoFrac                 +
    textFill *distributorTextColorR(screenX,screenY)+   //  (using leftover screenX,screenY values, but that’s OK)
    bevelR                                          +
    sideFill * sideR/ sineCount                     +
    plateFill*plateR/plateCount                      );

    *g=(BYTE) (
    backdropG*subPixelPart*logoFrac                 +
    textFill *distributorTextColorG(screenX,screenY)+
    bevelG                                          +
    sideFill * sideG/ sineCount                     +
    plateFill*plateG/plateCount                      );

    *b=(BYTE) (
    backdropB*subPixelPart*logoFrac                 +
    textFill *distributorTextColorB(screenX,screenY)+
    bevelB                                          +
    sideFill * sideB/ sineCount                     +
    plateFill*plateB/plateCount                      );

    searchlightMaskLevel=searchlightMaskAccum*subPixelPartOver255;
    if (searchlightMaskLevel) {
      if (searchlightFarR>0. || searchlightFarG>0. || searchlightFarB>0.) {
        searchlightMaskFrac=(double) searchlightMaskLevel*logoFrac;
        *r=(BYTE) (255.-(255.-(double) (*r))*(1.-searchlightFarR *searchlightMaskFrac));
        *g=(BYTE) (255.-(255.-(double) (*g))*(1.-searchlightFarG *searchlightMaskFrac));
        *b=(BYTE) (255.-(255.-(double) (*b))*(1.-searchlightFarB *searchlightMaskFrac)); }
      if (searchlightNearR>0. || searchlightNearG>0. || searchlightNearB>0.) {
        searchlightMaskFrac=(double) searchlightMaskLevel;
        *r=(BYTE) (255.-(255.-(double) (*r))*(1.-searchlightNearR*searchlightMaskFrac));
        *g=(BYTE) (255.-(255.-(double) (*g))*(1.-searchlightNearG*searchlightMaskFrac));
        *b=(BYTE) (255.-(255.-(double) (*b))*(1.-searchlightNearB*searchlightMaskFrac)); }}

    if (towerCount>0.) {
      *r=(BYTE) ((double) (*r)*(1.-towerFill)+towerR/towerCount*towerFill);
      *g=(BYTE) ((double) (*g)*(1.-towerFill)+towerG/towerCount*towerFill);
      *b=(BYTE) ((double) (*b)*(1.-towerFill)+towerB/towerCount*towerFill); }

    if (distInfoFill>0.) {
      *r=(BYTE) ((double) (*r)*(1.-distInfoFill)+DIST_INFO_R*distInfoFill);
      *g=(BYTE) ((double) (*g)*(1.-distInfoFill)+DIST_INFO_G*distInfoFill);
      *b=(BYTE) ((double) (*b)*(1.-distInfoFill)+DIST_INFO_B*distInfoFill); }

    //  Fade-down the whole image.
    //  (This is a subtractive, not a multiplicative fade.  It looks more like an old-time movie fade that way.)
    if (DistributorFade && inRender && DistributorImageReady) {
      if ((*r)<DistributorFade)  *r =        (BYTE) 0;
      else                      (*r)-=DistributorFade;
      if ((*g)<DistributorFade)  *g =        (BYTE) 0;
      else                      (*g)-=DistributorFade;
      if ((*b)<DistributorFade)  *b =        (BYTE) 0;
      else                      (*b)-=DistributorFade; }}

  //  Force the image to black-and-white.
  *r=*g;
  *b=*g; }



//  Used by drawDistributorPixel to determine the color of a logo character’s bevelled edge.

void accumulateBevelColor(
double screenX, double screenY, double sX, double sY, double eX, double eY, double *r, double *g, double *b, double frac) {

  double  dist, angle, level, red, green, blue, shadow ;

  //  Get the spotlight shadow multiplier.
  shadow=computeShadows(screenX,screenY,.5,1.);

  //  Turn the angle of the slope-line into a color.
  eX-=sX; eY-=sY; dist=sqrt(eX*eX+eY*eY);
  angle=acos(eX/dist); level=angle*2./CIRCLE_RADIANS; level*=level; level=.05+.95*level;

  red  =(double) BEVEL_COLOR_R*level*DISTRIBUTOR_BEVEL_STRENGTH*shadow; if (red  >255.) red  =255.;
  green=(double) BEVEL_COLOR_G*level*DISTRIBUTOR_BEVEL_STRENGTH*shadow; if (green>255.) green=255.;
  blue =(double) BEVEL_COLOR_B*level*DISTRIBUTOR_BEVEL_STRENGTH*shadow; if (blue >255.) blue =255.;

  *r+=red  *frac;
  *g+=green*frac;
  *b+=blue *frac; }



//  Forces the re-rendering of the area in which the Distributor Info text falls.
//
//  Note:  The controller never calls this function with render threads running.

void forceRasterDistInfo(bool lo, bool hi) {
  if (DistInfoChars) {
    forceRasterRect(lo,hi,
    screenToPixelX(DistInfoChar[              0].L)-1,
    screenToPixelY(DIST_INFO_TOP                  )-1,
    screenToPixelX(DistInfoChar[DistInfoChars-1].R)+2,
    screenToPixelY(DIST_INFO_BOT                  )+2); }}



//  Forces the re-rendering of the area in which the moving searchlights fall (in the Distributor image).
//
//  Note:  The math by which the searchlight values are converted to a shape in the image is found not only in
//         this function, but also in searchlightCalc().  If you change the way it works here, you should
//         seriously consider changing it there, too.
//
//  Note:  The controller never calls this function with render threads running.

void forceRasterSearchlight(bool lo, bool hi) {

  #define  ARB_EXT     10.   //  Arbitrary extension of searchlight beam from its center.
  #define  SAFETY_PAD   2.   //  Prevents seachlights from leaving faint lines behind them in the sky.

  double  edgeLsx, edgeLsy, edgeLex, edgeLey, theCos, screenT, angle ;
  double  edgeRsx, edgeRsy, edgeRex, edgeRey, theSin, screenB ;
  int4    i ;

  //  Loop through the searchlights, ignoring stationary ones.
  for (i=0; i<SEARCHLIGHT_COUNT; i++) {
    if (SearchlightAmplitude[i]>0.) {

      //  Build a horizontal, searchlight parallelagram that starts at the origin, and fans across the positive X-axis.
      edgeLsx=     0.;  angle=SearchlightAngle[i];
      edgeLex=ARB_EXT; theCos=cos(angle*CIRCLE_RADIANS);
      edgeRsx=     0.; theSin=sin(angle*CIRCLE_RADIANS);
      edgeRex=ARB_EXT;
      edgeLsy=SearchlightOuterSize[i]*SAFETY_PAD;
      edgeRsy=-edgeLsy;
      edgeLey=SearchlightOuterSize[i]*SAFETY_PAD*ARB_EXT*SearchlightExpansion[i];
      edgeRey=-edgeLey;

      //  Rotate the horizontal searchbeam parallelagram around to its on-screen angle.
      rotatePointAroundOrigin(&edgeLsx,&edgeLsy,theCos,theSin);
      rotatePointAroundOrigin(&edgeRsx,&edgeRsy,theCos,theSin);
      rotatePointAroundOrigin(&edgeLex,&edgeLey,theCos,theSin);
      rotatePointAroundOrigin(&edgeRex,&edgeRey,theCos,theSin);

      //  Translate the parallelagram to its on-screen location.
      edgeLsx+=SearchlightX[i];
      edgeLsy+=SearchlightY[i];
      edgeLex+=SearchlightX[i];
      edgeLey+=SearchlightY[i];
      edgeRsx+=SearchlightX[i];
      edgeRsy+=SearchlightY[i];
      edgeRex+=SearchlightX[i];
      edgeRey+=SearchlightY[i];

      //  Correct the scope of the parallelegram so it fits the top and bottom of the screen.  (It doesn’t matter if it fits
      //  the left and right sides of the screen.  Also, this code assumes that the searchlight never leans over so far as
      //  to cause a math/logic issue; i.e. this code is designed around the sweep of the searchlights in the original movie
      //  logo.)
      screenT=MOVIE_SCREEN_HEI*.5; screenB=-screenT;
      edgeLex=edgeLsx+(edgeLex-edgeLsx)*(screenT-edgeLsy)/(edgeLey-edgeLsy); edgeLey=screenT;
      edgeRex=edgeRsx+(edgeRex-edgeRsx)*(screenT-edgeRsy)/(edgeRey-edgeRsy); edgeRey=screenT;
      edgeLsx=edgeLex+(edgeLsx-edgeLex)*(screenB-edgeLey)/(edgeLsy-edgeLey); edgeLsy=screenB;
      edgeRsx=edgeRex+(edgeRsx-edgeRex)*(screenB-edgeRey)/(edgeRsy-edgeRey); edgeRsy=screenB;

      //  Force the area of the parallelgram to be re-rendered.
      forceRasterParallelagram(lo,hi,
      (int4) screenToPixelX(edgeLex)  ,
      (int4) screenToPixelX(edgeRex)+1,
      (int4) screenToPixelY(edgeLey)  ,
      (int4) screenToPixelX(edgeLsx)  ,
      (int4) screenToPixelX(edgeRsx)+1,
      (int4) screenToPixelY(edgeLsy)+1); }}}



//  Used by drawDistributorPixel to calculate the RGB searchlight component of a pixel that might fall within one
//  of the rendered searchlights (in the Distributor image).
//
//  Note:  The math by which the searchlight values are converted to a shape in the image is found not only in
//         this function, but also in forceRasterSearchlight().  If you change the way it works here, you should
//         seriously consider changing it there, too.

void searchlightCalc(double screenX, double screenY, double *r, double *g, double *b, double fillFrac, bool isNear,
bool showMovingSearchlights) {

  double  fracR=1., fracG=1., fracB=1., x, y, rotX, rotY, innerY, outerY, angle, theCos, theSin, bright ;
  double  aura, auraStartup, auraFadeoff, preCalc ;
  int4    i ;

  for (i=0; i<SEARCHLIGHT_COUNT; i++) {
    if (SearchlightNear[i]==isNear && (showMovingSearchlights || SearchlightAmplitude[i]==0.)) {
      angle =SearchlightAngle    [i];
      bright=SearchlightIntensity[i]; aura=bright/2.;
      rotX=screenX-SearchlightX[i]; theCos=cos(angle*CIRCLE_RADIANS);
      rotY=screenY-SearchlightY[i]; theSin=sin(angle*CIRCLE_RADIANS); x=rotX*theCos+rotY*theSin;
      if (x>0.) {
        y=rotY*theCos-rotX*theSin; if (y<0.) y*=-1.;
        innerY=SearchlightInnerSize[i]+x*SearchlightExpansion[i];
        if (y<=innerY) {   //  (searchlight beam)
          preCalc=1.-bright;
          fracR*=preCalc;
          fracG*=preCalc;
          fracB*=preCalc; }
        else {             //  (searchlight beam’s aura)
          outerY=innerY*SearchlightOuterSize[i]/SearchlightInnerSize[i];
          if (y<=outerY) {
            auraFadeoff=aura*(outerY-y)/(outerY-innerY);
            if (x>=SearchlightOuterSize[i]) {
              preCalc=1.-auraFadeoff;
              fracR*=preCalc;
              fracG*=preCalc;
              fracB*=preCalc; }
            else {
              auraStartup=x/SearchlightOuterSize[i]; preCalc=1.-auraFadeoff*auraStartup;
              fracR*=preCalc;
              fracG*=preCalc;
              fracB*=preCalc; }}}}}}

  *r+=fillFrac*(1.-fracR);
  *g+=fillFrac*(1.-fracG);
  *b+=fillFrac*(1.-fracB); }



//  Used by drawDistributorPixel when determining which extrusion side a specified pixel is part of (if any).
//
//  See the file “Polygon Constants” for a detailed description of how the polygon tags work.
//
//  Returns the largest negative value found, normalized for a pretend-distance of 1.0 from the testpoint to the
//  vanishing point.
//
//  This function is closely related to pointInSplinePoly().
//
//  See this webpage for an explanation of the point-in-spline-polygon technique:  http://alienryderflex.com/polyspline

double polyExtrusionCloseness(double *poly, double x, double y, double vanX, double vanY, double *sine) {

  #define  TINY_SLOPE_SEGMENT  .00001

  double  sX, sY, eX, eY, a, b, sRoot, f, plusOrMinus, topPart, bottomPart, xPart, yPart ;
  double  dist, theCos, theSin, dX, dY, dDist, closeness, closest=NOT_EVEN_CLOSE ;
  int4    i=0, j, k, start=0 ;
  bool    foundHit=NO ;

  y+=.000001;   //  (Prevent the need for special tests when f is exactly 0 or 1.)

  //  Translate the vanishing point so the point x,y is at the origin.
  vanX-=x; vanY-=y; dist=sqrt(vanX*vanX+vanY*vanY); theCos=vanX/dist; theSin=vanY/dist; closest*=dist*1.001;

  while (poly[i]!=END_POLY) {
    j=i+2; if (poly[i]==SPLINE) j++;
    if (poly[j]==END_POLY || poly[j]==NEW_LOOP) j=start;

    if (poly[i]!=SPLINE   && poly[j]!=SPLINE  ) {   //  LINE SEGMENT
      sX=poly[i]; sY=poly[i+1];
      eX=poly[j]; eY=poly[j+1];
      //  Translate the line segment so the point x,y is at the origin.
      sX-=x; sY-=y; eX-=x; eY-=y;
      //  Rotate the line segment so the vanishing point is on the positive X axis.
      rotatePointAroundOrigin(&sX,&sY,theCos,-theSin);
      rotatePointAroundOrigin(&eX,&eY,theCos,-theSin);
      //  Check the line segment to see if/where it crosses the X axis.
      if (sY<0. && eY>=0.
      ||  eY<0. && sY>=0.) {
        dX=eX-sX;
        dY=eY-sY; closeness=sX-sY/dY*dX;
        if (closeness>closest && closeness<0.) {
          closest=closeness; foundHit=YES; dDist=sqrt(dX*dX+dY*dY); if (dX<0.) dY*=-1.;
          *sine=dY/dDist; }}}

    else if (poly[j]==SPLINE) {                     //  SPLINE CURVE
      a=poly[j+1]; b=poly[j+2]; k=j+3; if (poly[k]==END_POLY || poly[k]==NEW_LOOP) k=start;
      if (poly[i]!=SPLINE) {
        sX=poly[i]; sY=poly[i+1]; }
      else {
        sX=(poly[i+1]+poly[j+1])/2.; sY=(poly[i+2]+poly[j+2])/2.; }
      if (poly[k]!=SPLINE) {
        eX=poly[k]; eY=poly[k+1]; }
      else {
        eX=(poly[j+1]+poly[k+1])/2.; eY=(poly[j+2]+poly[k+2])/2.; }
      //  Translate the spline so the point x,y is at the origin.
      sX-=x; sY-=y; a-=x; b-=y; eX-=x; eY-=y;
      //  Rotate the spline so the vanishing point is on the positive X axis.
      rotatePointAroundOrigin(&sX,&sY,theCos,-theSin);
      rotatePointAroundOrigin(&eX,&eY,theCos,-theSin);
      rotatePointAroundOrigin(& a,& b,theCos,-theSin);
      //  Check the spline to see if/where it crosses the X axis.
      bottomPart=2.*(sY+eY-b-b);
      if (bottomPart==0.) {
        b+=.0001; bottomPart-=.0004; }
      sRoot=2.*(b-sY); sRoot*=sRoot; sRoot-=2.*bottomPart*sY;
      if (sRoot>=0.) {
        sRoot=sqrt(sRoot); topPart=2.*(sY-b);
        for (plusOrMinus=-1.; plusOrMinus<1.1; plusOrMinus+=2.) {
          f=(topPart+plusOrMinus*sRoot)/bottomPart;
          if (f>=0. && f<=1.) {
            xPart=sX+f*(a-sX); closeness=xPart+f*(a+f*(eX-a)-xPart);
            if (closeness>closest && closeness<0.) {
              closest=closeness; f+=TINY_SLOPE_SEGMENT; foundHit=YES;
              xPart=sX+f*(a-sX); dX=xPart+f*(a+f*(eX-a)-xPart)-closeness;
              yPart=sY+f*(b-sY); dY=yPart+f*(b+f*(eY-b)-yPart);
              dDist=sqrt(dX*dX+dY*dY); if (dX<0.) dY*=-1.;
              *sine=dY/dDist; }}}}}

    //  Advance through the polygon data.  (See the file “Spline Corner Logic”.)
    if (poly[i]==SPLINE) i++;
    i+=2;
    if (poly[i]==NEW_LOOP) {
      i++; start=i; }}

  if (foundHit) return closest/dist  ;
  else          return NOT_EVEN_CLOSE; }



//  Used to compute simple shadows that are cast over the left and right sides of the
//  Distributor logo -- the areas where the hidden, front spotlight doesn’t shine.

double computeShadows(double x, double y, double side, double mid) {

  #define   LEFT_HARDNESS  9.
  #define  RIGHT_HARDNESS  6.5

  double  f ;

  //  Left shadow.
  f=(x*3+y*.75+1.05)*LEFT_HARDNESS;
  if (f>1.) f=1.;
  if (f<0.) f=0.;
  if (f<1.) return side*(1.-f)+mid*f;

  //  Right shadow.
  f=(-x*3.+y*2.1+.4)*RIGHT_HARDNESS;
  if (f>1.) f=1.;
  if (f<0.) f=0.;
  return side*(1.-f)+mid*f; }



//  Used to factor in the affect of the hidden, front spotlight on parts of the Distributor logo.

double computeFlood(double y) {

  double  f=.65-y*1.5 ;

  if (f<0.) f=0.;
  if (f>1.) f=1.;
  return f; }



//  Calculates the red component of the front-face text color in the Distributor
//  logo, taking into account the front floodlight and shadows that affect that color.

double distributorTextColorR(double x, double y) {

  double  f=computeFlood(y) ;

  return computeShadows(x,y,TEXT_SHADOW_R,TEXT_BRIGHT_R*f+TEXT_DIM_R*(1.-f)); }



//  Calculates the green component of the front-face text color in the Distributor
//  logo, taking into account the front floodlight and shadows that affect that color.

double distributorTextColorG(double x, double y) {

  double  f=computeFlood(y) ;

  return computeShadows(x,y,TEXT_SHADOW_G,TEXT_BRIGHT_G*f+TEXT_DIM_G*(1.-f)); }



//  Calculates the blue component of the front-face text color in the Distributor
//  logo, taking into account the front floodlight and shadows that affect that color.

double distributorTextColorB(double x, double y) {

  double  f=computeFlood(y) ;

  return computeShadows(x,y,TEXT_SHADOW_B,TEXT_BRIGHT_B*f+TEXT_DIM_B*(1.-f)); }



//  Determines whether or not a specified point is inside a specified spline-polygon.  Returns a non-
//  zero value if the point is inside, or zero otherwise.
//
//  See the file “Polygon Constants” for a detailed description of how the polygon tags work.
//
//  See this webpage for an explanation of the point-in-spline-polygon technique:  http://alienryderflex.com/polyspline

int4 pointInSplinePoly(double *poly, double x, double y) {

  double  sX, sY, eX, eY, a, b, sRoot, f, topPart, bottomPart, xPart ;
  int4    i=0, j, k, ii, jj, start=0, nodeCount=0 ;

  y+=.000001;   //  (Prevent the need for special tests when f is exactly 0 or 1.)

  while (poly[i]!=END_POLY) {
    j=i+2; if (poly[i]==SPLINE) j++;
    if (poly[j]==END_POLY || poly[j]==NEW_LOOP) j=start;

    if (poly[i]!=SPLINE   && poly[j]!=SPLINE  ) {   //  LINE SEGMENT
      ii=i+1; jj=j+1;
      if (poly[ii]<y && poly[jj]>=y
      ||  poly[jj]<y && poly[ii]>=y) {
        if (poly[i]+(y-poly[ii])/(poly[jj]-poly[ii])*(poly[j]-poly[i])<x) nodeCount++; }}

    else if (poly[j]==SPLINE) {                     //  SPLINE CURVE
      a=poly[j+1]; b=poly[j+2]; k=j+3; if (poly[k]==END_POLY || poly[k]==NEW_LOOP) k=start;
      if (poly[i]!=SPLINE) {
        sX=poly[i]; sY=poly[i+1]; }
      else {   //  interpolate a hard corner
        sX=(poly[i+1]+poly[j+1])/2.; sY=(poly[i+2]+poly[j+2])/2.; }
      if (poly[k]!=SPLINE) {
        eX=poly[k]; eY=poly[k+1]; }
      else {   //  interpolate a hard corner
        eX=(poly[j+1]+poly[k+1])/2.; eY=(poly[j+2]+poly[k+2])/2.; }
      bottomPart=2.*(sY+eY-b-b);
      if (bottomPart==0.) {   //  prevent division-by-zero
        b+=.0001; bottomPart-=.0004; }
      sRoot=2.*(b-sY); sRoot*=sRoot; sRoot-=2.*bottomPart*(sY-y);
      if (sRoot>=0.) {
        sRoot=sqrt(sRoot); topPart=2.*(sY-b);
        f=(topPart+sRoot)/bottomPart;
        if (f>=0. && f<=1.) {
          xPart=sX+f*(a-sX); if (xPart+f*(a+f*(eX-a)-xPart)<x) nodeCount++; }
        f=(topPart-sRoot)/bottomPart;
        if (f>=0. && f<=1.) {
          xPart=sX+f*(a-sX); if (xPart+f*(a+f*(eX-a)-xPart)<x) nodeCount++; }}}

    //  Advance through the polygon data.  (See the file “Spline Corner Logic”.)
    if (poly[i]==SPLINE) i++;
    i+=2;
    if (poly[i]==NEW_LOOP) {
      i++; start=i; }}

  return nodeCount&1; }



//  Used by drawDistributorPixel to determine whether or not a pixel is inside the bevelled edge of one of the
//  Distributor logo’s characters.
//
//  * If the test point is not in the polygon’s bevel, returns NO and does nothing to the bevel vector variables.
//    (Note that in that case there will be no indication of whether the test point is inside or outside of the
//    polygon -- probably you should first use pointInSplinePoly to determine whether the test point is in the
//    polygon at all, then call this function if it is.)
//  * If the test point is in the polygon’s bevel, returns YES and sets the bevel vector variables to indicate
//    the slope of the bevel at the test point.  (Note that the particular startpoint and magnitude of the bevel
//    vector are arbitrary -- only its direction should be considered meaningful by the calling code.)
//
//  This function is closely related to pointInSplinePoly().
//
//  See the file “Polygon Constants” for a detailed description of how the polygon tags work.
//
//  See this webpage for an explanation of the point-in-spline-polygon technique:  http://alienryderflex.com/polyspline

bool pointInSplinePolyBevel(double *poly, double x, double y, double bevScaleX, double bevScaleY,
double *bevSx, double *bevSy, double *bevEx, double *bevEy) {

  double  sX, sY, eX, eY, a, b, bevelPoly[15], dxA, dyA, distA, dxB, dyB, distB ;
  //  The following six variables are initialized only to prevent compiler warnings -- they shouldn’t need to be:
  double  dxP=1, dyP=1, distP=1, dxN=1, dyN=1, distN=1 ;

  int4  i=0L, j, k, ii, jj, start=0L, prev, next ;

  y+=.000001;   //  (Prevent the need for special tests when F is exactly 0 or 1.)

  //  Loop through bevel the polygons, checking each for bevel inclusion.
  while (poly[i]!=END_POLY) {
    j=i+2; if (poly[i]==SPLINE) j++;
    if (poly[j]==END_POLY || poly[j]==NEW_LOOP) j=start;

    if (poly[i]!=SPLINE   && poly[j]!=SPLINE  ) {   //  LINE SEGMENT
      prev=prevPoint(poly,start,i); ii=i+1;
      next=nextPoint(poly,start,j); jj=j+1;
      dxA=poly[j     ]-poly[i     ];
      dyA=poly[jj    ]-poly[ii    ]; distA=sqrt(dxA*dxA+dyA*dyA);      
      dxP=poly[i     ]-poly[prev  ];
      dyP=poly[ii    ]-poly[prev+1]; distP=sqrt(dxP*dxP+dyP*dyP);
      dxN=poly[next  ]-poly[j     ];
      dyN=poly[next+1]-poly[jj    ]; distN=sqrt(dxN*dxN+dyN*dyN);
      bevelPoly[0]=poly[i ];
      bevelPoly[1]=poly[ii];
      bevelPoly[2]=poly[j ];
      bevelPoly[3]=poly[jj];
      bevelPoly[8]=END_POLY;
      if (!linesIntersect(
      poly[i     ]+dyA/distA*BEVEL_SIZE*bevScaleX,
      poly[ii    ]-dxA/distA*BEVEL_SIZE*bevScaleY,
      poly[j     ]+dyA/distA*BEVEL_SIZE*bevScaleX,
      poly[jj    ]-dxA/distA*BEVEL_SIZE*bevScaleY,
      poly[j     ]+dyN/distN*BEVEL_SIZE*bevScaleX,
      poly[jj    ]-dxN/distN*BEVEL_SIZE*bevScaleY,
      poly[next  ]+dyN/distN*BEVEL_SIZE*bevScaleX,
      poly[next+1]-dxN/distN*BEVEL_SIZE*bevScaleY,&bevelPoly[4],&bevelPoly[5])) {
        bevelPoly[4]=poly[j ]+dyA/distA*BEVEL_SIZE*bevScaleX;
        bevelPoly[5]=poly[jj]-dxA/distA*BEVEL_SIZE*bevScaleY; }
      if (!linesIntersect(
      poly[j     ]+dyA/distA*BEVEL_SIZE*bevScaleX,
      poly[jj    ]-dxA/distA*BEVEL_SIZE*bevScaleY,
      poly[i     ]+dyA/distA*BEVEL_SIZE*bevScaleX,
      poly[ii    ]-dxA/distA*BEVEL_SIZE*bevScaleY,
      poly[i     ]+dyP/distP*BEVEL_SIZE*bevScaleX,
      poly[ii    ]-dxP/distP*BEVEL_SIZE*bevScaleY,
      poly[prev  ]+dyP/distP*BEVEL_SIZE*bevScaleX,
      poly[prev+1]-dxP/distP*BEVEL_SIZE*bevScaleY,&bevelPoly[6],&bevelPoly[7])) {
        bevelPoly[6]=poly[i ]+dyA/distA*BEVEL_SIZE*bevScaleX;
        bevelPoly[7]=poly[ii]-dxA/distA*BEVEL_SIZE*bevScaleY; }

      //  Offset the inside edge of the bevel to the upper-left, to approximate a 3D look.
      bevelPoly  [4]-=BEVEL_SIZE*BEVEL_OFFSET_FRAC;
      bevelPoly  [5]+=BEVEL_SIZE*BEVEL_OFFSET_FRAC;
      bevelPoly  [6]-=BEVEL_SIZE*BEVEL_OFFSET_FRAC;
      bevelPoly  [7]+=BEVEL_SIZE*BEVEL_OFFSET_FRAC;

      if (pointInSplinePoly(&bevelPoly[0],x,y)) {
        *bevSx=bevelPoly[0];
        *bevSy=bevelPoly[1];
        *bevEx=bevelPoly[2];
        *bevEy=bevelPoly[3]; return YES; }}

    else if (poly[j]==SPLINE) {                     //  SPLINE CURVE
      a=poly[j+1]; b=poly[j+2]; k=j+3; if (poly[k]==END_POLY || poly[k]==NEW_LOOP) k=start;
      if (poly[i]!=SPLINE) {
        sX=poly[i]; sY=poly[i+1]; prev=prevPoint(poly,start,i);
        dxP=sX-poly[prev  ]   ;
        dyP=sY-poly[prev+1]   ; distP=sqrt(dxP*dxP+dyP*dyP); }
      else {   //  interpolate a hard corner
        sX=(poly[i+1]+poly[j+1])/2.; sY=(poly[i+2]+poly[j+2])/2.; prev=-1L; }
      if (poly[k]!=SPLINE) {
        eX=poly[k]; eY=poly[k+1]; next=nextPoint(poly,start,k);
        dxN=   poly[next  ]-eX;
        dyN=   poly[next+1]-eY; distN=sqrt(dxN*dxN+dyN*dyN); }
      else {   //  interpolate a hard corner
        eX=(poly[j+1]+poly[k+1])/2.; eY=(poly[j+2]+poly[k+2])/2.; next=-1L; }
      dxA= a-sX;
      dyA= b-sY; distA=sqrt(dxA*dxA+dyA*dyA);
      dxB=eX- a;
      dyB=eY- b; distB=sqrt(dxB*dxB+dyB*dyB);
      bevelPoly[0]=    sX;
      bevelPoly[1]=    sY;
      bevelPoly[2]=SPLINE;
      bevelPoly[3]=     a;
      bevelPoly[4]=     b;
      bevelPoly[5]=    eX;
      bevelPoly[6]=    eY;
      if (next<0L || !linesIntersect(
      a           +dyB/distB*BEVEL_SIZE*bevScaleX,
      b           -dxB/distB*BEVEL_SIZE*bevScaleY,
      eX          +dyB/distB*BEVEL_SIZE*bevScaleX,
      eY          -dxB/distB*BEVEL_SIZE*bevScaleY,
      eX          +dyN/distN*BEVEL_SIZE*bevScaleX,
      eY          -dxN/distN*BEVEL_SIZE*bevScaleY,
      poly[next  ]+dyN/distN*BEVEL_SIZE*bevScaleX,
      poly[next+1]-dxN/distN*BEVEL_SIZE*bevScaleY,&bevelPoly[7],&bevelPoly[8])) {
        bevelPoly[7]=eX+dyB/distB*BEVEL_SIZE*bevScaleX;
        bevelPoly[8]=eY-dxB/distB*BEVEL_SIZE*bevScaleY; }
      bevelPoly  [9]=SPLINE;
      if (prev<0L || !linesIntersect(
      a           +dyA/distA*BEVEL_SIZE*bevScaleX,
      b           -dxA/distA*BEVEL_SIZE*bevScaleY,
      sX          +dyA/distA*BEVEL_SIZE*bevScaleX,
      sY          -dxA/distA*BEVEL_SIZE*bevScaleY,
      sX          +dyP/distP*BEVEL_SIZE*bevScaleX,
      sY          -dxP/distP*BEVEL_SIZE*bevScaleY,
      poly[prev  ]+dyP/distP*BEVEL_SIZE*bevScaleX,
      poly[prev+1]-dxP/distP*BEVEL_SIZE*bevScaleY,&bevelPoly[12],&bevelPoly[13])) {
        bevelPoly[12]=sX+dyA/distA*BEVEL_SIZE*bevScaleX;
        bevelPoly[13]=sY-dxA/distA*BEVEL_SIZE*bevScaleY; }
      bevelPoly  [14]=END_POLY;
      if (linesIntersect(bevelPoly[7],bevelPoly[8],
      a+dyB/distB*BEVEL_SIZE*bevScaleX,
      b-dxB/distB*BEVEL_SIZE*bevScaleY,
      a+dyA/distA*BEVEL_SIZE*bevScaleX,
      b-dxA/distA*BEVEL_SIZE*bevScaleY,bevelPoly[12],bevelPoly[13],&bevelPoly[10],&bevelPoly[11])) {

        //  Offset the inside edge of the bevel to the upper-left, to approximate a 3D look.
        bevelPoly[ 7]-=BEVEL_SIZE*BEVEL_OFFSET_FRAC;
        bevelPoly[ 8]+=BEVEL_SIZE*BEVEL_OFFSET_FRAC;
        bevelPoly[10]-=BEVEL_SIZE*BEVEL_OFFSET_FRAC;
        bevelPoly[11]+=BEVEL_SIZE*BEVEL_OFFSET_FRAC;
        bevelPoly[12]-=BEVEL_SIZE*BEVEL_OFFSET_FRAC;
        bevelPoly[13]+=BEVEL_SIZE*BEVEL_OFFSET_FRAC;

        if (pointInSplinePoly(&bevelPoly[0],x,y)) {
          splineSlopeNearPoint(x,y,bevelPoly[0],bevelPoly[1],bevelPoly[3],bevelPoly[4],bevelPoly[5],bevelPoly[6],
          bevSx,bevSy,bevEx,bevEy);
          return YES; }}}

    //  Advance through the polygon data.  (See the file “Spline Corner Logic”.)
    if (poly[i]==SPLINE) i++;
    i+=2;
    if (poly[i]==NEW_LOOP) {
      i++; start=i; }}

  //  The testpoint was not found to be inside a bevel.
  return NO; }



//  Convenience function used by pointInSplinePolyBevel.  Finds the previous point’s
//  coordinate pair, whether or not it’s a spline point.
//
//  See the file “Polygon Constants” for a detailed description of how the polygon tags work.

int4 prevPoint(double *poly, int4 start, int4 i) {
  if (i==start) {
    while (poly[i]!=END_POLY && poly[i]!=NEW_LOOP) {
      if  (poly[i]==SPLINE) i++;
      i+=2L; }}
  return i-2L; }



//  Convenience function used by pointInSplinePolyBevel.  Finds the next point’s coordinate
//  pair, whether or not it’s a spline point.
//
//  See the file “Polygon Constants” for a detailed description of how the polygon tags work.

int4 nextPoint(double *poly, int4 start, int4 i) {
  if (poly[i]==SPLINE) i++;
  i+=2L;  if (poly[i]==END_POLY || poly[i]==NEW_LOOP) i=start;
  if (poly[i]==SPLINE) i++;
  return i; }
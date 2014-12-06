//  Star Wars TSG
//  Darel Rex Finley, 2006-2011



//  This file contains structures that are shared among many parts of the sequence (Distributor, Film Company, Intro, etc.).



#include                "nil.h"
#include               "bool.h"
#include        "definitions.h"
#include "geometry functions.h"
#include      "TitleAndCrawl.h"

#include  "Shared Structures.h"



//  Globals used during rendering of all preview images.
int4    PreviewRowBytes ;
BYTE  **PreviewRgb=nil  ;



//  Dimensions of the image window, in which the user can see the preview images and the rendering activity.
int4  ImageWid=320, ImageHei=136, ImageHeiUsed=136, ImageHeiLetterBox=0 ;



//  Frame rate.
double  FramesPerSecond=24. ;



//  Globals that control pixel-by-pixel raster-order rendering.  See the
//  RasterRow_ struct definition, and the file “Raster Segments”, for more info.
int4        RasterSubPixels, RasterNextAvailRow ;
RasterRow_  Raster[IMAGE_HEI_MAX] ;
int4        RasterLoHi ;   //  (0=lo, 1=hi)



//  These functions should be used to set ImageWid, ImageHei, and ImageHeiUsed
//  -- they automatically re-populate the value of ImageHeiLetterBox.

void setImageWid    (int4 w) {
  ImageWid    =w; calcLetterBox(); }

void setImageHei    (int4 h) {
  ImageHei    =h; calcLetterBox(); }

void setImageHeiUsed(int4 h) {
  ImageHeiUsed=h; calcLetterBox(); }



//  Forces a parallelagram-shaped area of the image to be re-rendered.  This parallelagram has
//  horizontal top and bottom sides, but may have arbitrarily (and independently) angled left and
//  right sides.
//
//  Note:  The controller never calls this function with render threads running.

void forceRasterParallelagram(bool lo, bool hi, int4 lt, int4 rt, int4 t, int4 lb, int4 rb, int4 b) {

  int4    j, L, R ;
  double  frac    ;

  for (j=t; j<b; j++) {
    if (j>=0 && j<ImageHei) {
      frac=((double) j-(double) t)/((double) b-(double) t);
      L=(int4) ((double) lt+((double) lb-(double) lt)*frac)  ;
      R=(int4) ((double) rt+((double) rb-(double) rt)*frac)+1;
      if (L<ImageWid && R>0 && L<R) {
        if (L<       0) L=       0;
        if (R>ImageWid) R=ImageWid;
        if (lo) addRasterSegment(&Raster[j].left[0][0],&Raster[j].right[0][0],L,R);
        if (hi) addRasterSegment(&Raster[j].left[1][0],&Raster[j].right[1][0],L,R); }}}}



//  Forces a rectangular area of the image to be re-rendered.
//
//  Note:  The rectangle does not have to be pre-confined to the image area -- that
//         will happen automatically in forceRasterParallelagram which this function
//         uses.
//
//  Important:  Passed parameters should be in pixel coordinates; positive Y goes
//              downward from 0.  (i.e. top<bottom)
//
//  Note:  The controller never calls this function with render threads running.

void forceRasterRect(bool lo, bool hi, int4 left, int4 top, int4 right, int4 bottom) {
  forceRasterParallelagram(lo,hi,left,right,top,left,right,bottom); }



//  Forces the whole image to be re-rendered.
//
//  Note:  The controller never calls this function with render threads running.

void forceRasterWholeImage(bool lo, bool hi) {
  forceRasterRect(lo,hi,0,0,ImageWid,ImageHei); }



//  Clears out the Raster array so that nothing will be re-rendered.
//
//  Note:  The controller never calls this function with render threads running.

void nullRaster(bool lo, bool hi) {

  int4  i, j ;

  for   (i=0; i<               ImageHei; i++) {
    for (j=0; j<MAX_RENDER_SEGS_PER_ROW; j++) {
      if (lo) {
        Raster[i].left [0][j]=0;
        Raster[i].right[0][j]=0; }
      if (hi) {
        Raster[i].left [1][j]=0;
        Raster[i].right[1][j]=0; }}}}



//  Re-populate ImageHeiLetterBox.  To be called whenever ImageHei and/or ImageHeiUsed are changed.

void calcLetterBox() {
  ImageHeiLetterBox=(ImageHei-ImageHeiUsed)/2; }



//  Used by the function forceRasterParallelagram to add a set of pixels to the Raster array.
//
//  This function *assumes* that newA and newB are within the proper range (0 to ImageWid,
//  inclusive) and that newA is less than newB.
//
//  Note:  minI is initialized only to prevent a compiler warning.  It shouldn’t need to be.
//
//  Note:  See the file “Raster Segments” for a graphic depiction of what is going on here.

void addRasterSegment(int4 *a, int4 *b, int4 newA, int4 newB) {

  int4  i, j, openSlot=-1, diff, minI=0, minJ=0, minDiff ;

  //  Search for open slot into which new segment can be placed,
  //  handling various segment-overlap conditions at the same time.
  for (i=0; i<MAX_RENDER_SEGS_PER_ROW; i++) {
    if (a[i]>=b[i]) openSlot=i;
    else {
      if      (newA>=a[i] && newB<=b[i]) return;   //  Exit if the new segment is already covered.
      if      (newA<=a[i] && newB>=b[i]) {         //  If the existing segment is covered by the new segment..
        b[i]=a[i]; openSlot=i; }                   //  ..then kill the existing segment.
      else if (newA> a[i] && newA<=b[i]) {         //  If the existing segment is partially covered by the new segment..
        b[i]=a[i]; openSlot=i; newA=a[i]; }        //  ..then absorb the existing segment into the new segment.
      else if (newB>=a[i] && newB< b[i]) {         //  If the existing segment is partially covered by the new segment..
        a[i]=b[i]; openSlot=i; newB=b[i]; }}}      //  ..then absorb the existing segment into the new segment.

  //  If an open slot was found, put the new segment there and exit.
  if (openSlot>=0) {
    a[openSlot]=newA;
    b[openSlot]=newB; return; }

  //  No open slot -- must now combine two non-overlapping segments:
  minDiff=ImageWid;
  //  First, find which existing segment is closest to the new segment.
  for (i=0; i<MAX_RENDER_SEGS_PER_ROW; i++) {
    if (newA>b[i]) diff=newA-b[i];
    else           diff=a[i]-newB;
    if (diff<minDiff) {
      minDiff=diff; minI=i; }}
  //  Next, see if any two existing segments are closer to each other than that.
  for   (i=  0; i<MAX_RENDER_SEGS_PER_ROW-1; i++) {
    for (j=i+1; j<MAX_RENDER_SEGS_PER_ROW  ; j++) {
      if (a[i]>b[j]) diff=a[i]-b[j];
      else           diff=a[j]-b[i];
      if (diff<minDiff) {
        minDiff=diff; minI=i; minJ=j; }}}
  //  Either merge two existing segments..
  if (minJ) {
    if (a[minI]>b[minJ]) {
      a  [minI]=a[minJ]; a[minJ]=newA; b[minJ]=newB; }
    else {
      a  [minJ]=a[minI]; a[minI]=newA; b[minI]=newB; }}
  //  ..or merge an existing segment with the new segment.
  else {
    if (newA>b[minI]) b[minI]=newB;
    else              a[minI]=newA; }}



//  Used by the function pointInSplineArcPoly to handle straight line segments when processing a polygon.

void handleLineSegment(
double x, double y, double sX, double sY, double eX, double eY, int4 *nodeCount) {
  if (sY<y && eY>=y
  ||  eY<y && sY>=y) {
    if (sX+(y-sY)/(eY-sY)*(eX-sX)<x) (*nodeCount)++; }}



//  Used by the functions addNodesFromSplineArcPoly_FilmCo and addNodesFromSplineArcPoly_Title to handle
//  straight line segments when processing a polygon -- see the comments at those functions.

void handleLineSegmentForNodesRow(double y, double sX, double sY, double eX, double eY, int4 subPixelRow,
int4 polyCount, int4 *nodes, double *nodeX, int4 *polyTag) {

  int4  base ;

  if (sY<y && eY>=y
  ||  eY<y && sY>=y) {
    if (nodes[subPixelRow]<NODE_ROW_MAX) {
      base=subPixelRow*NODE_ROW_MAX;
      nodeX  [base+nodes[subPixelRow]]=sX+(y-sY)/(eY-sY)*(eX-sX);
      polyTag[base+nodes[subPixelRow]]=polyCount; nodes[subPixelRow]++; }}}



//  Called only once, on app launch, to verify that the expected data sizes are correct.
//
//  Returns an error message, or nil (not an empty string) if there is no error.

BYTE *verifyDataTypeSizes() {

  if (sizeof( BYTE)!=1
  ||  sizeof(short)!=2
  ||  sizeof( int4)!=4) {
    return "The fundamental data types are not sized for this app to compile correctly."; }

  //  Success.
  return nil; }
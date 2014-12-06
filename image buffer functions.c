//  Star Wars TSG
//  Darel Rex Finley, 2006-2011



#include                   "bool.h"
#include            "definitions.h"
#include      "Shared Structures.h"

#include "image buffer functions.h"



//  Fills the image with a specified color.
//
//  Note:  Even on a slower computer, this seems to execute fast enough to not
//         cause a user-irritating pause in the UI.  It is not multi-threaded.

void fillImage(short r, short g, short b) {

  BYTE  *rowR, *rowG, *rowB ;
  int4   i, j, preCalc      ;

  //  Exit if there is no bitmap to fill.
  if (!PreviewRgb) return;

  //  Fill the bitmap with a solid color.
  for (j=0L; j<ImageHei; j++) {
    preCalc=j*PreviewRowBytes;
    rowR=&PreviewRgb[0][preCalc];
    rowG=&PreviewRgb[1][preCalc];
    rowB=&PreviewRgb[2][preCalc];
    for (i=0L; i<ImageWid; i++) {
      rowR[i]=(BYTE) r;
      rowG[i]=(BYTE) g;
      rowB[i]=(BYTE) b; }}}



//  Copies the image to a specified buffer.
//
//  Warning:  Do not call this without sufficient allocated memory at the
//            location pointed to by ÒbufÓ.
//
//  Note:  Even on a slower computer, this seems to execute fast enough to not
//         cause a user-irritating pause in the UI.  It is not multi-threaded.

void copyImageToBuffer(BYTE *buf) {

  BYTE  *rowR, *rowG, *rowB, *bufRowR, *bufRowG, *bufRowB ;
  int4   i, j, preCalc ;

  for (j=0L; j<ImageHei; j++) {
    preCalc=j*PreviewRowBytes;
    rowR=&PreviewRgb[0][preCalc]; bufRowR=&buf[(0L*ImageHei+j)*ImageWid];
    rowG=&PreviewRgb[1][preCalc]; bufRowG=&buf[(1L*ImageHei+j)*ImageWid];
    rowB=&PreviewRgb[2][preCalc]; bufRowB=&buf[(2L*ImageHei+j)*ImageWid];
    for (i=0L; i<ImageWid; i++) {
      bufRowR[i]=rowR[i];
      bufRowG[i]=rowG[i];
      bufRowB[i]=rowB[i]; }}}



//  Copies a specified buffer to the image, applying a fade in the process.
//
//  Note:  Even on a slower computer, this seems to execute fast enough to not
//         cause a user-irritating pause in the UI.  It is not multi-threaded.
//
//  Very closely related to copyBufferToImage; kept separate for purposes of speed.
//
//  Uses a subtractive fade that looks more like an traditional movie fade than the
//  simpler, multiplicative fade.

void copyBufferToImageWithSubtractiveFade(BYTE *buf, BYTE subtractVal) {

  BYTE  *rowR, *rowG, *rowB, *bufRowR, *bufRowG, *bufRowB ;
  int4   i, j ;

  for   (j=0L; j<ImageHei; j++) {
    rowR=&PreviewRgb[0][j*PreviewRowBytes]; bufRowR=&buf[(0L*ImageHei+j)*ImageWid];
    rowG=&PreviewRgb[1][j*PreviewRowBytes]; bufRowG=&buf[(1L*ImageHei+j)*ImageWid];
    rowB=&PreviewRgb[2][j*PreviewRowBytes]; bufRowB=&buf[(2L*ImageHei+j)*ImageWid];
    for (i=0L; i<ImageWid; i++) {
      if (bufRowR[i]>subtractVal) rowR[i]=bufRowR[i]-subtractVal;
      else                        rowR[i]=              (BYTE) 0;
      if (bufRowG[i]>subtractVal) rowG[i]=bufRowG[i]-subtractVal;
      else                        rowG[i]=              (BYTE) 0;
      if (bufRowB[i]>subtractVal) rowB[i]=bufRowB[i]-subtractVal;
      else                        rowB[i]=              (BYTE) 0; }}}



//  Copies a specified buffer to the image.
//
//  Note:  Even on a slower computer, this seems to execute fast enough to not
//         cause a user-irritating pause in the UI.  It is not multi-threaded.

void copyBufferToImage(BYTE *buf) {

  BYTE  *rowR, *rowG, *rowB, *bufRowR, *bufRowG, *bufRowB ;
  int4   i, j, preCalc ;

  for (j=0L; j<ImageHei; j++) {
    preCalc=j*PreviewRowBytes;
    rowR=&PreviewRgb[0][preCalc]; bufRowR=&buf[(0L*ImageHei+j)*ImageWid];
    rowG=&PreviewRgb[1][preCalc]; bufRowG=&buf[(1L*ImageHei+j)*ImageWid];
    rowB=&PreviewRgb[2][preCalc]; bufRowB=&buf[(2L*ImageHei+j)*ImageWid];
    for (i=0L; i<ImageWid; i++) {
      rowR[i]=bufRowR[i];
      rowG[i]=bufRowG[i];
      rowB[i]=bufRowB[i]; }}}



//  Copies a specified buffer to the image, applying a fade in the process.
//
//  Note:  Even on a slower computer, this seems to execute fast enough to not
//         cause a user-irritating pause in the UI.  It is not multi-threaded.
//
//  Very closely related to copyBufferToImage; kept separate for purposes of speed.
//
//  Uses a simple, multiplicative fade.

void copyBufferToImageWithMultFade(BYTE *buf, double fadeFrac) {

  BYTE  *rowR, *rowG, *rowB, *bufRowR, *bufRowG, *bufRowB ;
  int4   i, j, preCalc ;

  for (j=0L; j<ImageHei; j++) {
    preCalc=j*PreviewRowBytes;
    rowR=&PreviewRgb[0][preCalc]; bufRowR=&buf[(0L*ImageHei+j)*ImageWid];
    rowG=&PreviewRgb[1][preCalc]; bufRowG=&buf[(1L*ImageHei+j)*ImageWid];
    rowB=&PreviewRgb[2][preCalc]; bufRowB=&buf[(2L*ImageHei+j)*ImageWid];
    for (i=0L; i<ImageWid; i++) {
      rowR[i]=(BYTE) ((double) bufRowR[i]*fadeFrac+.5);
      rowG[i]=(BYTE) ((double) bufRowG[i]*fadeFrac+.5);
      rowB[i]=(BYTE) ((double) bufRowB[i]*fadeFrac+.5); }}}



//  Used by the functions drawCrawlPixel, drawFilmCoPixel, and drawFilmCoPreviewPixel to re-order the whole-row
//  rendering nodes.
//
//  See the webpage http://alienryderflex.com/polygon_fill for information about whole-row node rendering.

void sortNodeRow(int4 subPixelRow, int4 *nodes, double *nodeX, int4 *polyTag) {

  int4    swapL, base=subPixelRow*NODE_ROW_MAX ;
  double  swapD ;
  int4    i=0   ;

  //  Simple, ÒbubbleÓ sort.  (Not super-efficient, but probably doesnÕt need to be.)
  while (i<nodes[subPixelRow]-1) {
    if (nodeX[base+i  ]>nodeX  [base+i+1]) {
      swapD            =nodeX  [base+i  ];
      nodeX  [base+i  ]=nodeX  [base+i+1];
      nodeX  [base+i+1]=swapD            ;
      swapL            =polyTag[base+i  ];
      polyTag[base+i  ]=polyTag[base+i+1];
      polyTag[base+i+1]=swapL            ; if (i) i--; }
    else {
      i++; }}}



//  Used by renderingThread to dump its local pixel buffers to the global image bitmap.  Also clears out the
//  .raster segments as it dumps them.  Returns YES if any pixels were actually transfered to the bitmap.
//
//  Important:  To minimize cache conflicts, only one thread should be executing this function at a time --
//              BitmapLock should be locked before calling this function.

bool emptyRenderRowBuffers(RenderRow_ *renderRow, int4 pixelBuffer, int4 pixelBufferRasterSeg, int4 pixelX, int4 rasterLoHi) {

  int4  buf, seg, rowOffset, x ;
  bool  anyPixelsDumped=NO     ;

  //  Dump the completed rows.
  for (buf=0; buf<pixelBuffer; buf++) {
    rowOffset=PreviewRowBytes*renderRow[buf].y;
    for (seg=0; seg<MAX_RENDER_SEGS_PER_ROW; seg++) {
      for (
      x=renderRow[buf].raster.left [rasterLoHi][seg];
      x<renderRow[buf].raster.right[rasterLoHi][seg]; x++) {
        PreviewRgb[0][rowOffset+x]=renderRow[buf].r[x];
        PreviewRgb[1][rowOffset+x]=renderRow[buf].g[x];
        PreviewRgb[2][rowOffset+x]=renderRow[buf].b[x]; anyPixelsDumped=YES; }
      renderRow[buf].raster.left [rasterLoHi][seg]=
      renderRow[buf].raster.right[rasterLoHi][seg]; }}

  if (pixelBuffer<PIXEL_ROW_BUFFERS) {

    //  Dump the completed segments of the uncompleted row.
    rowOffset=PreviewRowBytes*renderRow[pixelBuffer].y;
    for (seg=0; seg<pixelBufferRasterSeg; seg++) {
      for (
      x=renderRow[pixelBuffer].raster.left [rasterLoHi][seg];
      x<renderRow[pixelBuffer].raster.right[rasterLoHi][seg]; x++) {
        PreviewRgb[0][rowOffset+x]=renderRow[pixelBuffer].r[x];
        PreviewRgb[1][rowOffset+x]=renderRow[pixelBuffer].g[x];
        PreviewRgb[2][rowOffset+x]=renderRow[pixelBuffer].b[x]; anyPixelsDumped=YES; }
      renderRow[pixelBuffer].raster.left [rasterLoHi][seg]=
      renderRow[pixelBuffer].raster.right[rasterLoHi][seg]; }

    //  Dump the uncompleted segment of the uncompleted row.
    for (x=renderRow[pixelBuffer].raster.left[rasterLoHi][pixelBufferRasterSeg]; x<pixelX; x++) {
      PreviewRgb[0][rowOffset+x]=renderRow[pixelBuffer].r[x];
      PreviewRgb[1][rowOffset+x]=renderRow[pixelBuffer].g[x];
      PreviewRgb[2][rowOffset+x]=renderRow[pixelBuffer].b[x]; anyPixelsDumped=YES; }
    renderRow[pixelBuffer].raster.left[rasterLoHi][pixelBufferRasterSeg]=pixelX; }

  return anyPixelsDumped; }
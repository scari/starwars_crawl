//  Star Wars TSG
//  Darel Rex Finley, 2006-2011



#import              "definitions.h"
#import  "DistributorSearchlights.h"
#import              "Distributor.h"
#import            "TitleAndCrawl.h"
#import                   "FilmCo.h"
#import                    "Intro.h"
#import            "SequenceTimes.h"
#import            "cbf functions.h"
#import   "image buffer functions.h"
#import        "numeric functions.h"
#import       "geometry functions.h"
#import         "string functions.h"
#import "user interface functions.h"
#import        "Shared Structures.h"
#import                     "undo.h"
#import                <sys/xattr.h>

#import    "StarWarsTSGController.h"

#if COMPRESS_FONTS
#import "Font Compression Info (DistInfo).c"
#import "Font Compression Info (Intro).c"
#import "Font Compression Info (Crawl).c"
#endif



//  Multithreaded-rendering control variables.
NSLock  *RenderLock                 =nil ;
NSLock  *BitmapLock                 =nil ;
bool     KillRenderThreads          =NO  ;
bool     LastRenderImageUpdateNeeded=NO  ;
int4     RenderThreads              =0   ;



//  This is used to ensure that the proper variables will be set up before the first preview render occurs.
bool  UserContentChanged_EverCalled=NO ;



//  These are necessary because when the user changes the contents of a text field, or clicks on a radio button, the
//  OS provides no way for the invoked function to know what the old setting of the text field or radio button was.
int4  DistributorFieldContents[MAX_CHARS_DISTRIBUTOR+1] ;
int4     DistInfoFieldContents[MAX_CHARS_DIST_INFO  +1] ;
int4       FilmCoFieldContents[MAX_CHARS_FILM_CO    +1] ;
int4    FilmCoIncFieldContents[MAX_CHARS_FILM_CO_INC+1] ;
int4        IntroFieldContents[MAX_CHARS_INTRO      +1] ;
int4        TitleFieldContents[MAX_CHARS_TITLE      +1] ;
int4        CrawlFieldContents[MAX_CHARS_CRAWL      +1] ;
int4        WidthFieldContents[MAX_CHARS_FIELD_INT  +1] ;
int4       HeightFieldContents[MAX_CHARS_FIELD_INT  +1] ;
int4   HeightUsedFieldContents[MAX_CHARS_FIELD_INT  +1] ;
int4    FrameRateFieldContents[MAX_CHARS_FIELD_FLOAT+1] ;
int4  DestinationFieldContents[MAX_CHARS_FIELD_PATH +1] ;
int4    StarfieldFieldContents[MAX_CHARS_FIELD_PATH +1] ;
int4  ScreenSizeRadioButtonsSelectedRow=0 ;
int4   FrameRateRadioButtonsSelectedRow=0 ;



//  Globals that controls row-by-row or column-by-column image blurring.
int4  BlurRow, BlurCol ;



//  OS image structures.
NSBitmapImageRep  *PreviewBitmap=nil, *PreviewBitmapCopy=nil, *BackdropBitmap=nil, *TowerBitmap=nil, *TowerMaskBitmap=nil, *SearchlightMaskBitmap=nil ;
NSData            *TowerData=nil, *TowerMaskData=nil, *SearchlightMaskData=nil, *DistInfoFontData=nil, *IntroFontData=nil, *CrawlFontData=nil, *DistBackdropData=nil ;
NSImage           *TheImage=nil ;

//  Image widths and heights.
int4  PreviewBitmapWid, PreviewBitmapHei, TowerBitmapWid, TowerBitmapHei ;

//  Status variables.
bool           CustomImageSize=NO, CustomFrameRate=NO, ImageUpdatePending=NO, ImageNeedsUpdate=NO ;
double         MovieTime, StageTime, StageTimeNext, StageTimeSearchlights ;
bool           DisclaimerAgreed=NO, AppIsObserver=NO ;
int4           RenderStage, FilmCoRenderMode ;
bool           ChangesSaved   =YES ;   //  Used to decide whether to issue a “save your changes?” warning on Quit or Open.
bool           ContentChanged =YES ;
int4           WhichPreview=NONE   ;
unsigned int4  RenderFrame ;

//  Stored data.
double     OriginalCrawlToAllTextVisibleTime ;   //  Captured once by handleDisclaimerAgreed.
NSString  *UserContentFullPath=nil ;
double     SequenceTotalTime ;   //  Set by verifySequenceTimes().

//  Image bitmap planes.
BYTE  *Planes[5], *BackdropPlanes[5], *TowerPlanes[5], *TowerMaskPlanes[5], *SearchlightMaskPlanes[5] ;

//  Film Company buffers for different stages of Film Company logo color and appearance transformations.
BYTE
*FilmCoBufferGreen =nil ,
*FilmCoBufferBlue  =nil ,
*FilmCoBufferOrange=nil ,
*FilmCoBufferYellow=nil ,
*CrawlBufferStars  =nil ,
*DistributorBuffer =nil ,
*IntroBuffer       =nil ;



//  Custom starfield image.
int4               CustomStarfieldWid, CustomStarfieldHei, CustomStarfieldRowBytes ;
BYTE              *CustomStarfieldBitmapPlanes[5] ;
NSBitmapImageRep  *CustomStarfieldBitmap=nil      ;



@implementation StarWarsTSGController



//  Used to enable or disable the user controls in the control window.

- (void) enableControls:(bool) enable {
  [       DistributorField setEditable:enable                   ];
  [          DistInfoField setEditable:enable                   ];
  [            FilmCoField setEditable:enable                   ];
  [         FilmCoIncField setEditable:enable                   ];
  [             IntroField setEditable:enable                   ];
  [             TitleField setEditable:enable                   ];
  [             CrawlField setEditable:enable                   ];
  [             WidthField setEditable:enable && CustomImageSize];
  [            HeightField setEditable:enable && CustomImageSize];
  [        HeightUsedField setEditable:enable && CustomImageSize];
  [         FrameRateField setEditable:enable && CustomFrameRate];
  [       DestinationField setEditable:enable                   ];
  [         StarfieldField setEditable:enable && !AutoStarfield ];
  [  StarfieldAutoCheckbox setEnabled :enable                   ];
  [  StarfieldChooseButton setEnabled :enable && !AutoStarfield ];
  [DestinationChooseButton setEnabled :enable                   ];
  [ ScreenSizeRadioButtons setEnabled :enable                   ];
  [  FrameRateRadioButtons setEnabled :enable                   ]; }



//  Stop the full-sequence render.
//
//  Call this function after the user has already answered an “Are you sure?” dialog, in the affirmative.

- (void) stopRender {

  [self killRenderingThreads]; WhichPreview=NONE;

  //  Enable/change the controls and the menu options.
  [GoButton setTitle      :@"Go"];
  [self     enableControls:  YES];

  //  Stop the progress bar.
  [ProgressBar setHidden:YES];
  [ColorBar1   setHidden:YES];
  [ColorBar2   setHidden:YES];

  //  Idle the status.
  [StatusText setStringValue:@"Idle"];

  //  Release the offscreen image buffers.
  releaseFilmCoBuffers(); }



//  Launches a new set of render-threads.
//
//  Important:  Nothing is said in the OS documentation about the possibility of detachNewThreadSelector
//              failing to spawn a thread -- if that ever happens, this app will lock-up when the method
//              killRenderingThreads waits forever for a nonexistent thread to finish.

- (void) launchRenderingThreads:(int4) subPixels {

  double  charHei, aspectAdjuster ;
  int4    i, cores=MPProcessors() ;

  if (!UserContentChanged_EverCalled) return;   //  (see this global var’s definition)

  //  Force a certain number of rendering threads, if desired.
  if (RENDERING_THREAD_COUNT) cores=RENDERING_THREAD_COUNT;

  //  Don’t launch anything until the disclaimer is agreed.  (That agreement
  //  also indicates that various data structures have been set up.)
  if (!DisclaimerAgreed) return;

  //  Error checks.
  if (cores<=0     ) [self failApp:                          "MPProcessors returned a non-positive value."];
  if (RenderThreads) [self failApp:"launchRenderingThreads called with rendering threads already running."];

  //  Store the sampling state (lo or hi), for use by the “draw..Pixel” functions.
  RasterSubPixels=subPixels;
  if (RasterSubPixels==OVERSCAN_LO) RasterLoHi=0;
  else                              RasterLoHi=1;

  //  Set the subpixel resolution for the Title.  (This prevents ugly flickering when the Title is in the far distance.)
  if (RasterSubPixels<OVERSCAN_HI || TitleDist<=0.) {
    RasterSubPixels_Title=RasterSubPixels; }
  else {
    charHei=(TitleChar[0].rect.T-TitleChar[0].rect.B)/MOVIE_SCREEN_HEI*ImageHeiUsed/TitleDist;
    aspectAdjuster=(MOVIE_SCREEN_WID/MOVIE_SCREEN_HEI)/((double) ImageWid/(double) ImageHeiUsed); if (aspectAdjuster>1.) aspectAdjuster=1.;
    RasterSubPixels_Title=(int4) ((double) TITLE_CHAR_OVERSCAN_RES/(charHei*aspectAdjuster)+.5);
    if (RasterSubPixels_Title<OVERSCAN_HI ) RasterSubPixels_Title=OVERSCAN_HI ;
    if (RasterSubPixels_Title>OVERSCAN_MAX) RasterSubPixels_Title=OVERSCAN_MAX; }

  //  Reset the variable that tracks which rows of pixels have/haven’t been taken by a rendering thread.
  RasterNextAvailRow=0;

  //  Launch the rendering threads.
  KillRenderThreads=NO; LastRenderImageUpdateNeeded=YES;
  for (i=0; i<cores; i++) {
    [RenderLock   lock]; RenderThreads++;
    [RenderLock unlock]; [NSThread detachNewThreadSelector:@selector(renderingThread:) toTarget:self withObject:nil]; }

  //  Start periodic updating of the preview image.
  [self startImageUpdating]; }



//  Terminates all running render-threads.
//
//  Note:  The NSBeep() statement is never intended to execute -- it is necessary for the Deployment build, because
//         when in Deployment mode the compiler is too “smart” to allow the following statement to compile correctly:
//         while(RenderThreads);

- (void) killRenderingThreads {
  KillRenderThreads=YES; while (RenderThreads) if (!KillRenderThreads) NSBeep(); }



//  This method should not be called directly by code, but instead should
//  be spawned as a thread.  (That is done by launchRenderingThreads.)
//
//  dummyParameter is not used -- its presence is required by the OS.
//
//  See the file “Raster Segments” for an explanation of how the Raster array works.

- (void) renderingThread:(id) dummyParameter {

  int4  i, j, k, pixelX=0, pixelY=0 ;
  bool  endThread=NO ;

  //  Variables used for accellerated (whole-row) image rendering.  See the webpage at:  http://alienryderflex.com/polygon_fill
  bool    inPoly     [OVERSCAN_MAX], nodeRowsReady ;
  bool    inInnerPoly[OVERSCAN_MAX]                ;
  double  nodeX      [OVERSCAN_MAX*NODE_ROW_MAX]   ;   //  (nodeX and polyTag are one-dimensional so that they can be
  int4    polyTag    [OVERSCAN_MAX*NODE_ROW_MAX]   ;   //   passed into functions; otherwise bad C headaches occur.)
  int4    nodes      [OVERSCAN_MAX]                ;
  int4    nodeNext   [OVERSCAN_MAX]                ;

  //  To prevent cache conflicts and lock waits, we want each rendering thread to be spending most of its
  //  time working with its own, local variables.  This set of variables serves that purpose by allowing
  //  the thread to pull a significant amount of data about which pixels need to be rendered, and while
  //  working on those pixels to store the rendered RGB values locally until the local buffers fill up.
  RenderRow_  renderRow[PIXEL_ROW_BUFFERS] ;   //  (see definition of RenderRow_)
  int4        renderRows          =0       ;   //  how many of the renderRow[] items have data
  int4        rasterLoHi=RasterLoHi        ;   //  0 or 1 -- local copy of RasterLoHi
  int4        pixelBufferX        =0       ;   //  X-coordinate of next pixel to render
  int4        pixelBuffer         =0       ;   //  which renderRow item is currently in use
  int4        pixelBufferRasterSeg=0       ;   //  which .raster segment is currently in use

  bool  pixelFound ;

  //  Variables used for avoiding repetitive decoding of compressed-bitmap-
  //  font glyph row data.  (“cbf” stands for “compressed bitmap font”.)
  int4   cbfChar[OVERSCAN_HI*OVERSCAN_HI] ;   //0-255, or -1 at thread outset
  BYTE   cbfFill[OVERSCAN_HI*OVERSCAN_HI] ;
  short  cbfX   [OVERSCAN_HI*OVERSCAN_HI] ;
  short  cbfY   [OVERSCAN_HI*OVERSCAN_HI] ;
  bool   cbfRaw [OVERSCAN_HI*OVERSCAN_HI] ;
  BYTE  *cbfData[OVERSCAN_HI*OVERSCAN_HI] ;
  for    (i=0; i<OVERSCAN_HI*OVERSCAN_HI; i++) cbfChar[i]=-1;   //  (forces these variables to be repopulated when thread is new)

  //  Create an autorelease pool to ensure against memory leaks.
  NSAutoreleasePool  *pool=[NSAutoreleasePool new] ;

  //  Main loop.  Each rendering thread keeps rendering pixels until it runs
  //  out of pixels to render, or until KillRenderThreads tells it to terminate.
  while (!KillRenderThreads && !endThread) {

    //  Find a pixel to render, or a row/column to blur -- or end this thread if there aren’t any.
    if   (      RenderStage==FILMCO_FADE_UP
    &&    (FilmCoRenderMode==BLUR_A_H
    ||     FilmCoRenderMode==BLUR_B_H
    ||     FilmCoRenderMode==BLUR_C_H
    ||     FilmCoRenderMode==BLUR_A_V
    ||     FilmCoRenderMode==BLUR_B_V
    ||     FilmCoRenderMode==BLUR_C_V)) {   //  (find a row or column to blur)
      if ( FilmCoRenderMode==BLUR_A_H
      ||   FilmCoRenderMode==BLUR_B_H
      ||   FilmCoRenderMode==BLUR_C_H ) {   //  (find a row to blur)
        pixelX=0        ; [RenderLock   lock];
        pixelY=BlurRow++; [RenderLock unlock];
        if (pixelY>=ImageHei) endThread=YES;
        else {
          [BitmapLock lock]; k=PreviewRowBytes*pixelY;
          for (i=0; i<ImageWid; i++) {
            renderRow[0].r[i]=PreviewRgb[0][k  ];
            renderRow[0].g[i]=PreviewRgb[1][k  ];
            renderRow[0].b[i]=PreviewRgb[2][k++]; }
          [BitmapLock unlock]; }}
      else {                                //  (find a column to blur)
        pixelY=0        ; [RenderLock   lock];
        pixelX=BlurCol++; [RenderLock unlock];
        if (pixelX>=ImageWid) endThread=YES;
        else {
          [BitmapLock   lock]; k=pixelX;
          for (j=0; j<ImageHei; j++) {
            renderRow[0].r[j]=PreviewRgb[0][k];
            renderRow[0].g[j]=PreviewRgb[1][k];
            renderRow[0].b[j]=PreviewRgb[2][k]; k+=PreviewRowBytes; }
          [BitmapLock unlock]; }}}
    else {                                  //  (find a single pixel to render)
      pixelFound=NO;
      while (!pixelFound && !endThread) {
        if (!renderRows) {   //  (grab some data from the Raster array, to be rendered by this thread)
          [RenderLock lock]; if (RenderStage==FILMCO_FADE_UP) [BitmapLock lock];
          while (RasterNextAvailRow<ImageHei && renderRows<PIXEL_ROW_BUFFERS) {
            renderRow[renderRows].raster=Raster[RasterNextAvailRow  ];
            renderRow[renderRows].y     =       RasterNextAvailRow++ ;
            if (RenderStage==FILMCO_FADE_UP) {   //  (Film Co is built additively, so we must acquire the current bitmap contents)
              k=PreviewRowBytes*renderRow[renderRows].y;
              for (i=0; i<ImageWid; i++) {
                renderRow[renderRows].r[i]=PreviewRgb[0][k  ];
                renderRow[renderRows].g[i]=PreviewRgb[1][k  ];
                renderRow[renderRows].b[i]=PreviewRgb[2][k++]; }}
            renderRows++; }
          [RenderLock unlock]; if (RenderStage==FILMCO_FADE_UP) [BitmapLock unlock];
          nodeRowsReady=NO; pixelBuffer=0; pixelBufferRasterSeg=0; pixelBufferX=renderRow[0].raster.left[rasterLoHi][0];
          for (i=renderRows; i<PIXEL_ROW_BUFFERS; i++) {   //  (neutralize unused buffers)
            renderRow[i].y=0;
            for (j=0; j<MAX_RENDER_SEGS_PER_ROW; j++) {
              renderRow[i].raster.left [rasterLoHi][j]=0;
              renderRow[i].raster.right[rasterLoHi][j]=0; }}
          if (!renderRows) endThread=YES; }
        else {
          if (pixelBufferX>=renderRow[pixelBuffer].raster.right[rasterLoHi][pixelBufferRasterSeg]) {   //  (end of seg)
            if (++pixelBufferRasterSeg>=MAX_RENDER_SEGS_PER_ROW) {   //  (end of row)
              if (++pixelBuffer>=renderRows) {
                [BitmapLock   lock]; if (emptyRenderRowBuffers(&renderRow[0],pixelBuffer,0,0,rasterLoHi)) ImageNeedsUpdate=YES;
                [BitmapLock unlock];
                [RenderLock   lock]; for (i=0; i<renderRows; i++) Raster[renderRow[i].y]=renderRow[i].raster;
                [RenderLock unlock]; renderRows=0; }
              else {
                pixelBufferRasterSeg=0; nodeRowsReady=NO;
                pixelBufferX=renderRow[pixelBuffer].raster.left[rasterLoHi][pixelBufferRasterSeg]; }}
            else {
              pixelBufferX  =renderRow[pixelBuffer].raster.left[rasterLoHi][pixelBufferRasterSeg]; }}
          else {
            pixelFound=YES; pixelX=pixelBufferX++; pixelY=renderRow[pixelBuffer].y; }}}}

    if (!endThread) {

      //  Render the pixel.
      if      (WhichPreview==DISTRIBUTOR_LOGO) {
        drawDistributorPixel  (&renderRow[pixelBuffer].r[pixelX],&renderRow[pixelBuffer].g[pixelX],&renderRow[pixelBuffer].b[pixelX],pixelX,renderRow[pixelBuffer].y,&cbfChar[0],&cbfFill[0],&cbfX[0],&cbfY[0],&cbfRaw[0],&cbfData[0],nil,NO); }
      else if (WhichPreview==     FILMCO_LOGO) {
        drawFilmCoPreviewPixel(&renderRow[pixelBuffer].r[pixelX],&renderRow[pixelBuffer].g[pixelX],&renderRow[pixelBuffer].b[pixelX],pixelX,renderRow[pixelBuffer].y,&nodeRowsReady,&nodes[0],&nodeX[0],&polyTag[0],&nodeNext[0],&inPoly[0] ); }
      else if (WhichPreview==      INTRO_TEXT) {
        drawIntroPixel        (&renderRow[pixelBuffer].r[pixelX],&renderRow[pixelBuffer].g[pixelX],&renderRow[pixelBuffer].b[pixelX],pixelX,renderRow[pixelBuffer].y,&cbfChar[0],&cbfFill[0],&cbfX[0],&cbfY[0],&cbfRaw[0],&cbfData[0]       ); }
      else if (WhichPreview==      TITLE_LOGO) {
        drawCrawlPixel        (&renderRow[pixelBuffer].r[pixelX],&renderRow[pixelBuffer].g[pixelX],&renderRow[pixelBuffer].b[pixelX],pixelX,renderRow[pixelBuffer].y,&cbfChar[0],&cbfFill[0],&cbfX[0],&cbfY[0],&cbfRaw[0],&cbfData[0],nil,NO,
        &nodeRowsReady,&nodes[0],&nodeX[0],&polyTag[0],&nodeNext[0],&inPoly[0],&inInnerPoly[0],
        CustomStarfieldBitmapPlanes[0],CustomStarfieldBitmapPlanes[1],CustomStarfieldBitmapPlanes[2],CustomStarfieldWid,CustomStarfieldHei,CustomStarfieldRowBytes); }
      else if (WhichPreview==      TEXT_CRAWL) {
        drawCrawlPixel        (&renderRow[pixelBuffer].r[pixelX],&renderRow[pixelBuffer].g[pixelX],&renderRow[pixelBuffer].b[pixelX],pixelX,renderRow[pixelBuffer].y,&cbfChar[0],&cbfFill[0],&cbfX[0],&cbfY[0],&cbfRaw[0],&cbfData[0],nil,NO,
        &nodeRowsReady,&nodes[0],&nodeX[0],&polyTag[0],&nodeNext[0],&inPoly[0],&inInnerPoly[0],
        CustomStarfieldBitmapPlanes[0],CustomStarfieldBitmapPlanes[1],CustomStarfieldBitmapPlanes[2],CustomStarfieldWid,CustomStarfieldHei,CustomStarfieldRowBytes); }
      else if (WhichPreview==      GENERATING) {
        if      (RenderStage==DISTRIBUTOR_FADE_UP
        ||       RenderStage==DISTRIBUTOR_HOLD
        ||       RenderStage==DISTRIBUTOR_FADE_DOWN) {
          drawDistributorPixel(&renderRow[pixelBuffer].r[pixelX],&renderRow[pixelBuffer].g[pixelX],&renderRow[pixelBuffer].b[pixelX],pixelX,renderRow[pixelBuffer].y,&cbfChar[0],&cbfFill[0],&cbfX[0],&cbfY[0],&cbfRaw[0],&cbfData[0],
          DistributorBuffer,YES); }
        else if (RenderStage==FILMCO_FADE_UP) {
          if      (FilmCoRenderMode==SILHOUETTE_A
          ||       FilmCoRenderMode==SILHOUETTE_B
          ||       FilmCoRenderMode==SILHOUETTE_C
          ||       FilmCoRenderMode==FRILL_BORDERS
          ||       FilmCoRenderMode==BORDERED_OVERLAY
          ||       FilmCoRenderMode==BLUE_CORE
          ||       FilmCoRenderMode==ORANGE_TINT
          ||       FilmCoRenderMode==YELLOW_TINT) {
            drawFilmCoPixel(&renderRow[pixelBuffer].r[pixelX],&renderRow[pixelBuffer].g[pixelX],&renderRow[pixelBuffer].b[pixelX],pixelX,renderRow[pixelBuffer].y,FilmCoRenderMode,RenderStage,nil,nil,nil,nil,
            &nodeRowsReady,&nodes[0],&nodeX[0],&polyTag[0],&nodeNext[0],&inPoly[0]); }
          else if (FilmCoRenderMode==BLUR_A_H
          ||       FilmCoRenderMode==BLUR_B_H
          ||       FilmCoRenderMode==BLUR_C_H) {
            blurFilmCoPixels(&renderRow[0].r[0],&renderRow[0].g[0],&renderRow[0].b[0],FilmCoRenderMode,FilmCoBellRadH);
            [BitmapLock lock]; k=PreviewRowBytes*pixelY;
            for (i=0; i<ImageWid; i++) {   //  (copy blurred row of pixels back to bitmap)
              PreviewRgb[0][k  ]=renderRow[0].r[i];
              PreviewRgb[1][k  ]=renderRow[0].g[i];
              PreviewRgb[2][k++]=renderRow[0].b[i]; }
            [BitmapLock unlock]; ImageNeedsUpdate=YES; }
          else if (FilmCoRenderMode==BLUR_A_V
          ||       FilmCoRenderMode==BLUR_B_V
          ||       FilmCoRenderMode==BLUR_C_V) {
            blurFilmCoPixels(&renderRow[0].r[0],&renderRow[0].g[0],&renderRow[0].b[0],FilmCoRenderMode,FilmCoBellRadV);
            [BitmapLock lock];
            k=pixelX;
            for (j=0; j<ImageHei; j++) {   //  (copy blurred column of pixels back to bitmap)
              PreviewRgb[0][k]=renderRow[0].r[j];
              PreviewRgb[1][k]=renderRow[0].g[j];
              PreviewRgb[2][k]=renderRow[0].b[j]; k+=PreviewRowBytes; }
            [BitmapLock unlock]; ImageNeedsUpdate=YES; }
          else if (FilmCoRenderMode==BUFFERS_READY) {
            drawFilmCoPixel(&renderRow[pixelBuffer].r[pixelX],&renderRow[pixelBuffer].g[pixelX],&renderRow[pixelBuffer].b[pixelX],pixelX,renderRow[pixelBuffer].y,FilmCoRenderMode,RenderStage,
            FilmCoBufferGreen,FilmCoBufferBlue,FilmCoBufferOrange,FilmCoBufferYellow,
            &nodeRowsReady,&nodes[0],&nodeX[0],&polyTag[0],&nodeNext[0],&inPoly[0]); }}
        else if (RenderStage==FILMCO_SPARKLE_GREEN
        ||       RenderStage==FILMCO_SPARKLE_GREEN_TO_BLUE
        ||       RenderStage==FILMCO_SPARKLE_BLUE_TO_ORANGE
        ||       RenderStage==FILMCO_ORANGE_TO_YELLOW
        ||       RenderStage==FILMCO_FADE_DOWN) {
          drawFilmCoPixel(&renderRow[pixelBuffer].r[pixelX],&renderRow[pixelBuffer].g[pixelX],&renderRow[pixelBuffer].b[pixelX],pixelX,renderRow[pixelBuffer].y,FilmCoRenderMode,RenderStage,
          FilmCoBufferGreen,FilmCoBufferBlue,FilmCoBufferOrange,FilmCoBufferYellow,&nodeRowsReady,&nodes[0],&nodeX[0],&polyTag[0],&nodeNext[0],&inPoly[0]); }
        else if (RenderStage==INTRO_FADE_UP && !IntroImageReady) {
          drawIntroPixel(&renderRow[pixelBuffer].r[pixelX],&renderRow[pixelBuffer].g[pixelX],&renderRow[pixelBuffer].b[pixelX],pixelX,renderRow[pixelBuffer].y,&cbfChar[0],&cbfFill[0],&cbfX[0],&cbfY[0],&cbfRaw[0],&cbfData[0]); }
        else if (RenderStage==TITLE_PULL_BACK
        ||       RenderStage==CRAWL_AND_TITLE_PULL_BACK
        ||       RenderStage==CRAWL_AND_TITLE_FADE_DOWN
        ||       RenderStage==CRAWL_TO_ALL_TEXT_VISIBLE
        ||       RenderStage==CRAWL_OFF_TO_DISTANCE
        ||       RenderStage==CRAWL_FADE_DOWN
        ||       RenderStage==JUST_STARS) {
          drawCrawlPixel(&renderRow[pixelBuffer].r[pixelX],&renderRow[pixelBuffer].g[pixelX],&renderRow[pixelBuffer].b[pixelX],pixelX,renderRow[pixelBuffer].y,&cbfChar[0],&cbfFill[0],&cbfX[0],&cbfY[0],&cbfRaw[0],&cbfData[0],
          CrawlBufferStars,YES,&nodeRowsReady,&nodes[0],&nodeX[0],&polyTag[0],&nodeNext[0],&inPoly[0],&inInnerPoly[0],
          CustomStarfieldBitmapPlanes[0],CustomStarfieldBitmapPlanes[1],CustomStarfieldBitmapPlanes[2],CustomStarfieldWid,CustomStarfieldHei,CustomStarfieldRowBytes); }}}}

  if (renderRows) {

    //  Copy the pixel buffers to the bitmap.
    [BitmapLock   lock]; if (emptyRenderRowBuffers(&renderRow[0],pixelBuffer,pixelBufferRasterSeg,pixelX,rasterLoHi)) ImageNeedsUpdate=YES;
    [BitmapLock unlock];

    //  Update the Raster array.
    [RenderLock   lock]; for (i=0; i<renderRows; i++) Raster[renderRow[i].y]=renderRow[i].raster;
    [RenderLock unlock]; }

  //  Decrement the thread count and end the thread.
  [RenderLock   lock]; RenderThreads--;
  [RenderLock unlock];

  //  Kill the autorelease pool.
  [pool release]; }



//  This method is called when the user attempts to stop an ongoing full-sequence (not preview) render, either by
//  clicking the “Stop” button or by trying to Quit the app (which includes trying to restart or shutdown the
//  computer, in which case the OS will invoke the Quit attempt).
//
//  Returns YES if rendering was stopped, or NO if the user said to continue it.
//
//  Note:  Don’t call this method if full-sequence rendering is not running.

- (bool) abortRender {

  if ([self ask:"Are you sure you want to stop this render?  You will not be able to continue it where you left off."
  no:"Continue Render" yes:"Stop Render"]) {
    [self stopRender]; return YES; }

  return NO; }



//  This method is called when the user clicks the “Go” button (or
//  “Stop” button, as it appears during a full-sequence render).

- (IBAction) handleGoStopButton:(id) sender {

  int4    i, imageBytes=3L*ImageWid*ImageHei ;
  double  totalBarWid, bar1Wid, bar2Wid, bar1Frac, crawlHeightTime, newX ;
  double  crawlSize ;   //  (1.0 if the Crawl is no lengthier than the original movie crawl (plus “Episode X”), or more than 1.0 if lengthier than that)
  NSRect  barRect   ;

  [self killRenderingThreads];

  if (WhichPreview!=GENERATING) {   //  START RENDER

    //  Make absolutely sure the sequence cannot be generated if the disclaimer was not agreed-to.
    //  (In theory, this should not be needed, because the “Go” button should be accessible only
    //  after the disclaimer is agreed-to -- but just to be safe, here it is anyway.)
    if (!DisclaimerAgreed) return;

    //  Disallow the user from trying to save hundreds of image files on the desktop.
    if (![self desktopWarning]) return;

    //  Load the custom starfield image, if any.
    if (!AutoStarfield) if (![self loadCustomStarfield]) return;

    WhichPreview=GENERATING;

    //  Disable/change the controls and menu options.
    [GoButton setTitle      :@"Stop"];
    [self     enableControls:     NO];

    //  Set the render-control variables.
    RenderStage=0; RenderFrame=0; MovieTime=0.; StageTime=0.; StageTimeNext=getSequenceTime(RenderStage);
    PreviewRgb     =&Planes[0];
    PreviewRowBytes=(int4) [PreviewBitmap bytesPerRow];
    nullRaster(YES,YES); fillImage(0,0,0); ImageNeedsUpdate=YES;

    //  Adjust the time allowed for the crawl, to accommodate any length of user-entered text.
    [self canFailApp:prepareToDrawCrawlImage(&Planes[0],(int4) [PreviewBitmap bytesPerRow])];
    crawlSize=calcCrawlBottom()/ORIGINAL_MOVIE_CRAWL_BOTTOM; if (crawlSize<1.) crawlSize=1.;
    crawlHeightTime=calcCrawlHeightTime(OriginalCrawlToAllTextVisibleTime);
    crawlHeightTime*=crawlSize;
    setSequenceTime(CRAWL_TO_ALL_TEXT_VISIBLE,crawlHeightTime-
    getSequenceTime(CRAWL_AND_TITLE_PULL_BACK)               -
    getSequenceTime(CRAWL_AND_TITLE_FADE_DOWN)                );
    [self canFailApp:verifySequenceTimes(&SequenceTotalTime)];

    //  Adjust the proportions of the two color-bar pieces, so they
    //  correctly reflect a possibly lengthy user-entered crawl.
    totalBarWid=[ColorBar1 frame].size.width
    +           [ColorBar2 frame].size.width;
    bar1Frac=0.; for (i=0; i<CRAWL_AND_TITLE_PULL_BACK; i++) bar1Frac+=getSequenceTime(i)/SequenceTotalTime;
    bar1Wid=(int4) (totalBarWid*bar1Frac+.5); bar2Wid=totalBarWid-bar1Wid;
    barRect=[ColorBar1 frame]; barRect.size.width=bar1Wid; newX=barRect.origin.x+bar1Wid; [ColorBar1 setFrame:barRect];
    barRect=[ColorBar2 frame]; barRect.size.width=bar2Wid;      barRect.origin.x=newX   ; [ColorBar2 setFrame:barRect];

    //  Start the progress bar.
    [ProgressBar setDoubleValue:.0001];
    [ProgressBar setHidden     :   NO];
    [ColorBar1   setHidden     :   NO];
    [ColorBar2   setHidden     :   NO];

    //  Initialize the status.
    showStatusFrame(StatusText,RenderFrame);

    //  Allocate extra image buffers for use when rendering the Film Company logo.
    FilmCoBufferGreen =malloc(imageBytes);  CrawlBufferStars=FilmCoBufferGreen;       StarsImageReady=NO;
    FilmCoBufferBlue  =malloc(imageBytes); DistributorBuffer=FilmCoBufferGreen; DistributorImageReady=NO;
    FilmCoBufferOrange=malloc(imageBytes);       IntroBuffer=FilmCoBufferGreen;       IntroImageReady=NO;
    FilmCoBufferYellow=malloc(imageBytes);
    if (!FilmCoBufferGreen
    ||  !FilmCoBufferBlue
    ||  !FilmCoBufferOrange
    ||  !FilmCoBufferYellow) {
      [self alert:"Unable to allocate space for needed image buffers.  Rendering cannot proceed."];
      [self stopRender]; return; }

    [self launchRenderingThreads:OVERSCAN_HI]; }

  else {   //  STOP RENDER

    [self abortRender]; }}



#if COMPRESS_FONTS

//  Developer-only method to compress bitmap font images.  (This does
//  not use multiprocessing -- it’s for one-shot use by the developer.)
//
//  (See comment at definition of COMPRESS_FONTS.)

- (void) compressBitmapFont:(BYTE *) fontFile fontInfo:(int4 *) fontInfo fontInfoSize:(int4) fontInfoSize {

  #define  FONT_DATA_MAX_SPACE  10485760L   //  (10 megabytes) (must not be larger than the max value of an unsigned int4)

  int4           i, j, c, rowsI, row, x, bmpX, bmpY, imageRowBytes, glyph, glyphs, glyphWid, glyphHei, highestUnicodeVal=0 ;
  BYTE          *font, *rowData, fill, count, *imageStart, *imageDataBytes, fileName[200] ;
  NSData        *imageData, *compressedData ;
  short          fileNameLen ;
  NSString      *filePath    ;
  NSFileHandle  *theFile     ;

  //  Acquire the raw bitmap font image.
  //  For BMP format details used here, see:  http://fortunecity.com/skyscraper/windows/364/bmpffrmt.html
  imageData=[NSData dataWithContentsOfFile:[[NSString stringWithUTF8String:fontFile] stringByExpandingTildeInPath]];
  if (!imageData) [self failApp:"Bitmap image file not found."];
  imageDataBytes=(BYTE *) [imageData bytes];
  imageStart    =imageDataBytes+(int4) getULongLittleEndian(&imageDataBytes[10])    ;
  imageRowBytes =      forceDiv((int4) getULongLittleEndian(&imageDataBytes[18]),4L);

  //  Allocate a comfortably large space for the compressed font.
  font=malloc(FONT_DATA_MAX_SPACE); if (!font) [self failApp:"Unable to allocate memory for font compression."];

  //  Determine the highest Unicode character value used.
  glyphs=(fontInfoSize/(int4) sizeof(fontInfo[0])-3L)/4L;
  for (glyph=0; glyph<glyphs; glyph++) {
    c=fontInfo[3L+4L*glyph]; convertMacCharToUnicode(&c); if (c>highestUnicodeVal) highestUnicodeVal=c; }
  if (highestUnicodeVal>=UTF8_CHARS) [self failApp:"Invalid cbf character value."];

  //  Perform the compression.  (See the file “Compressed Bitmap Font Definition”.)
  glyphHei=(int4) fontInfo[2];
  putULongLittleEndian(&font[0],glyphHei            );
  putULongLittleEndian(&font[4],highestUnicodeVal+1L);
  for (i=8L; i<8L+4L*(highestUnicodeVal+1L); i++) font[i]=(BYTE) 0;
  //  Loop through the glyphs, writing each one to the in-RAM file structure.
  for (glyph=0L; glyph<glyphs; glyph++) {
    //  Get the glyph’s character value.
    c=fontInfo[3L+4L*glyph];
    //  Correct non-Unicode character value.
    convertMacCharToUnicode(&c);
    //  Write the glyph’s data offset.
    if (font[8L+4L*c] || font[9L+4L*c] || font[10L+4L*c] || font[11L+4L*c]) [self failApp:(BYTE *) [[NSString stringWithFormat:@"%s - Duplicate glyph:  %d",fontFile,c] UTF8String]];
    putULongLittleEndian(&font[8L+4L*c],i);
    //  Write the glyph’s width.
    if      (i+ 4L         >FONT_DATA_MAX_SPACE) [self failApp:"Used up RAM allocation during font compression.  (a)"];
    glyphWid=fontInfo[3L+4L*glyph+3L]; putULongLittleEndian(&font[i],glyphWid); i+=4L;
    //  Remember the location of the glyph’s row-offset list.
    if      (i+ 4L*glyphHei>FONT_DATA_MAX_SPACE) [self failApp:"Used up RAM allocation during font compression.  (b)"];
    rowsI=i; i+=4L*glyphHei;
    //  Loop through the rows of the glyph.
    for (row=0L; row<glyphHei; row++) {
      //  Write the row’s data offset.
      putULongLittleEndian(&font[rowsI+4L*row],i);
      //  Prepare to compress one row.
      bmpX=fontInfo[3L+4L*glyph+2L]; x=0L; fill=(BYTE) 0;
      bmpY=fontInfo[3L+4L*glyph+1L]*glyphHei+row;
      rowData=imageStart+imageRowBytes*bmpY+bmpX;
      //  Write the chunk(s) that make up the row.
      while (x<glyphWid) {
        //  Count the number of fill bytes.
        count=(BYTE) 0;
        while (x<glyphWid && rowData[x]==fill && count<(BYTE) 255) {
          count++; x++; }
        //  Write the fill count.
        if     (i+1L>FONT_DATA_MAX_SPACE) [self failApp:"Used up RAM allocation during font compression.  (c)"];
        font[i++]=count;
        //  Decide whether this chunk will have any more data.
        if (count<(BYTE) 255) {
          //  Count the number of raw bytes, writing their values in while counting.
          if   (i+1L>FONT_DATA_MAX_SPACE) [self failApp:"Used up RAM allocation during font compression.  (d)"];
          j=i+1L; count=(BYTE) 0;
          while (x<glyphWid && rowData[x] && rowData[x]!=(BYTE) 255 && count<(BYTE) 255) {
            if (j+1L>FONT_DATA_MAX_SPACE) [self failApp:"Used up RAM allocation during font compression.  (e)"];
            count++; font[j++]=rowData[x++]; }
          //  Write the raw count.
          font[i]=count; i=j;
          //  Determine the new fill color (black or white).
          if (count) {
            if (font[i-1L]<(BYTE) 128) fill=(BYTE)   0;
            else                       fill=(BYTE) 255; }
          else {
            fill=(BYTE) 255-fill; }}}}}

  //  Get an NSData object for the compressed font data.
  compressedData=[NSData dataWithBytesNoCopy:font length:i freeWhenDone:NO];
  if (!compressedData) [self failApp:"Unable to allocate NSData object when writing compressed-font file."];

  //  Construct the file name, simply by changing “.bmp” to “.dat”.
  copyText(&fileName[0],fontFile,sizeof(fileName)/sizeof(fileName[0])-1);
  fileNameLen=0; while (fileName[fileNameLen]) fileNameLen++;
  if (!strEqual(&fileName[fileNameLen-4],".bmp"  )) [self failApp:"Font filename does not end in “.bmp”."];
  copyText(&fileName[fileNameLen-4],".dat",4);

  //  Create the (empty) file.
  filePath=[[NSString stringWithUTF8String:&fileName[0]] stringByExpandingTildeInPath];
  if (![[NSFileManager defaultManager] createFileAtPath:filePath contents:nil attributes:nil]) {
    [self failApp:"Unable to create file in which to write compressed-font data."]; }

  //  Open the file for writing.
  theFile=[NSFileHandle fileHandleForWritingAtPath:filePath];
  if (!theFile) [self failApp:"Unable to open file for writing of compressed-font data."];

  //  Write the compressed-font data.
  [theFile writeData:[NSData dataWithBytes:&font[0] length:i]];

  //  Close the file.
  [theFile closeFile];

  //  Release the memory block that held the compressed-font data.
  free(font); }

#endif



//  This function is invoked when the user agrees to the opening disclaimer.  It performs
//  all once-only app initializations.  (This is used instead of awakeFromNib.)

- (IBAction) handleDisclaimerAgreed:(id) sender {

  int4  i, j, text[TEXT_FIELD_MAX+1] ;

  //  Verify size of data types.
  [self canFailApp:verifyDataTypeSizes()];

  /*
  //  Diagnostic to discover the extended attributes of a particular file.
  BYTE  *path   ="/Users/softwaredevelopment/_test_macroman.txt";
  int4   bufSize=listxattr(path,  NULL,      0,XATTR_NOFOLLOW) ;
  BYTE  *buffer =malloc(bufSize+1L) ;
  listxattr               (path,buffer,bufSize,XATTR_NOFOLLOW) ;
  buffer[bufSize]=0; for (i=0; i<bufSize; i++) if (!buffer[i]) buffer[i]='~';
  bufSize++;//dummy instruction; put a diagnostic stop here
  */

  /*
  //  Diagnostic to discover the value of a particular extended attribute of a particular file.
  BYTE  *path ="/Users/softwaredevelopment/_test_utf8.txt", buffer[1001] ;
  int4   count=getxattr(path,"com.apple.TextEncoding",&buffer[0],1000,0,XATTR_NOFOLLOW) ;
  buffer[count]=0; [self failApp:&buffer[0]];
  */

  //  Compute crawl speed.
  CrawlSpeed=-ORIGINAL_MOVIE_CRAWL_BOTTOM/getSequenceTime(CRAWL_TO_ALL_TEXT_VISIBLE);

  //  If requested via the COMPRESS_FONTS constant, perform bitmap font compression and exit.  (This
  //  exists for development purposes only -- see the file “Bitmap Font Compression” for more information.)
  #if COMPRESS_FONTS
  [self compressBitmapFont:"~/Font (DistInfo).bmp" fontInfo:&fontCompressionInfo_DistInfo[0] fontInfoSize:sizeof(fontCompressionInfo_DistInfo)];
  [self compressBitmapFont:"~/Font (Intro).bmp"    fontInfo:&fontCompressionInfo_Intro   [0] fontInfoSize:sizeof(fontCompressionInfo_Intro   )];
  [self compressBitmapFont:"~/Font (Crawl).bmp"    fontInfo:&fontCompressionInfo_Crawl   [0] fontInfoSize:sizeof(fontCompressionInfo_Crawl   )];
  [self alert:"Bitmap font compression completed; app will now terminate."];
  [NSApp terminate:nil]; return;
  #endif

  //  Dynamically allocate arrays for Film Company sparkle bell curves.
  FilmCoBellH=(double *) malloc(FilmCoBellDimensionH*sizeof(double));
  FilmCoBellV=(double *) malloc(FilmCoBellDimensionV*sizeof(double));
  if (!FilmCoBellH || !FilmCoBellV) [self failApp:"Unable to allocate memory for Film Co sparkle bell curves."];

  //  Create the lock objects.
  RenderLock=[NSLock new];
  BitmapLock=[NSLock new];

  //  Close the disclaimer window.
  [DisclaimerWindow close];

  //  Process the spline-polygon fonts.
  [self canFailApp:processDistributorFont()];
  [self canFailApp:     processFilmCoFont()];
  [self canFailApp:      processTitleFont()];

  //  Fix the Mac Roman characters in the upper/lowercase matchup strings, by converting them to Unicode.
  i=0;
  while                    ( SpecialCaseCharsLower[i  ])  {
    convertMacCharToUnicode(&SpecialCaseCharsLower[i  ]);
    convertMacCharToUnicode(&SpecialCaseCharsUpper[i++]); }

  //  Initialize the Distributor image-rendering process.
  DistributorText[0]=0;
  DistInfoText   [0]=0;
  FilmCoText     [0]=0;
  FilmCoIncText  [0]=0;
  nullRaster           (YES,YES);   //  (must start with this, to ensure raster-segment data is cleared)
  forceRasterWholeImage(YES,YES);

  //  Verify sequence time array integrity.
  [self canFailApp:verifySequenceTimes(&SequenceTotalTime)];

  //  Build arrays for image rendering.
  generateStarField(); constructSparkles(); generateMetalScratches();

  //  Stash original crawl time for later adjustments.
  OriginalCrawlToAllTextVisibleTime=getSequenceTime(CRAWL_TO_ALL_TEXT_VISIBLE);

  //  Build the bitmaps.
  createBitmap               (); createDistributorBackdropImage(self); BYTE *distributorData=(BYTE *) [DistBackdropData bytes] ;
  createBackdropBitmap       (distributorData);
  createTowerBitmap          ();
  createTowerMaskBitmap      (); [self createTowerImages         ];
  createSearchlightMaskBitmap(); [self createSearchlightMaskImage];

  //  Load compressed bitmap fonts.
  [self loadCbfDistInfo];
  [self loadCbfIntro   ];
  [self loadCbfCrawl   ];

  //  Create preview-display image.
  [TheImageView setImage:TheImage];

  //  Get data planes from bitmaps.
  [        PreviewBitmap getBitmapDataPlanes:&               Planes[0]];
  [       BackdropBitmap getBitmapDataPlanes:&       BackdropPlanes[0]];
  [          TowerBitmap getBitmapDataPlanes:&          TowerPlanes[0]];
  [      TowerMaskBitmap getBitmapDataPlanes:&      TowerMaskPlanes[0]];
  [SearchlightMaskBitmap getBitmapDataPlanes:&SearchlightMaskPlanes[0]];

  //  Extract image data from Distributor backdrop BMP.
  for   (j=0L; j<DistributorBackdropHei; j++) {
    for (i=0L; i<DistributorBackdropWid; i++) {
      BackdropPlanes[0][DistributorBackdropWid*(DistributorBackdropHei-1L-j)+i]=distributorData[BMP_HEADERSIZE_RGB+(DistributorBackdropWid*j+i)*3L+2L];
      BackdropPlanes[1][DistributorBackdropWid*(DistributorBackdropHei-1L-j)+i]=distributorData[BMP_HEADERSIZE_RGB+(DistributorBackdropWid*j+i)*3L+1L];
      BackdropPlanes[2][DistributorBackdropWid*(DistributorBackdropHei-1L-j)+i]=distributorData[BMP_HEADERSIZE_RGB+(DistributorBackdropWid*j+i)*3L   ]; }}
  [DistBackdropData release]; DistBackdropData=nil;

  //  Extract image data from front-searchlight-tower BMP.
  BYTE  *twrData=(BYTE *) [TowerData bytes] ;
  for   (j=0L; j<TowerBitmapHei; j++) {
    for (i=0L; i<TowerBitmapWid; i++) {
      TowerPlanes[0][(int4) TowerBitmapWid*((int4) TowerBitmapHei-1L-j)+i]=twrData[BMP_HEADERSIZE_RGB+((int4) TowerBitmapWid*j+i)*3L+2L];
      TowerPlanes[1][(int4) TowerBitmapWid*((int4) TowerBitmapHei-1L-j)+i]=twrData[BMP_HEADERSIZE_RGB+((int4) TowerBitmapWid*j+i)*3L+1L];
      TowerPlanes[2][(int4) TowerBitmapWid*((int4) TowerBitmapHei-1L-j)+i]=twrData[BMP_HEADERSIZE_RGB+((int4) TowerBitmapWid*j+i)*3L   ]; }}
  [TowerData release]; TowerData=nil;

  //  Extract image data from front-searchlight-tower mask BMP.
  BYTE  *twrMskData=(BYTE *) [TowerMaskData bytes] ;
  for   (j=0L; j<TowerBitmapHei; j++) {
    for (i=0L; i<TowerBitmapWid; i++) {
      TowerMaskPlanes[0][(int4) TowerBitmapWid*((int4) TowerBitmapHei-1L-j)+i]=twrMskData[BMP_HEADERSIZE_GREY+(int4) TowerBitmapWid*j+i]; }}
  [TowerMaskData release]; TowerMaskData=nil;

  //  Extract image data from rear-searchlight mask BMP.
  BYTE  *srchMskData=(BYTE *) [SearchlightMaskData bytes] ;
  for   (j=0L; j<DistributorBackdropHei; j++) {
    for (i=0L; i<DistributorBackdropWid; i++) {
      SearchlightMaskPlanes[0][DistributorBackdropWid*(DistributorBackdropHei-1L-j)+i]=srchMskData[BMP_HEADERSIZE_GREY+DistributorBackdropWid*j+i]; }}
  [SearchlightMaskData release]; SearchlightMaskData=nil;

  //  Acquire the initial contents of the text fields, for image previewing.
  copyTextUtf8ToLongStr(&text[0],(BYTE *) [[DistributorField stringValue] UTF8String],MAX_CHARS_DISTRIBUTOR); prepDistributorText(&text[0]); copyTextLong(&DistributorText[0],&text[0],MAX_CHARS_DISTRIBUTOR);
  copyTextUtf8ToLongStr(&text[0],(BYTE *) [[   DistInfoField stringValue] UTF8String],MAX_CHARS_DIST_INFO  ); prepDistInfoText   (&text[0]); copyTextLong(&   DistInfoText[0],&text[0],MAX_CHARS_DIST_INFO  ); [self canFailApp:prepareDistInfoChars()];
  copyTextUtf8ToLongStr(&text[0],(BYTE *) [[     FilmCoField stringValue] UTF8String],MAX_CHARS_FILM_CO    ); prepFilmCoText     (&text[0]); copyTextLong(&     FilmCoText[0],&text[0],MAX_CHARS_FILM_CO    );
  copyTextUtf8ToLongStr(&text[0],(BYTE *) [[  FilmCoIncField stringValue] UTF8String],MAX_CHARS_FILM_CO_INC); prepFilmCoIncText  (&text[0]); copyTextLong(&  FilmCoIncText[0],&text[0],MAX_CHARS_FILM_CO_INC);
  copyTextUtf8ToLongStr(&text[0],(BYTE *) [[      IntroField string     ] UTF8String],MAX_CHARS_INTRO      );                                copyTextLong(&      IntroText[0],&text[0],MAX_CHARS_INTRO      );
  copyTextUtf8ToLongStr(&text[0],(BYTE *) [[      TitleField stringValue] UTF8String],MAX_CHARS_TITLE      ); prepTitleText      (&text[0]); copyTextLong(&      TitleText[0],&text[0],MAX_CHARS_TITLE      );
  copyTextUtf8ToLongStr(&text[0],(BYTE *) [[      CrawlField string     ] UTF8String],MAX_CHARS_CRAWL      );                                copyTextLong(&      CrawlText[0],&text[0],MAX_CHARS_CRAWL      );

  //  Copy the content of the user controls from the Interface-Builder-created interface.
  [self copyControlValuesToVars];

  //  Read-in the user-content file, if one was requested on app launch.
  if (UserContentFullPath) [self readFile:UserContentFullPath];

  //  Register to be notified of relevant user activity.
  [[NSNotificationCenter defaultCenter] addObserver:self
  selector:@selector(userContentChanged:) name:NSTextDidChangeNotification object:nil];
  AppIsObserver=YES;

  //  Set the desired properties of the multi-line text fields (NSTextView objects).
  [IntroField setRichText:NO];
  [CrawlField setRichText:NO];

  //  Bring windows into view.
  [  ImageWindow           orderFront:self];
  [ControlWindow makeKeyAndOrderFront:self]; DisclaimerAgreed=YES;
  [         self checkForFieldFocusChanges];

  //  Force first preview image to be generated.
  [self userContentChanged:nil];

  //  Start periodic updating of the preview image.
  [self startImageUpdating]; }



//  Creates a delayed firing of the method updateImageAndAdvanceSequence, which will
//  in turn create a delayed firing of itself, etc., until the image is fully rendered.

- (void) startImageUpdating {
  if (!ImageUpdatePending) {
    ImageUpdatePending=YES; [self performSelector:@selector(updateImageAndAdvanceSequence) withObject:nil afterDelay:IMAGE_UPDATE_DELAY]; }}



//  Updates the image in the image window to match the offscreen bitmap, and
//  advance the rendering of the whole sequence as needed.
//
//  This method is invoked by delayed firing, in the method startImageUpdating.

- (void) updateImageAndAdvanceSequence {

  BYTE  *errText ;

  ImageUpdatePending=NO;

  //  If requested by the developer via a constant, show the thread-count diagnostic message in the control window’s title bar.
  if (THREAD_DIAGNOSTIC) [ControlWindow setTitle:[NSString stringWithFormat:@"RenderThreads: %d",RenderThreads]];

  [RenderLock lock];
  [BitmapLock lock];

  //  Update the image if rendering is underway, or if rendering just finished.
  if (ImageNeedsUpdate && (RenderThreads || WhichPreview==GENERATING || LastRenderImageUpdateNeeded && WhichPreview!=GENERATING)) {
    [TheImageView setImage:nil]; [TheImage release]; TheImage=nil;
    createImage(); [TheImageView setImage:TheImage]; if (!RenderThreads && WhichPreview!=GENERATING) LastRenderImageUpdateNeeded=NO; }
  ImageNeedsUpdate=NO;

  [BitmapLock unlock];
  [RenderLock unlock];

  //  Schedule the next image update.
  if (WhichPreview==GENERATING || RenderThreads) [self startImageUpdating];

  //  Handle full-sequence-render (not just a preview image).
  if (WhichPreview==GENERATING) {

    //  1.  Exit if the current frame is still rendering.
    if (RenderThreads) {
      return; }

    //  2.  If the current frame needs another rendering pass, start it and exit.
    if ((RenderStage==DISTRIBUTOR_FADE_UP
    ||   RenderStage==DISTRIBUTOR_HOLD
    ||   RenderStage==DISTRIBUTOR_FADE_DOWN) && !DistributorImageReady) {
      DistributorImageReady=YES; copyImageToBuffer(DistributorBuffer); forceRasterSearchlight(YES,YES); nullRaster(YES,NO);
      copyBufferToImageWithSubtractiveFade(DistributorBuffer,DistributorFade); ImageNeedsUpdate=YES;
      [self launchRenderingThreads:OVERSCAN_HI];
      return; }
    if (RenderStage==FILMCO_FADE_UP) {
      if      (FilmCoRenderMode==BORDERED_OVERLAY) copyImageToBuffer(FilmCoBufferGreen );
      else if (FilmCoRenderMode==BLUE_CORE       ) copyImageToBuffer(FilmCoBufferBlue  );
      else if (FilmCoRenderMode==ORANGE_TINT     ) copyImageToBuffer(FilmCoBufferOrange);
      else if (FilmCoRenderMode==YELLOW_TINT     ) copyImageToBuffer(FilmCoBufferYellow);
      if      (FilmCoRenderMode!=BUFFERS_READY   ) {
        FilmCoRenderMode++;
        if   (FilmCoRenderMode!=BUFFERS_READY) {
          forceRasterFilmCo(YES,YES);
          if (FilmCoRenderMode==BLUR_A_H     ) {
            prepareToDrawFilmCoImage(&Planes[0],(int4) [PreviewBitmap bytesPerRow],.7 ); BlurRow=0; BlurCol=0; }
          if (FilmCoRenderMode==BLUR_B_H     ) {
            prepareToDrawFilmCoImage(&Planes[0],(int4) [PreviewBitmap bytesPerRow],.15); BlurRow=0; BlurCol=0; }
          if (FilmCoRenderMode==BLUR_C_H     ) {
            prepareToDrawFilmCoImage(&Planes[0],(int4) [PreviewBitmap bytesPerRow],.15); BlurRow=0; BlurCol=0; }}
        else {
          forceRasterWholeImage(YES,YES); }
        nullRaster(YES,NO); [self launchRenderingThreads:OVERSCAN_HI];
        return; }}
    if (RenderStage==INTRO_FADE_UP && !IntroImageReady) {
      IntroImageReady=        YES;
      IntroFade      =stageFrac();
      copyImageToBuffer            (IntroBuffer          );
      copyBufferToImageWithMultFade(IntroBuffer,IntroFade); ImageNeedsUpdate=YES;
      [self launchRenderingThreads:OVERSCAN_HI];
      return; }
    if ((RenderStage==TITLE_PULL_BACK
    ||   RenderStage==CRAWL_AND_TITLE_PULL_BACK
    ||   RenderStage==CRAWL_AND_TITLE_FADE_DOWN
    ||   RenderStage==CRAWL_TO_ALL_TEXT_VISIBLE
    ||   RenderStage==CRAWL_OFF_TO_DISTANCE
    ||   RenderStage==CRAWL_FADE_DOWN
    ||   RenderStage==JUST_STARS               ) && !StarsImageReady && AutoStarfield) {
      forceRasterWholeImage(YES,YES); nullRaster(YES,NO);
      StarsImageReady=YES; setCrawlState(RenderStage,stageFrac(),self);
      copyImageToBuffer(CrawlBufferStars); [self launchRenderingThreads:OVERSCAN_HI];
      return; }

    //  3.  Write the rendered image to a file.
    errText=createAndWriteBmpFile([DestinationField stringValue],RenderFrame);
    if (errText) {
      [self alert:errText]; [self stopRender];
      return; }

    //  4.  Step to the next frame.
    RenderFrame++; showStatusFrame(StatusText,RenderFrame); MovieTime+=1./FramesPerSecond;
    [ProgressBar setDoubleValue:100.*MovieTime/SequenceTotalTime];
    #if GREEN_CYAN_DIAGNOSTIC
    fillImage(0,255,0); ImageNeedsUpdate=YES;
    #endif

    //  5.  Handle stage transition if we just stepped into a new stage -- or exit if the sequence is complete.
    if (MovieTime>=StageTimeNext) {
      RenderStage++;
      if (RenderStage==SEQUENCE_END) {
        [self stopRender]; NSBeep();
        [self alert:"Render completed!  (Consult tutorial at alienryderflex.com/crawl for info about compositing image files into a movie file.)"];
        return; }
      StageTime=StageTimeNext; StageTimeNext+=getSequenceTime(RenderStage);
      if (RenderStage==DISTRIBUTOR_FADE_UP) StageTimeSearchlights=StageTime;
      //  Make beginning-of-stage image-rendering settings.
      if      (RenderStage==DISTRIBUTOR_HOLD) {
        copyBufferToImage(DistributorBuffer); ImageNeedsUpdate=YES; }
      else if (RenderStage!=DISTRIBUTOR_FADE_DOWN
      &&       RenderStage!=INTRO
      &&       RenderStage!=INTRO_FADE_DOWN
      &&       RenderStage<=TITLE_PULL_BACK) {
        forceRasterWholeImage(YES,YES); }
      //  Handle various preparations for the new stage.
      if (RenderStage==DARK_B
      ||  RenderStage==DARK_C
      ||  RenderStage==DARK_D) {
        fillImage(0,0,0); ImageNeedsUpdate=YES; }
      if (RenderStage==DISTRIBUTOR_FADE_UP
      ||  RenderStage==DISTRIBUTOR_FADE_DOWN) {
        prepareToDrawDistributorImage(
        &Planes[0],&BackdropPlanes[0],(int4) [PreviewBitmap bytesPerRow],
        &TowerPlanes[0],&TowerMaskPlanes[0],&SearchlightMaskPlanes[0],TowerBitmapWid,TowerBitmapHei,
        (int4) [          TowerBitmap bytesPerRow],
        (int4) [      TowerMaskBitmap bytesPerRow],
        (int4) [SearchlightMaskBitmap bytesPerRow],
        (int4) [       BackdropBitmap bytesPerRow]);
        if (RenderStage==DISTRIBUTOR_FADE_UP) setDistributorFade(   stageFrac());
        else                                  setDistributorFade(1.-stageFrac());
        setSearchlightAngles(MovieTime-StageTimeSearchlights); }
      if (RenderStage==DISTRIBUTOR_HOLD) {
        setDistributorFade(1.); setSearchlightAngles(MovieTime-StageTimeSearchlights); }
      if (RenderStage==FILMCO_FADE_UP) {
        prepareToDrawFilmCoImage(&Planes[0],(int4) [PreviewBitmap bytesPerRow],1.);
        FilmCoFade      =stageFrac();
        FilmCoRenderMode=SILHOUETTE_A; }
      if (RenderStage==FILMCO_SPARKLE_GREEN_TO_BLUE
      ||  RenderStage==FILMCO_SPARKLE_BLUE_TO_ORANGE
      ||  RenderStage==FILMCO_ORANGE_TO_YELLOW
      ||  RenderStage==FILMCO_FADE_DOWN             ) {
        FilmCoFade=stageFrac(); }
      if (RenderStage==INTRO_FADE_UP) {
        [self canFailApp:prepareToDrawIntroImage(&Planes[0],(int4) [PreviewBitmap bytesPerRow])];
        IntroFade=stageFrac(); IntroImageReady=NO; }
      if (RenderStage==INTRO) IntroFade=1.;
      if (RenderStage==INTRO_FADE_DOWN) IntroFade=1.-stageFrac();
      if (RenderStage==TITLE_PULL_BACK) {
        prepareToDrawTitleImage                 (&Planes[0],(int4) [PreviewBitmap bytesPerRow]) ;
        [self canFailApp:prepareToDrawCrawlImage(&Planes[0],(int4) [PreviewBitmap bytesPerRow])]; }
      if (RenderStage==CRAWL_AND_TITLE_PULL_BACK
      ||  RenderStage==CRAWL_AND_TITLE_FADE_DOWN
      ||  RenderStage==CRAWL_TO_ALL_TEXT_VISIBLE
      ||  RenderStage==CRAWL_OFF_TO_DISTANCE
      ||  RenderStage==CRAWL_FADE_DOWN) {
        setCrawlState(RenderStage,stageFrac(),self); }}

    //  6.  Decide what areas of the frame should be rendered, and make other frame-by-frame adjustments.
    if (RenderStage==DISTRIBUTOR_FADE_UP) {
      setDistributorFade(   stageFrac()); setSearchlightAngles(MovieTime-StageTimeSearchlights);
      forceRasterSearchlight(YES,YES); copyBufferToImageWithSubtractiveFade(DistributorBuffer,DistributorFade); ImageNeedsUpdate=YES; }
    if (RenderStage==DISTRIBUTOR_HOLD) {
      setDistributorFade(1.);
      forceRasterSearchlight(YES,YES);    setSearchlightAngles(MovieTime-StageTimeSearchlights);
      forceRasterSearchlight(YES,YES); }
    if (RenderStage==DISTRIBUTOR_FADE_DOWN) {
      setDistributorFade(1.-stageFrac()); setSearchlightAngles(MovieTime-StageTimeSearchlights);
      forceRasterSearchlight(YES,YES); copyBufferToImageWithSubtractiveFade(DistributorBuffer,DistributorFade); ImageNeedsUpdate=YES; }
    if (RenderStage==FILMCO_FADE_UP
    ||  RenderStage==FILMCO_SPARKLE_GREEN_TO_BLUE
    ||  RenderStage==FILMCO_SPARKLE_BLUE_TO_ORANGE
    ||  RenderStage==FILMCO_ORANGE_TO_YELLOW
    ||  RenderStage==FILMCO_FADE_DOWN             ) {
      FilmCoFade=stageFrac();
      forceRasterWholeImage(YES,YES); }
    if (RenderStage==FILMCO_SPARKLE_GREEN) {
      forceRasterWholeImage(YES,YES); }
    if (RenderStage>=FILMCO_FADE_UP
    &&  RenderStage<=FILMCO_FADE_DOWN) {
      setFilmCoProgressByMovieTime(MovieTime); }
    if (RenderStage>=INTRO_FADE_UP
    &&  RenderStage<=INTRO_FADE_DOWN) {
      if (RenderStage==INTRO_FADE_UP  ) IntroFade=   stageFrac();
      if (RenderStage==INTRO          ) IntroFade=1.            ;
      if (RenderStage==INTRO_FADE_DOWN) IntroFade=1.-stageFrac();
      if (RenderStage!=INTRO          ) {
        copyBufferToImageWithMultFade(IntroBuffer,IntroFade); ImageNeedsUpdate=YES; }}
    if (RenderStage==          TITLE_PULL_BACK
    ||  RenderStage==CRAWL_AND_TITLE_PULL_BACK
    ||  RenderStage==CRAWL_AND_TITLE_FADE_DOWN) {
      forceCrawlRowArrayCreation();
      forceRasterRect(YES,YES,
      (int4) screenToPixelX(-MOVIE_SCREEN_WID/2./TitleDist)-2,
      (int4) screenToPixelY( MOVIE_SCREEN_HEI/2./TitleDist)-2,
      (int4) screenToPixelX( MOVIE_SCREEN_WID/2./TitleDist)+3,
      (int4) screenToPixelY(-MOVIE_SCREEN_HEI/2./TitleDist)+3);
      setCrawlState(RenderStage,stageFrac(),self);
      if (RenderStage==CRAWL_AND_TITLE_PULL_BACK
      ||  RenderStage==CRAWL_AND_TITLE_FADE_DOWN) {
        forceRasterCrawl(YES,YES); }}
    if (RenderStage==CRAWL_TO_ALL_TEXT_VISIBLE
    ||  RenderStage==CRAWL_OFF_TO_DISTANCE
    ||  RenderStage==CRAWL_FADE_DOWN      ) {
      setCrawlState(RenderStage,stageFrac(),self); forceRasterCrawl(YES,YES); forceCrawlRowArrayCreation(); }
    if (RenderStage==JUST_STARS) {
      setCrawlState(RenderStage,stageFrac(),self); }
    nullRaster(YES,NO);

    //  7.  Start new frame rendering and exit.
    [self launchRenderingThreads:OVERSCAN_HI];
    return; }

  //  We’re not doing a full-sequence render right now -- handle other events as follows:

  //  If a lo-sampling pass has just finished, restart image rendering in hi-sampling mode.
  [RenderLock lock];
  if (!KillRenderThreads && !RenderThreads && RasterSubPixels!=OVERSCAN_HI) {
    #if GREEN_CYAN_DIAGNOSTIC
    fillImage(0,255,255); ImageNeedsUpdate=YES; 
    #endif
    [RenderLock unlock]; [self launchRenderingThreads:OVERSCAN_HI]; }
  else {
    [RenderLock unlock]; }}



//  This method is invoked when the user chooses “Open” from the menu bar.

- (IBAction) handleOpen:(id) sender {

  NSOpenPanel  *thePanel ;
  int4          answer   ;

  [self killRenderingThreads];

  //  Give the user a chance to save unsaved changes.
  if (!ChangesSaved) {
    answer=[self ask3:"Save your changes before replacing them with the contents of another file?"
    zero:"Save Changes" one:"Discard Changes" two:"Cancel Open"];
    if (answer<0 || answer && ![self saveUserContent]) {
      [self launchRenderingThreads:RasterSubPixels];
      return; }}

  //  Let the user choose a file to open.
  thePanel=[NSOpenPanel openPanel];
  if (!thePanel) {
    [self alert:"Unable to create an NSOpenPanel in handleOpen.  App may be unstable."];
    return; }
  [thePanel setCanChooseDirectories   : NO];
  [thePanel setCanChooseFiles         :YES];
  [thePanel setAllowsMultipleSelection: NO];
  if ([thePanel runModalForTypes:[NSArray arrayWithObject:[@USER_CONTENT_FILE_EXTENSION substringFromIndex:1]]]==NSOKButton) {

    //  Read-in the user-chosen file.
    if (UserContentFullPath) [UserContentFullPath release];
    UserContentFullPath=[[[thePanel filenames] objectAtIndex:0] copy];
    if ([self readFile:UserContentFullPath]) {
      resetUndoArray(); nullRaster(YES,YES); [self userContentChanged:nil]; ChangesSaved=YES; }}

  if (!RenderThreads) [self launchRenderingThreads:RasterSubPixels]; }



//  These two functions work only for the two multi-line text fields (Intro and Crawl),
//  and assume the field tab order has not been rearranged.

- (void) moveToNextResponder:(id) sender {
  if      (hasFocus(IntroField)) [ControlWindow makeFirstResponder:      TitleField];
  else if (hasFocus(CrawlField)) [ControlWindow makeFirstResponder:DistributorField]; }

- (void) moveToPrevResponder:(id) sender {
  if      (hasFocus(IntroField)) [ControlWindow makeFirstResponder:  FilmCoIncField];
  else if (hasFocus(CrawlField)) [ControlWindow makeFirstResponder:      TitleField]; }



//  This method is invoked when the user chooses “Save” from the menu bar.

- (IBAction) handleSave:(id) sender {

  [self killRenderingThreads];

  if ([self saveUserContent]) {   //  (file was successfully saved)

    //  Set the control window’s title to the filename.
    [ControlWindow setTitle:[self windowTitleFromPath:UserContentFullPath]]; }

  [self launchRenderingThreads:RasterSubPixels]; }



//  This method is invoked when the user chooses “Save As” from the menu bar.

- (IBAction) handleSaveAs:(id) sender {

  [self killRenderingThreads];

  //  Stash the path, in case Save As is cancelled.
  NSString  *oldPath=UserContentFullPath ;

  //  Clear the path, so saveUserContent will ask the user to choose a new one.
  UserContentFullPath=nil;

  if ([self saveUserContent]) {   //  (file was successfully saved)

    //  Set the control window’s title to the filename.
    [ControlWindow setTitle:[self windowTitleFromPath:UserContentFullPath]];

    //  Release the old path if necessary.
    if (oldPath) [oldPath release]; }

  else {   //  (file was not saved)

    //  Restore the file path to its previous value.
    UserContentFullPath=oldPath; }

  [self launchRenderingThreads:RasterSubPixels]; }



//  This method is automatically invoked when the application gets a request from the OS to open a specified file.

- (bool) application:(NSApplication *) theApp openFile:(NSString *) fullFilePath {

  int4  answer ;

  [self killRenderingThreads];

  //  Disallow file-opening during full-sequence render.
  if (WhichPreview==GENERATING) {
    [self alert:"You cannot open a file during a full-sequence render."];
    [self launchRenderingThreads:RasterSubPixels];
    return NO; }

  //  Give the user a chance to save unsaved changes.
  if (!ChangesSaved) {
    answer=[self ask3:"Save your changes before replacing them with the contents of another file?"
    zero:"Save Changes" one:"Discard Changes" two:"Cancel Open"];
    if (answer<0 || answer && ![self saveUserContent]) {
      [self launchRenderingThreads:RasterSubPixels];
      return NO; }}

  //  Store the full path of the specified file.
  if (UserContentFullPath) [UserContentFullPath release];
  UserContentFullPath=[fullFilePath copy];

  //  Read-in the file, if the disclaimer has already been agreed-to.
  if (DisclaimerAgreed && [self readFile:UserContentFullPath]) {
    nullRaster(YES,YES); [self userContentChanged:nil];
    return YES; }
  else {
    [self launchRenderingThreads:RasterSubPixels];
    return  NO; }}



//  Read a user file.
//
//  Returns YES on success, otherwise NO.
//
//  Note:  Not to be called with render-threads running.

- (bool) readFile:(NSString *) fullPath {

  int4  ioDistributor     [MAX_CHARS_DISTRIBUTOR+1] ;
  int4  ioDistributorInfo [MAX_CHARS_DIST_INFO  +1] ;
  int4  ioFilmCompany     [MAX_CHARS_FILM_CO    +1] ;
  int4  ioFilmCompanyInc  [MAX_CHARS_FILM_CO_INC+1] ;
  int4  ioIntro           [MAX_CHARS_INTRO      +1] ;
  int4  ioTitle           [MAX_CHARS_TITLE      +1] ;
  int4  ioCrawl           [MAX_CHARS_CRAWL      +1] ;
  int4  ioScreen          [MAX_CHARS_OPTION     +1] ;
  int4  ioStarfieldAuto   [MAX_CHARS_CHECKBOX   +1] ;
  int4  ioScreenWidth     [MAX_CHARS_FIELD_INT  +1] ;
  int4  ioScreenHeight    [MAX_CHARS_FIELD_INT  +1] ;
  int4  ioScreenHeightUsed[MAX_CHARS_FIELD_INT  +1] ;
  int4  ioFrameRate       [MAX_CHARS_OPTION     +1] ;
  int4  ioFrameRateFps    [MAX_CHARS_FIELD_FLOAT+1] ;
  int4  ioDestination     [MAX_CHARS_FIELD_PATH +1] ;
  int4  ioStarfield       [MAX_CHARS_FIELD_PATH +1] ;

  //  Parse the XML-like content out of the file and into the above string spaces.
  BYTE  *err=parseSwtsgFile(fullPath,
  &ioDistributor     [0],sizeof(ioDistributor     )/sizeof(ioDistributor     [0])-1,
  &ioDistributorInfo [0],sizeof(ioDistributorInfo )/sizeof(ioDistributorInfo [0])-1,
  &ioFilmCompany     [0],sizeof(ioFilmCompany     )/sizeof(ioFilmCompany     [0])-1,
  &ioFilmCompanyInc  [0],sizeof(ioFilmCompanyInc  )/sizeof(ioFilmCompanyInc  [0])-1,
  &ioIntro           [0],sizeof(ioIntro           )/sizeof(ioIntro           [0])-1,
  &ioTitle           [0],sizeof(ioTitle           )/sizeof(ioTitle           [0])-1,
  &ioCrawl           [0],sizeof(ioCrawl           )/sizeof(ioCrawl           [0])-1,
  &ioScreen          [0],sizeof(ioScreen          )/sizeof(ioScreen          [0])-1,
  &ioStarfieldAuto   [0],sizeof(ioStarfieldAuto   )/sizeof(ioStarfieldAuto   [0])-1,
  &ioScreenWidth     [0],sizeof(ioScreenWidth     )/sizeof(ioScreenWidth     [0])-1,
  &ioScreenHeight    [0],sizeof(ioScreenHeight    )/sizeof(ioScreenHeight    [0])-1,
  &ioScreenHeightUsed[0],sizeof(ioScreenHeightUsed)/sizeof(ioScreenHeightUsed[0])-1,
  &ioFrameRate       [0],sizeof(ioFrameRate       )/sizeof(ioFrameRate       [0])-1,
  &ioFrameRateFps    [0],sizeof(ioFrameRateFps    )/sizeof(ioFrameRateFps    [0])-1,
  &ioDestination     [0],sizeof(ioDestination     )/sizeof(ioDestination     [0])-1,
  &ioStarfield       [0],sizeof(ioStarfield       )/sizeof(ioStarfield       [0])-1);
  if (err) {
    [self alert:err]; return NO; }

  //  Force data integrity for radio-button and checkbox settings.
  if (       ioScreen[0]<'0' ||        ioScreen[0]> '7' ||        ioScreen[1]) {
    ioScreen         [0]='7';          ioScreen[1]='\0'; }
  if (    ioFrameRate[0]<'0' ||     ioFrameRate[0]> '3' ||     ioFrameRate[1]) {
    ioFrameRate      [0]='3';       ioFrameRate[1]='\0'; }
  if (ioStarfieldAuto[0]<'0' || ioStarfieldAuto[0]> '1' || ioStarfieldAuto[1]) {
    ioStarfieldAuto  [0]='0';   ioStarfieldAuto[1]='\0'; }

  //  Load the fields and buttons with the acquired data.
  convertLongStrToUtf8(&ioDistributor     [0]); [      DistributorField setStringValue :[NSString stringWithUTF8String:(BYTE *) &ioDistributor     [0]]];
  convertLongStrToUtf8(&ioDistributorInfo [0]); [         DistInfoField setStringValue :[NSString stringWithUTF8String:(BYTE *) &ioDistributorInfo [0]]];
  convertLongStrToUtf8(&ioFilmCompany     [0]); [           FilmCoField setStringValue :[NSString stringWithUTF8String:(BYTE *) &ioFilmCompany     [0]]];
  convertLongStrToUtf8(&ioFilmCompanyInc  [0]); [        FilmCoIncField setStringValue :[NSString stringWithUTF8String:(BYTE *) &ioFilmCompanyInc  [0]]];
  convertLongStrToUtf8(&ioIntro           [0]); [            IntroField setString      :[NSString stringWithUTF8String:(BYTE *) &ioIntro           [0]]];
  convertLongStrToUtf8(&ioTitle           [0]); [            TitleField setStringValue :[NSString stringWithUTF8String:(BYTE *) &ioTitle           [0]]];
  convertLongStrToUtf8(&ioCrawl           [0]); [            CrawlField setString      :[NSString stringWithUTF8String:(BYTE *) &ioCrawl           [0]]];
  [                                              ScreenSizeRadioButtons selectCellAtRow:(int4) (          ioScreen[0]-'0')                   column:0  ];
  [                                               StarfieldAutoCheckbox setState       :(int4) (   ioStarfieldAuto[0]-'0')                             ];
  convertLongStrToUtf8(&ioScreenWidth     [0]); [            WidthField setStringValue :[NSString stringWithUTF8String:(BYTE *) &ioScreenWidth     [0]]];
  convertLongStrToUtf8(&ioScreenHeight    [0]); [           HeightField setStringValue :[NSString stringWithUTF8String:(BYTE *) &ioScreenHeight    [0]]];
  convertLongStrToUtf8(&ioScreenHeightUsed[0]); [       HeightUsedField setStringValue :[NSString stringWithUTF8String:(BYTE *) &ioScreenHeightUsed[0]]];
  [                                               FrameRateRadioButtons selectCellAtRow:(int4) (       ioFrameRate[0]-'0')                   column:0  ];
  convertLongStrToUtf8(&ioFrameRateFps    [0]); [        FrameRateField setStringValue :[NSString stringWithUTF8String:(BYTE *) &ioFrameRateFps    [0]]];
  convertLongStrToUtf8(&ioDestination     [0]); [      DestinationField setStringValue :[NSString stringWithUTF8String:(BYTE *) &ioDestination     [0]]];
  convertLongStrToUtf8(&ioStarfield       [0]); [        StarfieldField setStringValue :[NSString stringWithUTF8String:(BYTE *) &ioStarfield       [0]]];

  //  Set the editability of the text fields as per the user settings.
  CustomImageSize=(ioScreen       [0]=='7'); [     WidthField setEditable:CustomImageSize];
  [                                               HeightField setEditable:CustomImageSize];
  [                                           HeightUsedField setEditable:CustomImageSize];
  CustomFrameRate=(ioFrameRate    [0]=='3'); [ FrameRateField setEditable:CustomFrameRate];
  AutoStarfield  =(ioStarfieldAuto[0]=='1'); [ StarfieldField setEditable:!AutoStarfield ];
  [                                     StarfieldChooseButton setEnabled :!AutoStarfield ];

  //  Stash acquired control values in variables that this app controls.
  [self copyControlValuesToVars];

  //  Set the control window’s title to the filename.
  [ControlWindow setTitle:[self windowTitleFromPath:fullPath]];

  //  Wrap-up.
  ChangesSaved=YES; return YES; }



//  Copy the controls values back to change-detection variables (for use by Undo-tracking code).

- (void) copyControlValuesToVars {

  ScreenSizeRadioButtonsSelectedRow=[ScreenSizeRadioButtons selectedRow];
  FrameRateRadioButtonsSelectedRow =[ FrameRateRadioButtons selectedRow];

  copyTextUtf8ToLongStr(&DistributorFieldContents[0],(BYTE *) [[DistributorField stringValue] UTF8String],MAX_CHARS_DISTRIBUTOR);
  copyTextUtf8ToLongStr(&   DistInfoFieldContents[0],(BYTE *) [[   DistInfoField stringValue] UTF8String],MAX_CHARS_DIST_INFO  );
  copyTextUtf8ToLongStr(&     FilmCoFieldContents[0],(BYTE *) [[     FilmCoField stringValue] UTF8String],MAX_CHARS_FILM_CO    );
  copyTextUtf8ToLongStr(&  FilmCoIncFieldContents[0],(BYTE *) [[  FilmCoIncField stringValue] UTF8String],MAX_CHARS_FILM_CO_INC);
  copyTextUtf8ToLongStr(&      IntroFieldContents[0],(BYTE *) [[      IntroField string     ] UTF8String],MAX_CHARS_INTRO      );
  copyTextUtf8ToLongStr(&      TitleFieldContents[0],(BYTE *) [[      TitleField stringValue] UTF8String],MAX_CHARS_TITLE      );
  copyTextUtf8ToLongStr(&      CrawlFieldContents[0],(BYTE *) [[      CrawlField string     ] UTF8String],MAX_CHARS_CRAWL      );
  copyTextUtf8ToLongStr(&      WidthFieldContents[0],(BYTE *) [[      WidthField stringValue] UTF8String],MAX_CHARS_FIELD_INT  );
  copyTextUtf8ToLongStr(&     HeightFieldContents[0],(BYTE *) [[     HeightField stringValue] UTF8String],MAX_CHARS_FIELD_INT  );
  copyTextUtf8ToLongStr(& HeightUsedFieldContents[0],(BYTE *) [[ HeightUsedField stringValue] UTF8String],MAX_CHARS_FIELD_INT  );
  copyTextUtf8ToLongStr(&  FrameRateFieldContents[0],(BYTE *) [[  FrameRateField stringValue] UTF8String],MAX_CHARS_FIELD_FLOAT);
  copyTextUtf8ToLongStr(&DestinationFieldContents[0],(BYTE *) [[DestinationField stringValue] UTF8String],MAX_CHARS_FIELD_PATH ); }



//  Strips off the folder path and file extension, to make a simple filename that can be used as a window title.

- (NSString *) windowTitleFromPath:(NSString *) path {

  NSRange  oneChar ;

  //  Strip off the filename extension.
  if  ([[[path substringFromIndex:[path length]-[@USER_CONTENT_FILE_EXTENSION length]] lowercaseString]
  isEqualToString:[@USER_CONTENT_FILE_EXTENSION lowercaseString]]) {
    path=[path substringToIndex  :[path length]-[@USER_CONTENT_FILE_EXTENSION length]]; }

  //  Strip off the folder information.
  oneChar.location=[path length]; oneChar.length=1;
  while (oneChar.location) {
    oneChar.location--;
    if ([[path substringWithRange:oneChar] isEqualToString:@"/"]) {
      path=[path substringFromIndex:oneChar.location+1]; oneChar.location=0; }}

  //  Return the result.
  return path; }



//  This function is automatically invoked when the user selects Quit from the menu bar or presses
//  Cmd-Q, or when the OS asks the app to quit.
//
//  This method cannot be named “handleQuit” -- it must be named as it is, for the OS to invoke it.

- (NSApplicationTerminateReply) applicationShouldTerminate:(NSApplication *) sender {

  int4  answer ;

  //  Give the user a chance to prevent a running render from being stopped.
  if (WhichPreview==GENERATING && ![self abortRender]) return NSTerminateCancel;

  //  Give the user a chance to save unsaved changes.
  if (!ChangesSaved) {
    answer=[self ask3:"Save your changes before quitting?" zero:"Save Changes" one:"Discard Changes" two:"Cancel Quit"];
    if (answer<0 || answer && ![self saveUserContent]) return NSTerminateCancel; }

  //  Disengage/deallocate various things before terminating app.
  if    (DistributorGlyph) {
    free(DistributorGlyph); DistributorGlyph=nil; }
  if    (     FilmCoGlyph) {
    free(     FilmCoGlyph);      FilmCoGlyph=nil; }
  if    (      TitleGlyph) {
    free(      TitleGlyph);       TitleGlyph=nil; }
  [self killRenderingThreads];
  if (AppIsObserver) {
    AppIsObserver=NO; [[NSNotificationCenter defaultCenter] removeObserver:self]; }
  releaseFilmCoBuffers  ();
  releaseBellCurveArrays();
  if (            TheImage ) {
    [             TheImage  release];             TheImage =nil; }
  if (        PreviewBitmap) {
    [         PreviewBitmap release];         PreviewBitmap=nil; }
  if (    PreviewBitmapCopy) {
    [     PreviewBitmapCopy release];     PreviewBitmapCopy=nil; }
  if (       BackdropBitmap) {
    [        BackdropBitmap release];        BackdropBitmap=nil; }
  if (          TowerBitmap) {
    [           TowerBitmap release];           TowerBitmap=nil; }
  if (      TowerMaskBitmap) {
    [       TowerMaskBitmap release];       TowerMaskBitmap=nil; }
  if (SearchlightMaskBitmap) {
    [ SearchlightMaskBitmap release]; SearchlightMaskBitmap=nil; }
  if (  UserContentFullPath) {
    [   UserContentFullPath release];   UserContentFullPath=nil; }
  if (     DistInfoFontData) {
    [      DistInfoFontData release];      DistInfoFontData=nil; }
  if (        IntroFontData) {
    [         IntroFontData release];         IntroFontData=nil; }
  if (        CrawlFontData) {
    [         CrawlFontData release];         CrawlFontData=nil; }
  if (  SearchlightMaskData) {
    [   SearchlightMaskData release];   SearchlightMaskData=nil; }
  if (            TowerData) {
    [             TowerData release];             TowerData=nil; }
  if (        TowerMaskData) {
    [         TowerMaskData release];         TowerMaskData=nil; }
  if (     DistBackdropData) {
    [      DistBackdropData release];      DistBackdropData=nil; }
  if (           RenderLock) {
    [            RenderLock release];            RenderLock=nil; }
  if (           BitmapLock) {
    [            BitmapLock release];            BitmapLock=nil; }

  //  All clear -- allow the app to terminate.
  return NSTerminateNow; }



//  This function attempts to write the user-content to a file.  It returns YES if it actually wrote it, or NO
//  if for any reason it didn’t.
//
//  Presents a save-file dialog to the user only if no path has been set by a previous file-save or file-open.
//
//  This function does not set the window title to the file name -- that should be done by the calling code.

- (bool) saveUserContent {

  int4           i, strLen=0, errSize=2000000000 ;
  BYTE          *dataUtf8String, *attrVal ;
  NSString      *dataString ;
  NSSavePanel   *thePanel   ;
  NSFileHandle  *theFile    ;

  //  Present a save-file dialog if a user-content file isn’t already specified.
  if (!UserContentFullPath) {
    thePanel=[NSSavePanel savePanel];
    [thePanel setAllowedFileTypes:[NSArray arrayWithObject:[@USER_CONTENT_FILE_EXTENSION substringFromIndex:1]]];
    if ([thePanel runModal]==NSFileHandlingPanelOKButton) UserContentFullPath=[[thePanel filename] copy];
    else                                                  return NO; }

  //  Create the (empty) file.
  if (![[NSFileManager defaultManager] createFileAtPath:[UserContentFullPath stringByExpandingTildeInPath] contents:nil attributes:nil]) {
    [self alert:"Unable to create the user file."          ]; return NO; }

  //  Open the file for writing.
  theFile=     [NSFileHandle fileHandleForWritingAtPath:[UserContentFullPath stringByExpandingTildeInPath]];
  if (!theFile) {
    [self alert:"Unable to open the user file for writing."]; return NO; }

  //  Construct XML data to be written to the file.  (This code creates and abandons
  //  a fair number of temporary NSString objects, but they are all auto-released.)
  dataString=[  NSString stringWithUTF8String   : "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r"];
  dataString=[dataString stringByAppendingString:@"<StarWarsTSG>\r"                             ];
  dataString=[dataString stringByAppendingString:@"  <Distributor>"                             ];
  dataString=[dataString stringByAppendingString:[self xmlSafe:[DistributorField stringValue]]  ];
  dataString=[dataString stringByAppendingString:@"</Distributor>\r"                            ];
  dataString=[dataString stringByAppendingString:@"  <DistributorInfo>"                         ];
  dataString=[dataString stringByAppendingString:[self xmlSafe:[   DistInfoField stringValue]]  ];
  dataString=[dataString stringByAppendingString:@"</DistributorInfo>\r"                        ];
  dataString=[dataString stringByAppendingString:@"  <FilmCompany>"                             ];
  dataString=[dataString stringByAppendingString:[self xmlSafe:[     FilmCoField stringValue]]  ];
  dataString=[dataString stringByAppendingString:@"</FilmCompany>\r"                            ];
  dataString=[dataString stringByAppendingString:@"  <FilmCompanyInc>"                          ];
  dataString=[dataString stringByAppendingString:[self xmlSafe:[  FilmCoIncField stringValue]]  ];
  dataString=[dataString stringByAppendingString:@"</FilmCompanyInc>\r"                         ];
  dataString=[dataString stringByAppendingString:@"  <Intro>"                                   ];
  dataString=[dataString stringByAppendingString:[self xmlSafe:[      IntroField string     ]]  ];
  dataString=[dataString stringByAppendingString:@"</Intro>\r"                                  ];
  dataString=[dataString stringByAppendingString:@"  <Title>"                                   ];
  dataString=[dataString stringByAppendingString:[self xmlSafe:[      TitleField stringValue]]  ];
  dataString=[dataString stringByAppendingString:@"</Title>\r"                                  ];
  dataString=[dataString stringByAppendingString:@"  <Crawl>"                                   ];
  dataString=[dataString stringByAppendingString:[self xmlSafe:[      CrawlField string     ]]  ];
  dataString=[dataString stringByAppendingString:@"</Crawl>\r"                                  ];
  dataString=[dataString stringByAppendingString:@"  <Screen>"                                  ];
  dataString=[dataString stringByAppendingString:[NSString stringWithFormat:@"%d",[ScreenSizeRadioButtons selectedRow]]];
  dataString=[dataString stringByAppendingString:@"</Screen>\r"                                 ];
  dataString=[dataString stringByAppendingString:@"  <StarfieldAuto>"                           ];
  dataString=[dataString stringByAppendingString:[NSString stringWithFormat:@"%d",[StarfieldAutoCheckbox        state]]];
  dataString=[dataString stringByAppendingString:@"</StarfieldAuto>\r"                          ];
  dataString=[dataString stringByAppendingString:@"  <ScreenWidth>"                             ];
  dataString=[dataString stringByAppendingString:[self xmlSafe:[      WidthField stringValue]]  ];
  dataString=[dataString stringByAppendingString:@"</ScreenWidth>\r"                            ];
  dataString=[dataString stringByAppendingString:@"  <ScreenHeight>"                            ];
  dataString=[dataString stringByAppendingString:[self xmlSafe:[     HeightField stringValue]]  ];
  dataString=[dataString stringByAppendingString:@"</ScreenHeight>\r"                           ];
  dataString=[dataString stringByAppendingString:@"  <ScreenHeightUsed>"                        ];
  dataString=[dataString stringByAppendingString:[self xmlSafe:[ HeightUsedField stringValue]]  ];
  dataString=[dataString stringByAppendingString:@"</ScreenHeightUsed>\r"                       ];
  dataString=[dataString stringByAppendingString:@"  <FrameRate>"                               ];
  dataString=[dataString stringByAppendingString:[NSString stringWithFormat:@"%d",[FrameRateRadioButtons selectedRow]]];
  dataString=[dataString stringByAppendingString:@"</FrameRate>\r"                              ];
  dataString=[dataString stringByAppendingString:@"  <FrameRateFps>"                            ];
  dataString=[dataString stringByAppendingString:[self xmlSafe:[  FrameRateField stringValue]]  ];
  dataString=[dataString stringByAppendingString:@"</FrameRateFps>\r"                           ];
  dataString=[dataString stringByAppendingString:@"  <Destination>"                             ];
  dataString=[dataString stringByAppendingString:[self xmlSafe:[DestinationField stringValue]]  ];
  dataString=[dataString stringByAppendingString:@"</Destination>\r"                            ];
  dataString=[dataString stringByAppendingString:@"  <Starfield>"                               ];
  dataString=[dataString stringByAppendingString:[self xmlSafe:[  StarfieldField stringValue]]  ];
  dataString=[dataString stringByAppendingString:@"</Starfield>\r"                              ];
  dataString=[dataString stringByAppendingString:@"</StarWarsTSG>\r"                            ];

  //  Write the XML data to the file.
  dataUtf8String=(BYTE *) [dataString UTF8String]; while (dataUtf8String[strLen] && strLen<errSize) strLen++;
  if (strLen>=errSize) {
    [theFile closeFile]; [self alert:"Size overrun when attempting to write user file."]; return NO; }
  [theFile writeData:[NSData dataWithBytes:dataUtf8String length:strLen]];

  //  Close the file.
  [theFile closeFile];

  //  Tag the file so other apps can tell it’s UTF-8.
  //  Note:  SWTSG never looks at this tag; it decides whether the user file is XML based on
  //         the presence/absense of the standard XML header.
  //  Note:  If this fails, so be it.  We just try to set the attribute, and then move on.
  attrVal=UTF8_ATTR_VALUE; i=0; while (attrVal[i]) i++;
  setxattr([UserContentFullPath UTF8String],UTF8_ATTR_NAME,UTF8_ATTR_VALUE,i,0,XATTR_NOFOLLOW);

  //  Success.
  ChangesSaved=YES; pushSaveMarkerOntoUndoArray(); return YES; }



//  This function is automatically invoked whenever the user clicks on the menu
//  bar.  It decides which menu items will be active and which will be dimmed-out.

- (bool) validateMenuItem:(NSMenuItem *) theItem {
  return !(
  [[theItem title] isEqualToString:@"Open..."   ] && (!DisclaimerAgreed || WhichPreview==GENERATING) ||
  [[theItem title] isEqualToString:@"Save"      ] &&   ChangesSaved                                  ||
  [[theItem title] isEqualToString:@"Save As..."] && (!DisclaimerAgreed || WhichPreview==GENERATING) ||
  [[theItem title] isEqualToString:@"Undo"      ] && (!UndoCount        || WhichPreview==GENERATING) ||
  [[theItem title] isEqualToString:@"Redo"      ] && (!RedoCount        || WhichPreview==GENERATING)   ); }



//  This is a delegate function of NSTextField.
//
//  Note:  textLimit is initialized only to prevent a compiler warning -- it shouldn’t need to be.

- (IBAction) controlTextDidChange:(id) sender {

  NSTextField  *field    =nil ;
  int4          textLimit=0   ;

  //  Limit the text to a defined maximum length.
  if (hasFocus(DistributorField)) {
    field=DistributorField; textLimit=MAX_CHARS_DISTRIBUTOR; }
  if (hasFocus(   DistInfoField)) {
    field=   DistInfoField; textLimit=MAX_CHARS_DIST_INFO  ; }
  if (hasFocus(     FilmCoField)) {
    field=     FilmCoField; textLimit=MAX_CHARS_FILM_CO    ; }
  if (hasFocus(  FilmCoIncField)) {
    field=  FilmCoIncField; textLimit=MAX_CHARS_FILM_CO_INC; }
  if (hasFocus(      TitleField)) {
    field=      TitleField; textLimit=MAX_CHARS_TITLE      ; }
  if (hasFocus(      WidthField)) {
    field=      WidthField; textLimit=MAX_CHARS_FIELD_INT  ; }
  if (hasFocus(     HeightField)) {
    field=     HeightField; textLimit=MAX_CHARS_FIELD_INT  ; }
  if (hasFocus( HeightUsedField)) {
    field= HeightUsedField; textLimit=MAX_CHARS_FIELD_INT  ; }
  if (hasFocus(  FrameRateField)) {
    field=  FrameRateField; textLimit=MAX_CHARS_FIELD_FLOAT; }
  if (hasFocus(DestinationField)) {
    field=DestinationField; textLimit=MAX_CHARS_FIELD_PATH ; }
  if (hasFocus(  StarfieldField)) {
    field=  StarfieldField; textLimit=MAX_CHARS_FIELD_PATH ; }
  if (field && [[field stringValue] length]>textLimit) {
    [self alert:TEXT_LIMIT_REACHED];
    [field setStringValue:[[field stringValue] substringToIndex:textLimit]]; [self userContentChanged:nil]; }

  //  Create an Undo item so the user’s change can be undone.
  if (field==DistributorField) makeFieldUndo(DistributorField,UNDO_TEXT_DISTRIBUTOR,&DistributorFieldContents[0]);
  if (field==   DistInfoField) makeFieldUndo(   DistInfoField,UNDO_TEXT_DISTINFO   ,&   DistInfoFieldContents[0]);
  if (field==     FilmCoField) makeFieldUndo(     FilmCoField,UNDO_TEXT_FILMCO     ,&     FilmCoFieldContents[0]);
  if (field==  FilmCoIncField) makeFieldUndo(  FilmCoIncField,UNDO_TEXT_FILMCOINC  ,&  FilmCoIncFieldContents[0]);
  if (field==      TitleField) makeFieldUndo(      TitleField,UNDO_TEXT_TITLE      ,&      TitleFieldContents[0]);
  if (field==      WidthField) makeFieldUndo(      WidthField,UNDO_TEXT_WIDTH      ,&      WidthFieldContents[0]);
  if (field==     HeightField) makeFieldUndo(     HeightField,UNDO_TEXT_HEIGHT     ,&     HeightFieldContents[0]);
  if (field== HeightUsedField) makeFieldUndo( HeightUsedField,UNDO_TEXT_HEIGHTUSED ,& HeightUsedFieldContents[0]);
  if (field==  FrameRateField) makeFieldUndo(  FrameRateField,UNDO_TEXT_FRAMERATE  ,&  FrameRateFieldContents[0]);
  if (field==DestinationField) makeFieldUndo(DestinationField,UNDO_TEXT_DESTINATION,&DestinationFieldContents[0]);
  if (field==  StarfieldField) makeFieldUndo(  StarfieldField,UNDO_TEXT_STARFIELD  ,&  StarfieldFieldContents[0]); }



//  This is a delegate function of NSTextView.
//
//  Note:  textLimit is initialized only to prevent a compiler warning -- it shouldn’t need to be.

- (void) textDidChange:(NSNotification *) aNotification {

  NSTextView  *field    =nil ;
  int4         textLimit=0   ;

  //  Limit the text to a defined maximum length.
  if (hasFocus(IntroField)) {
    field=IntroField; textLimit=MAX_CHARS_INTRO; }
  if (hasFocus(CrawlField)) {
    field=CrawlField; textLimit=MAX_CHARS_CRAWL; }
  if (field && [[field string] length]>textLimit) {
    [self alert:TEXT_LIMIT_REACHED];
    [field setString:[[field string] substringToIndex:textLimit]]; [self userContentChanged:nil]; }

  //  Create an Undo item so the user’s change can be undone.
  if (field==IntroField) makeViewUndo(IntroField,UNDO_TEXT_INTRO,&IntroFieldContents[0]);
  if (field==CrawlField) makeViewUndo(CrawlField,UNDO_TEXT_CRAWL,&CrawlFieldContents[0]); }



//  This method is invoked when the user chooses “Undo” from the menu bar.

- (IBAction) handleUndo:(id) sender {

  int4   text[TEXT_FIELD_MAX+1] ;
  short  type, data ;

  //  Exit if there is nothing to undo.
  if (!UndoCount) return;

  [self killRenderingThreads];

  //  Warn the user if they are about to step back over a save-file event.
  if (lastUndoActionHasSaveAfter()) {
    if ([self ask:"You are about to undo past the last point this file was saved.  Do you want to do this?"
    no:"Undo" yes:"Don’t Undo"]) {
      [self launchRenderingThreads:RasterSubPixels];
      return; }}

  //  Pull the data that needs to be restored.
  pullUndo(&type,&data,&text[0]);

  //  Restore the data to the user interface.
  [self restoreUndoDataToUI:type data:data text:&text[0]];

  if (!RenderThreads) [self launchRenderingThreads:RasterSubPixels]; }



//  This method is invoked when the user chooses “Redo” from the menu bar.

- (IBAction) handleRedo:(id) sender {

  int4   text[TEXT_FIELD_MAX+1] ;
  short  type, data ;

  //  Exit if there is nothing to redo.
  if (!RedoCount) return;

  [self killRenderingThreads];

  //  Pull the data that needs to be restored.
  pullRedo(&type,&data,&text[0]);

  //  Restore the data to the user interface.
  [self restoreUndoDataToUI:type data:data text:&text[0]];

  if (!RenderThreads) [self launchRenderingThreads:RasterSubPixels]; }



//  Used by handleUndo and handleRedo to put the data into the user interface.
//
//  Important:  The string of int4s at “text” will be altered.

- (void) restoreUndoDataToUI:(short) type data:(short) data text:(int4 *) text {

  int4  txtA[TEXT_FIELD_MAX+1] ;
  int4  txtB[TEXT_FIELD_MAX+1] ;
  int4  txtC[TEXT_FIELD_MAX+1] ;

  //  Radio-button options.
  if (type==UNDO_OPT_SCREENSIZE      ) {
    [ScreenSizeRadioButtons selectCellAtRow:(int4) data column:0]; [self setScreenSize:nil]; [TabView selectLastTabViewItem:nil];
    if (!getScreenWidth(data) && is3strings(text)) {
      copy1stringTo3(&txtA[0],&txtB[0],&txtC[0],text,TEXT_FIELD_MAX);
      restoreField                             (      WidthField,txtA,&      WidthFieldContents[0],NO ,TabView,self              );
      restoreField                             (     HeightField,txtB,&     HeightFieldContents[0],NO ,TabView,self              );
      restoreField                             ( HeightUsedField,txtC,& HeightUsedFieldContents[0],NO ,TabView,self              ); }
    return; }
  if (type==UNDO_OPT_FRAMERATE       ) {
    [FrameRateRadioButtons selectCellAtRow:(int4) data column:0]; [self setFrameRate:nil]; [TabView selectLastTabViewItem:nil];
    if (!getFrameRate(data))       restoreField(  FrameRateField,text,&  FrameRateFieldContents[0],NO ,TabView,self              );
    return; }
  if (type== UNDO_CHECK_STARFIELDAUTO) {
    [StarfieldAutoCheckbox setState:(int4) data];
    return; }

  //  Text-field contents.
  if (type==UNDO_TEXT_DISTRIBUTOR) restoreField(DistributorField,text,&DistributorFieldContents[0],YES,TabView,self              );
  if (type==UNDO_TEXT_DISTINFO   ) restoreField(   DistInfoField,text,&   DistInfoFieldContents[0],YES,TabView,self              );
  if (type==UNDO_TEXT_FILMCO     ) restoreField(     FilmCoField,text,&     FilmCoFieldContents[0],YES,TabView,self              );
  if (type==UNDO_TEXT_FILMCOINC  ) restoreField(  FilmCoIncField,text,&  FilmCoIncFieldContents[0],YES,TabView,self              );
  if (type==UNDO_TEXT_INTRO      ) restoreView (      IntroField,text,&      IntroFieldContents[0],YES,TabView,self,ControlWindow);
  if (type==UNDO_TEXT_TITLE      ) restoreField(      TitleField,text,&      TitleFieldContents[0],YES,TabView,self              );
  if (type==UNDO_TEXT_CRAWL      ) restoreView (      CrawlField,text,&      CrawlFieldContents[0],YES,TabView,self,ControlWindow);
  if (type==UNDO_TEXT_WIDTH      ) restoreField(      WidthField,text,&      WidthFieldContents[0],NO ,TabView,self              );
  if (type==UNDO_TEXT_HEIGHT     ) restoreField(     HeightField,text,&     HeightFieldContents[0],NO ,TabView,self              );
  if (type==UNDO_TEXT_HEIGHTUSED ) restoreField( HeightUsedField,text,& HeightUsedFieldContents[0],NO ,TabView,self              );
  if (type==UNDO_TEXT_FRAMERATE  ) restoreField(  FrameRateField,text,&  FrameRateFieldContents[0],NO ,TabView,self              );
  if (type==UNDO_TEXT_DESTINATION) restoreField(DestinationField,text,&DestinationFieldContents[0],NO ,TabView,self              );
  if (type==UNDO_TEXT_STARFIELD  ) restoreField(  StarfieldField,text,&  StarfieldFieldContents[0],NO ,TabView,self              );

  //  Allow image window to react to input-focus change.
  [self checkForFieldFocusChanges]; }



//  This method is invoked by the NSNotification system (as requested by this app on app launch), and is also called by this app’s own code.
//
//  The parameter “note” is ignored.

- (void) userContentChanged:(NSNotification *) note {

  int4    imageWid, imageHei, imageHeiUsed, newText[TEXT_FIELD_MAX+1] ;
  bool    errFound=NO ;
  NSRect  oldWindRect ;
  double  val, val2   ;

  [self killRenderingThreads];

  //  Takes note that changes to user content need to be saved.
  ChangesSaved=NO;

  //  Discover and react to changes in the distributor name.
  copyTextUtf8ToLongStr(&newText[0],(BYTE *) [[DistributorField stringValue] UTF8String],MAX_CHARS_DISTRIBUTOR);
  prepDistributorText  (&newText[0]);
  if (!int4StrEqual(&DistributorText[0],&newText[0])) {
    ContentChanged=YES; copyTextLong(&DistributorText[0],&newText[0],MAX_CHARS_DISTRIBUTOR);
    forceRasterRect(YES,YES,
    (int4) screenToPixelX(DISTRIBUTOR_LOGO_RECT_L)  ,
    (int4) screenToPixelY(DISTRIBUTOR_LOGO_RECT_T)-1,
    (int4) screenToPixelX(DISTRIBUTOR_LOGO_RECT_R)+1,
    (int4) screenToPixelY(DISTRIBUTOR_LOGO_RECT_B)  ); }

  //  Discover and react to changes in the distributor-info text.
  copyTextUtf8ToLongStr(&newText[0],(BYTE *) [[DistInfoField stringValue] UTF8String],MAX_CHARS_DIST_INFO);
  prepDistInfoText     (&newText[0]);
  if (!int4StrEqual(&DistInfoText[0],&newText[0])) {
    ContentChanged=YES;
    forceRasterDistInfo(YES,YES); copyTextLong(&DistInfoText[0],&newText[0],MAX_CHARS_DIST_INFO); [self canFailApp:prepareDistInfoChars()];
    forceRasterDistInfo(YES,YES); }

  //  Discover and react to changes in the film-company name.
  copyTextUtf8ToLongStr(&newText[0],(BYTE *) [[FilmCoField stringValue] UTF8String],MAX_CHARS_FILM_CO);
  prepFilmCoText       (&newText[0]);
  if (!int4StrEqual(&FilmCoText[0],&newText[0])) {
    ContentChanged=YES; copyTextLong(&FilmCoText[0],&newText[0],MAX_CHARS_FILM_CO);
    forceRasterFilmCo(YES,YES); }

  //  Discover and react to changes in the film-company inc text.
  copyTextUtf8ToLongStr(&newText[0],(BYTE *) [[FilmCoIncField stringValue] UTF8String],MAX_CHARS_FILM_CO_INC);
  prepFilmCoIncText    (&newText[0]);
  if (!int4StrEqual(&FilmCoIncText[0],&newText[0])) {
    ContentChanged=YES; copyTextLong(&FilmCoIncText[0],&newText[0],MAX_CHARS_FILM_CO_INC);
    forceRasterRect(YES,YES,
    (int4) screenToPixelX(                                         BaseArcFrillCapX)-2,
    (int4) screenToPixelY(                     FILMCO_ARC_CENTER_Y+BaseArcFrillRad )-2,
    (int4) screenToPixelX(                                        -BaseArcFrillCapX)+2,
    (int4) screenToPixelY(FILMCO_TEXT_BOTTOM -(FILMCO_ARC_CENTER_Y+BaseArcFrillRad
    -                     FILMCO_TEXT_BOTTOM)* FILMCO_DECENDER_FRAC                )+2); }

  //  Discover and react to changes in the intro text.
  copyTextUtf8ToLongStr(&newText[0],(BYTE *) [[IntroField string] UTF8String],MAX_CHARS_INTRO);
  if (!int4StrEqual(&IntroText[0],&newText[0])) {
    ContentChanged=YES; forceRasterIntro(YES,YES); copyTextLong(&IntroText[0],&newText[0],MAX_CHARS_INTRO); }

  //  Discover and react to changes in the title.
  copyTextUtf8ToLongStr(&newText[0],(BYTE *) [[TitleField stringValue] UTF8String],MAX_CHARS_TITLE);
  prepTitleText(&newText[0]);
  if (!int4StrEqual(&TitleText[0],&newText[0])) {
    ContentChanged=YES; copyTextLong(&TitleText[0],&newText[0],MAX_CHARS_TITLE); }

  //  Discover and react to changes in the crawl text.
  copyTextUtf8ToLongStr(&newText[0],(BYTE *) [[CrawlField string] UTF8String],MAX_CHARS_CRAWL);
  if (!int4StrEqual(&CrawlText[0],&newText[0])) {
    ContentChanged=YES; forceRasterCrawl(YES,YES); copyTextLong(&CrawlText[0],&newText[0],MAX_CHARS_CRAWL); }

  //  Validate the contents of the numeric textfields.
  //
  //  Important:  See the comment at definition of SequenceTimes concerning
  //              maximum frame duration (i.e. minimum frame rate).
  val=[ WidthField doubleValue]; imageWid=(int4) val;
  if (val<(double) IMAGE_WID_MIN
  ||  val>(double) IMAGE_WID_MAX ||  val!=(int4) val) {
    if (!errFound) {
      [ErrorMessage setStringValue:[NSString stringWithFormat:[NSString stringWithCString:
      "The “Width” field must contain an integer ranging from %u to %u."
      encoding:NSMacOSRomanStringEncoding],IMAGE_WID_MIN,IMAGE_WID_MAX]]; }
    [GoButton     setEnabled    : NO]; errFound=YES; imageWid    =0; }
  val=[HeightField doubleValue]; imageHei=(int4) val;
  if (val<(double) IMAGE_HEI_MIN
  ||  val>(double) IMAGE_HEI_MAX ||  val!=(int4) val) {
    if (!errFound) {
      [ErrorMessage setStringValue:[NSString stringWithFormat:[NSString stringWithCString:
      "The “Height” field must contain an integer ranging from %u to %u."
      encoding:NSMacOSRomanStringEncoding],IMAGE_HEI_MIN,IMAGE_HEI_MAX]]; }
    [GoButton     setEnabled    : NO]; errFound=YES; imageHei    =0; }
  val2=[HeightUsedField doubleValue]; imageHeiUsed=(int4) val2;
  if (val2<(double) IMAGE_HEI_USED_MIN
  ||  val2>(double) IMAGE_HEI_MAX      || val2>val || val2!=(int4) val2) {
    if (!errFound) {
      [ErrorMessage setStringValue:[NSString stringWithFormat:[NSString stringWithCString:
      "The “Height Used” field must contain an integer ranging from %u to %u, and must be no larger than the “Height.”"
      encoding:NSMacOSRomanStringEncoding],IMAGE_HEI_USED_MIN,IMAGE_HEI_MAX]]; }
    [GoButton     setEnabled    : NO]; errFound=YES; imageHeiUsed=0; }
  val=[FrameRateField doubleValue];
  if (val<(double) MOVIE_FRAME_RATE_MIN
  ||  val>(double) MOVIE_FRAME_RATE_MAX) {
    if (!errFound) {
      [ErrorMessage setStringValue:[NSString stringWithFormat:[NSString stringWithCString:
      "The “Frame Rate” field must contain a value ranging from %u.0 to %u.0."
      encoding:NSMacOSRomanStringEncoding],MOVIE_FRAME_RATE_MIN,MOVIE_FRAME_RATE_MAX]]; }
    [GoButton     setEnabled    : NO]; errFound=YES; }
  else {
    FramesPerSecond=val; }
  if (!errFound) {
    [GoButton     setEnabled    :YES];
    [ErrorMessage setStringValue:@""]; }

  //  Discover and react to changes to the image size.
  if (imageWid && imageHei && imageHeiUsed) {
    if (imageHeiUsed!=ImageHeiUsed) {
      setImageHeiUsed(imageHeiUsed); ContentChanged=YES;
      forceRasterRect(YES,YES,0,0,imageWid,imageHei); }
    if (imageWid!=ImageWid
    ||  imageHei!=ImageHei) {
      nullRaster(YES,YES);
      setImageWid(imageWid);
      setImageHei(imageHei);
      //  Hide the image window.
      [ImageWindow orderOut:nil];
      //  Resize the image window, trying to preserve the screen position of its upper-left corner.
      oldWindRect=[ImageWindow frame]; [TheImageView setImage:nil];
      [ImageWindow setContentSize      :NSMakeSize (imageWid,imageHei)];
      [ImageWindow setFrameTopLeftPoint:NSMakePoint(oldWindRect.origin.x,oldWindRect.origin.y+oldWindRect.size.height)];
      //  Resize the graphic image.
      if (TheImage  ) {
        [ TheImage   release]; TheImage  =nil; }
      if (PreviewBitmap) {
        [ PreviewBitmap release]; PreviewBitmap=nil; }
      [TheImageView setFrame:NSMakeRect(0,0,imageWid,imageHei)];
      createBitmap(); [PreviewBitmap getBitmapDataPlanes:&Planes[0]];
      createImage (); [TheImageView             setImage:TheImage  ];
      //  Show the image window.
      [ImageWindow orderFront:nil];
      //  Recreate the SubStar arrays.
      createSubStarRanges();
      //  (finish)
      forceRasterWholeImage(YES,YES); }}

  //  Restart image rendering, if the user-entered content on which it is based has changed.
  if (ContentChanged || !UserContentChanged_EverCalled) {
    #if GREEN_CYAN_DIAGNOSTIC
    fillImage(0,255,0); ImageNeedsUpdate=YES;
    #endif
    ContentChanged=NO;
    if (WhichPreview==DISTRIBUTOR_LOGO || !UserContentChanged_EverCalled) {
      UserContentChanged_EverCalled=YES;
      prepareToDrawDistributorImage(
      &Planes[0],&BackdropPlanes[0],(int4) [PreviewBitmap bytesPerRow],
      &TowerPlanes[0],&TowerMaskPlanes[0],&SearchlightMaskPlanes[0],TowerBitmapWid,TowerBitmapHei,
      (int4) [          TowerBitmap bytesPerRow],
      (int4) [      TowerMaskBitmap bytesPerRow],
      (int4) [SearchlightMaskBitmap bytesPerRow],
      (int4) [       BackdropBitmap bytesPerRow]);
      setDistributorFade(1.); setSearchlightAngles(DISTRIBUTOR_PREVIEW_TIME); }
    else if (WhichPreview==FILMCO_LOGO) {
      prepareToDrawFilmCoImage(&Planes[0],(int4) [PreviewBitmap bytesPerRow],1.); FilmCoFade=1.; }
    else if (WhichPreview== INTRO_TEXT) {
      [self canFailApp:prepareToDrawIntroImage(&Planes[0],(int4) [PreviewBitmap bytesPerRow])];
      IntroFade=1.; forceRasterIntro(YES,YES); }
    else if (WhichPreview== TITLE_LOGO) {
      prepareToDrawTitleImage                 (&Planes[0],(int4) [PreviewBitmap bytesPerRow]) ;
      TitleDist=TITLE_PREVIEW_DIST; TitleOpacity=1.; CrawlOpacity=0.; CrawlBottomEdge=0.;
      forceRasterTitlePreview(YES,YES); }
    else if (WhichPreview== TEXT_CRAWL) {
      [self canFailApp:prepareToDrawCrawlImage(&Planes[0],(int4) [PreviewBitmap bytesPerRow])];
      TitleDist=1.; TitleOpacity=0.; CrawlOpacity=1.; CrawlBottomEdge=calcCrawlBottom();
      forceRasterCrawl(YES,YES); }}

  [self launchRenderingThreads:OVERSCAN_LO]; }



//  Called by NSTextFieldWithFocusDetection’s and NSTextViewWithFocusDetection’s
//  becomeFirstResponder methods, and by this file’s userContentChanged method.

- (void) checkForFieldFocusChanges {

  bool  newFocus=NO ;

  //  Until the disclaimer is agreed, refuse to run this function.  (This protection is needed because
  //  this method is a delgate method for text-field subclasses, and therefore can be fired before
  //  handleDisclaimerAgreed has had a chance to set up vital data structures.)
  if (!DisclaimerAgreed) return;

  //  If a full-sequence render is underway, changes in field focus are ignored.
  if (WhichPreview==GENERATING) return;

  //  React to a change of textfield focus, which affects which preview image needs to be rendered.
  if      (WhichPreview!=DISTRIBUTOR_LOGO && (hasFocus(DistributorField) || hasFocus( DistInfoField))) {
    [self killRenderingThreads]; newFocus=YES;
    prepareToDrawDistributorImage(
    &Planes[0],&BackdropPlanes[0],(int4) [PreviewBitmap bytesPerRow],
    &TowerPlanes[0],&TowerMaskPlanes[0],&SearchlightMaskPlanes[0],TowerBitmapWid,TowerBitmapHei,
    (int4) [          TowerBitmap bytesPerRow],
    (int4) [      TowerMaskBitmap bytesPerRow],
    (int4) [SearchlightMaskBitmap bytesPerRow],
    (int4) [       BackdropBitmap bytesPerRow]); setDistributorFade(1.); setSearchlightAngles(DISTRIBUTOR_PREVIEW_TIME);
    WhichPreview        =DISTRIBUTOR_LOGO; forceRasterWholeImage(YES,YES); }
  else if (WhichPreview!=     FILMCO_LOGO && (hasFocus(     FilmCoField) || hasFocus(FilmCoIncField))) {
    [self killRenderingThreads]; newFocus=YES;
    prepareToDrawFilmCoImage(&Planes[0],(int4) [PreviewBitmap bytesPerRow],1.); FilmCoFade=1.;
    WhichPreview        =     FILMCO_LOGO; nullRaster(YES,YES); fillImage(0,0,0); ImageNeedsUpdate=YES; forceRasterFilmCo(YES,YES); }
  else if (WhichPreview!=      INTRO_TEXT &&  hasFocus(      IntroField)                             ) {
    [self killRenderingThreads]; newFocus=YES;
    [self canFailApp:prepareToDrawIntroImage(&Planes[0],(int4) [PreviewBitmap bytesPerRow])]; IntroFade=1.;
    WhichPreview        =      INTRO_TEXT; nullRaster(YES,YES); fillImage(0,0,0); ImageNeedsUpdate=YES; forceRasterIntro (YES,YES); }
  else if (WhichPreview!=      TITLE_LOGO &&  hasFocus(      TitleField)                             ) {
    [self killRenderingThreads]; newFocus=YES;
    prepareToDrawTitleImage(&Planes[0],(int4) [PreviewBitmap bytesPerRow]);
    TitleDist=TITLE_PREVIEW_DIST; TitleOpacity=1.; CrawlOpacity=0.; CrawlBottomEdge=0.;
    WhichPreview        =      TITLE_LOGO; nullRaster(YES,YES); fillImage(0,0,0); ImageNeedsUpdate=YES; forceRasterTitlePreview(YES,YES); }
  else if (WhichPreview!=      TEXT_CRAWL &&  hasFocus(      CrawlField)                             ) {
    [self killRenderingThreads]; newFocus=YES;
    [self canFailApp:prepareToDrawCrawlImage(&Planes[0],(int4) [PreviewBitmap bytesPerRow])];
    TitleDist=1.; TitleOpacity=0.; CrawlOpacity=1.; CrawlBottomEdge=calcCrawlBottom();
    WhichPreview        =      TEXT_CRAWL; nullRaster(YES,YES); fillImage(0,0,0); ImageNeedsUpdate=YES; forceRasterCrawl (YES,YES); }

  #if GREEN_CYAN_DIAGNOSTIC
  fillImage(0,255,0); ImageNeedsUpdate=YES;
  #endif

  if (newFocus) [self launchRenderingThreads:OVERSCAN_LO]; }



//  This function is automatically invoked when the user clicks the screen-size
//  radio button set (even if they click the already-selected radio button).
//
//  Note:  If this method is called with a nil sender, it will still work, but
//         will not update the Undo array.

- (IBAction) setScreenSize:(id) sender {

  int4  selRow=[ScreenSizeRadioButtons selectedRow] ;
  int4  text[TEXT_FIELD_MAX+1] ;

  //  Do nothing if the chosen radio button was already selected.
  if (selRow==ScreenSizeRadioButtonsSelectedRow) return;

  //  Handle the new selection.
  if (sender) {
    copy3stringsTo1(&text[0],&WidthFieldContents[0],&HeightFieldContents[0],&HeightUsedFieldContents[0],TEXT_FIELD_MAX);
    pushUndo(UNDO_OPT_SCREENSIZE,ScreenSizeRadioButtonsSelectedRow,selRow,text,nil); }
  ScreenSizeRadioButtonsSelectedRow=selRow;
  if (getScreenWidth(selRow)>0L) {
    [     WidthField setIntValue:getScreenWidth     (selRow)];
    [    HeightField setIntValue:getScreenHeight    (selRow)];
    [HeightUsedField setIntValue:getScreenHeightUsed(selRow)]; CustomImageSize=NO; }
  else {
    [     WidthField setStringValue:@""];
    [    HeightField setStringValue:@""];
    [HeightUsedField setStringValue:@""]; CustomImageSize=YES; }
  [       WidthField setEditable:CustomImageSize];
  [      HeightField setEditable:CustomImageSize];
  [  HeightUsedField setEditable:CustomImageSize]; [self userContentChanged:nil];
  copyTextUtf8ToLongStr(&     WidthFieldContents[0],(BYTE *) [[     WidthField stringValue] UTF8String],TEXT_FIELD_MAX);
  copyTextUtf8ToLongStr(&    HeightFieldContents[0],(BYTE *) [[    HeightField stringValue] UTF8String],TEXT_FIELD_MAX);
  copyTextUtf8ToLongStr(&HeightUsedFieldContents[0],(BYTE *) [[HeightUsedField stringValue] UTF8String],TEXT_FIELD_MAX); }



//  This function is automatically invoked when the user clicks the screen-size
//  radio button set (even if they click the already-selected radio button).
//
//  Note:  If this method is called with a nil sender, it will still work, but
//         will not update the Undo array.

- (IBAction) setFrameRate:(id) sender {

  int4  selRow=[FrameRateRadioButtons selectedRow] ;

  //  Do nothing if the chosen radio button was already selected.
  if (selRow==FrameRateRadioButtonsSelectedRow) return;

  //  Handle the new selection.
  if (sender) pushUndo(UNDO_OPT_FRAMERATE,FrameRateRadioButtonsSelectedRow,selRow,&FrameRateFieldContents[0],nil);
  FrameRateRadioButtonsSelectedRow=selRow;
  if (getFrameRate(selRow)>0) {
    [FrameRateField setDoubleValue:getFrameRate(selRow)]; CustomFrameRate=NO;  }
  else {
    [FrameRateField setStringValue:@""]; CustomFrameRate=YES; }
  [  FrameRateField setEditable:CustomFrameRate]; [self userContentChanged:nil];
  copyTextUtf8ToLongStr(&FrameRateFieldContents[0],(BYTE *) [[FrameRateField stringValue] UTF8String],TEXT_FIELD_MAX); }



//  This function is automatically invoked when the user clicks the starfield “Auto” checkbox.
//
//  Note:  If this method is called with a nil sender, it will still work, but will not update the Undo array.

- (IBAction) setStarfieldAuto:(id) sender {

  int4  checkboxVal =[StarfieldAutoCheckbox state] ;
  if   (checkboxVal!=1) checkboxVal=0;

  //  Handle the new selection.
  if (sender) pushUndo(UNDO_CHECK_STARFIELDAUTO,1-checkboxVal,checkboxVal,&StarfieldFieldContents[0],nil);
  [StarfieldField        setEditable:(checkboxVal==0)]; AutoStarfield=(checkboxVal==1);
  [StarfieldChooseButton setEnabled :(checkboxVal==0)]; [self userContentChanged:nil]; }



//  This function is invoked when the user clicks the choose-location button next to the destination field.

- (IBAction) chooseFramesDestinationFolder:(id) sender {

  NSOpenPanel  *thePanel=[NSOpenPanel openPanel] ;

  //  Initialize location-selection panel’s parameters.
  [thePanel setCanChooseDirectories   :                         YES];
  [thePanel setCanChooseFiles         :                          NO];
  [thePanel setAllowsMultipleSelection:                          NO];
  [thePanel setTitle                  :@"Choose Frames Destination"];

  //  Run the modal panel and store the result in the text field (if the user didn’t cancel the panel).
  if ([thePanel runModalForTypes:nil]==NSOKButton) {
    [DestinationField setStringValue:[[thePanel filenames] objectAtIndex:0]]; }}



//  This function is invoked when the user clicks the choose-file button next to the starfield field.

- (IBAction) chooseStarfieldFile:(id) sender {

  NSOpenPanel  *thePanel=[NSOpenPanel openPanel] ;

  //  Initialize location-selection panel’s parameters.
  [thePanel setCanChooseDirectories   :                       NO];
  [thePanel setCanChooseFiles         :                      YES];
  [thePanel setAllowsMultipleSelection:                       NO];
  [thePanel setTitle                  :@"Choose Starfield Image"];

  //  Run the modal panel and store the result in the text field (if the user didn’t cancel the panel).
  if ([thePanel runModalForTypes:[NSImage imageFileTypes]]==NSOKButton) {
    [StarfieldField setStringValue:[[thePanel filenames] objectAtIndex:0]]; }}



//  This is used as a wrapper around many model (i.e. C) function calls that can return error messages.
//
//  If this method is passed an errMessage (i.e. one that is both non-nil and not empty), it does not
//  return -- instead it force-quits the app with a modal display of the errMessage.
//
//  Note:  If this method closes the app, it very probably leaves allocated objects hanging in memory.
//         It should not be used for normal quitting.

- (void) canFailApp:(BYTE *) errMessage {
  if (errMessage && (*errMessage)) {
    [self killRenderingThreads];
    NSRunCriticalAlertPanel(@"Fatal Error",[NSString stringWithCString:errMessage encoding:NSMacOSRomanStringEncoding],nil,nil,nil);
    [NSApp terminate:nil]; }}



//  This method does not return -- instead it force-quits the app with a modal display of the errMessage string.
//
//  Note:  This method very probably leaves allocated objects hanging in memory.  It should be used only for
//         emergency failing of the app; not for normal quitting.

- (void) failApp:(BYTE *) errMessage {
  [self killRenderingThreads]; ChangesSaved=YES; WhichPreview=NONE;
  NSRunCriticalAlertPanel(@"Fatal Error",[NSString stringWithCString:errMessage encoding:NSMacOSRomanStringEncoding],nil,nil,nil);
  [NSApp terminate:nil]; }



//  Displays a modal warning message to the user.

- (void) alert:(BYTE *) alertMessage {

  bool  rendering=(RenderThreads>0) ;

  if (rendering) [self   killRenderingThreads];
  NSRunAlertPanel(@"Alert",[NSString stringWithCString:alertMessage encoding:NSMacOSRomanStringEncoding],nil,nil,nil);
  if (rendering) [self launchRenderingThreads:RasterSubPixels]; }



//  Presents a modal dialog to the user asking a question for which there are two possible answers.  Returns
//  the user’s answer as a int4 integer -- whatever value was returned by NSRunAlertPanel, which this function
//  calls.
//
//  Important:  The texts passed into this method for “textNO” and “textYES” do not necessarily correspond to
//              the yes-or-no answer to the question in “alertMessage”.  Only the *actual text* in textNO and
//              textYES indicate what the answers to the question are.  textNO means just that this method
//              returns NO if the user clicks that button, or YES if the user clicks the textYES button.
//
//  Note:  The textNO button is hilited as the default button, and will respond to the Return key.

- (int4) ask:(BYTE *) alertMessage no:(BYTE *) textNO yes:(BYTE *) textYES {

  bool  answer, rendering=(RenderThreads>0) ;

  [self killRenderingThreads];

  answer=!NSRunAlertPanel(@"Alert",
  [NSString stringWithCString:alertMessage encoding:NSMacOSRomanStringEncoding],
  [NSString stringWithCString:textNO       encoding:NSMacOSRomanStringEncoding],
  [NSString stringWithCString:textYES      encoding:NSMacOSRomanStringEncoding],nil);

  if (rendering) [self launchRenderingThreads:RasterSubPixels];

  return answer; }



//  Similar to the “ask” method, but presents three possible answers to the user, instead of just two.
//
//  If the user clicks the answer specified by “text0”, this method will return a value of 0; etc.

- (int4) ask3:(BYTE *) alertMessage zero:(BYTE *) text0 one:(BYTE *) text1 two:(BYTE *) text2 {

  bool  rendering=(RenderThreads>0) ;
  int4  answer ;

  [self killRenderingThreads];

  answer=NSRunAlertPanel(@"Alert",
  [NSString stringWithCString:alertMessage encoding:NSMacOSRomanStringEncoding],
  [NSString stringWithCString:text0        encoding:NSMacOSRomanStringEncoding],
  [NSString stringWithCString:text1        encoding:NSMacOSRomanStringEncoding],
  [NSString stringWithCString:text2        encoding:NSMacOSRomanStringEncoding]);

  if (rendering) [self launchRenderingThreads:RasterSubPixels];

  return answer; }



//  Creates an OS image object for the rear-searchlight-mask bitmap.

- (void) createSearchlightMaskImage {

  NSBundle  *theBundle=[NSBundle mainBundle] ;

  SearchlightMaskData=[NSData dataWithContentsOfFile:[theBundle pathForResource:@"Searchlight Mask" ofType:@"bmp"]];
  if (!SearchlightMaskData) [self failApp:"Searchlight Mask failed to load."];
  [SearchlightMaskData retain]; }



//  Creates OS image objects for the front searchlight tower.

- (void) createTowerImages {

  NSBundle  *theBundle=[NSBundle mainBundle] ;

  TowerData    =[NSData dataWithContentsOfFile:[theBundle pathForResource:@"Tower"      ofType:@"bmp"]];
  TowerMaskData=[NSData dataWithContentsOfFile:[theBundle pathForResource:@"Tower Mask" ofType:@"bmp"]];
  if (!TowerData    ) [self failApp:    "Tower failed to load."];
  if (!TowerMaskData) [self failApp:"TowerMask failed to load."];
  [TowerData     retain];
  [TowerMaskData retain]; }



//  Replaces special characters in a string, so that it can be used in XML-like data without confusing the tag structure.

- (NSString *) xmlSafe:(NSString *) string {

  int4            str[TEXT_FIELD_MAX+1], *newStr ;
  unsigned int4   len=0, newLen=0, i=0, j=0      ;
  NSString       *returnStr ;

  //  Get the Unicode text of the passed-in NSString, as a string of int4s.
  copyTextUtf8ToLongStr(&str[0],(BYTE *) [string UTF8String],TEXT_FIELD_MAX);

  //  Determine the current length of the string, and the length of the not-yet-created, XML-safe string.
  while (str[len]) {
    if  (str[len]=='<' || str[len]=='>') newLen+=3;
    if  (str[len]=='&'                 ) newLen+=4;
    len++; newLen++; }

  //  Allocate space for the new, XML-safe string.
  newStr=malloc((newLen+1)*sizeof(int4)); if (!newStr) [self failApp:"Out of memory in xmlSafe."];

  //  Copy the string to the new, XML-safe string, modifying it in the process.
  while     (str[i]) {
    if      (str[i]=='<') {
      newStr[j++]='&';
      newStr[j++]='l';
      newStr[j++]='t';
      newStr[j++]=';'; }
    else if (str[i]=='>') {
      newStr[j++]='&';
      newStr[j++]='g';
      newStr[j++]='t';
      newStr[j++]=';'; }
    else if (str[i]=='&') {
      newStr[j++]='&';
      newStr[j++]='a';
      newStr[j++]='m';
      newStr[j++]='p';
      newStr[j++]=';'; }
    else {
      newStr[j++]=str[i]; }
    i++; }
  newStr[j]=0; if (j!=newLen) [self failApp:"Logic error detected in xmlSafe."];

  //  Get return value, cleanup, and exit.
  convertLongStrToUtf8(newStr); returnStr=[NSString stringWithUTF8String:(BYTE *) newStr]; free(newStr); return returnStr; }



//  Load the Distributor Info cbf (font).

- (void) loadCbfDistInfo {

  NSBundle  *theBundle=[NSBundle mainBundle] ;

  DistInfoFontData=[NSData dataWithContentsOfFile:[theBundle pathForResource:@"Font (DistInfo)" ofType:@"dat"]];
  if (!DistInfoFontData) [self failApp:"Font (DistInfo) failed to load."];
  [DistInfoFontData retain];
  if (!correctBitmapFontEndianness((BYTE *) [DistInfoFontData bytes],(int4) [DistInfoFontData length])) {
    [self failApp:"Failed to correct endianness of compressed bitmap font in loadCbfDistInfo."]; }
  correctBitmapFontMissingGlyphs  ((BYTE *) [DistInfoFontData bytes]);

  //  Set global pointer to font data.
  CbfDistInfo=(BYTE *) [DistInfoFontData bytes]; }



//  Finds out if the user is trying to store hundreds of image files in a location
//  called “desktop”.  If so, announces that the user may not do that, and returns NO.

- (bool) desktopWarning {

  BYTE  folderName[9], *path=(BYTE *) [[DestinationField stringValue] UTF8String] ;
  int4  i=0L, j ;

  if (!path) [self failApp:"Unable to acquire destination path in desktopWarning."];

  while (path[i]            ) i++;   //  (find the end of the path)
  while (i && path[i-1]=='/') i--;   //  (ignore any trailing slashes)
  if (i<8) return YES;               //  (exit if the path is too short to be the desktop)

  //  Copy a piece of the path and make it lowercase.  (This code conveniently
  //  ignores the possibility of multi-byte characters; if such occur in these
  //  last eight bytes, then the path cannot be the desktop.)
  for (j=0; j<8; j++) {
    folderName[j]=path[i-8+j]; charToUpperCaseAlphaOnly(&folderName[j]); }
  folderName  [j]=0;

  //  Exit if the folder is not the desktop.
  if (!strEqual(&folderName[0],"/DESKTOP")) return YES;

  //  Inform the user that this may not be done.
  [self alert:"You may not store hundreds of image files in a location called “Desktop”.  Please choose another destination."]; return NO; }



//  Load the Intro cbf (font).

- (void) loadCbfIntro {

  NSBundle  *theBundle=[NSBundle mainBundle] ;

  IntroFontData=[NSData dataWithContentsOfFile:[theBundle pathForResource:@"Font (Intro)" ofType:@"dat"]];
  if (!IntroFontData) [self failApp:"Font (Intro) failed to load."];
  [IntroFontData retain];
  if (!correctBitmapFontEndianness((BYTE *) [IntroFontData bytes],(int4) [IntroFontData length])) {
    [self failApp:"Failed to correct endianness of compressed bitmap font in loadCbfIntro."]; }
  correctBitmapFontMissingGlyphs  ((BYTE *) [IntroFontData bytes]);

  //  Set global pointer to font data.
  CbfIntro=(BYTE *) [IntroFontData bytes]; }



//  Load the Crawl cbf (font).

- (void) loadCbfCrawl {

  NSBundle  *theBundle=[NSBundle mainBundle] ;

  CrawlFontData=[NSData dataWithContentsOfFile:[theBundle pathForResource:@"Font (Crawl)" ofType:@"dat"]];
  if (!CrawlFontData) [self failApp:"Font (Crawl) failed to load."];
  [CrawlFontData retain];
  if (!correctBitmapFontEndianness((BYTE *) [CrawlFontData bytes],(int4) [CrawlFontData length])) {
    [self failApp:"Failed to correct endianness of compressed bitmap font in loadCbfCrawl."]; }
  correctBitmapFontMissingGlyphs  ((BYTE *) [CrawlFontData bytes]);

  //  Set global pointer to font data.
  CbfCrawl=(BYTE *) [CrawlFontData bytes]; }



//  Load the custom starfield image.  If this fails for any reason, it will display an explanatory alert dialog, then return NO to the calling code.

- (bool) loadCustomStarfield {

  NSData            *tiffData, *bmpData ;
  unsigned int4      bmpDataOffSet      ;
  NSBitmapImageRep  *tempBitmap         ;
  NSImage           *tempImage          ;
  NSString          *filePath           ;
  BYTE              *bmpBytes           ;
  int4               i, j               ;

  //  Get the path of the file.
  filePath=[StarfieldField stringValue];

  //  Load the image file into an image object.
  tempImage=[NSImage alloc];
  if (!tempImage) {
    [self alert:"Unable to create image structure; OS may be running out of memory."             ];                      return NO; }
  if (![tempImage initWithContentsOfFile:filePath]) {
    [self alert:"Unable to load the starfield image.  Choose another image or select “Auto”."    ];                      return NO; }//...really doesn’t have to be released??

  //  Get a TIFF image representation from the image.
  tiffData=[tempImage TIFFRepresentationUsingCompression:NSTIFFCompressionNone factor:1];
  if (!tiffData) {
    [self alert:"Unable to process the starfield image; OS may be running out of memory.  (a)"   ]; [tempImage release]; return NO; }

  //  Turn the TIFF into a bitmap image representation.
  tempBitmap=[NSBitmapImageRep imageRepWithData:tiffData];
  if (!tempBitmap) {
    [self alert:"Unable to process the starfield image; OS may be running out of memory.  (b)"   ]; [tempImage release]; return NO; }

  //  Turn the bitmap image representation into a BMP structure.
  bmpData=[tempBitmap representationUsingType:NSBMPFileType properties:nil];
  if (!bmpData) {
    [self alert:"Unable to process the starfield image; OS may be running out of memory.  (c)"   ]; [tempImage release]; return NO; }

  //  Extract the RGB pixel data from the BMP structure.  (For details of the BMP format
  //  used here, see:  http://fortunecity.com/skyscraper/windows/364/bmpffrmt.html )
  bmpBytes=(BYTE *) [bmpData bytes];
  if (!bmpBytes) {
    [self alert:"Unable to process the starfield image; OS may be running out of memory.  (d)"   ]; [tempImage release]; return NO; }
  if (bmpBytes[0]!='B'
  ||  bmpBytes[1]!='M') {
    [self alert:"Unable to load image; OS’s NSBMPFileType did not start with “BM”."              ]; [tempImage release]; return NO; }
  bmpDataOffSet=getULongLittleEndian(&bmpBytes[10]);
  CustomStarfieldWid=getULongLittleEndian(&bmpBytes[18]); if (CustomStarfieldWid<0) CustomStarfieldWid*=-1;//...does negative indicate row order?  try both kinds of BMP files with SWTSG
  CustomStarfieldHei=getULongLittleEndian(&bmpBytes[22]); if (CustomStarfieldHei<0) CustomStarfieldHei*=-1;
  //...here, disallow image if it is too large
  if (bmpBytes[28]!=24
  ||  bmpBytes[29]    ) {
    [self alert:"Unable to load image; OS’s NSBmpFileType did not have 24-bit color."            ]; [tempImage release]; return NO; }
  if (getULongLittleEndian (&bmpBytes[30])) {
    [self alert:"Unable to load image; OS’s NSBmpFileType was not sans-compression as requested."]; [tempImage release]; return NO; }
  if (!createStarfieldBitmap(&CustomStarfieldBitmap,CustomStarfieldWid,CustomStarfieldHei)) {
    [self alert:"Unable to process the starfield image; OS may be running out of memory.  (e)"   ]; [tempImage release]; return NO; }
  if (!CustomStarfieldBitmap) {
    [self alert:"Unable to process the starfield image; OS may be running out of memory.  (f)"   ]; [tempImage release]; return NO; }
  [CustomStarfieldBitmap getBitmapDataPlanes:&CustomStarfieldBitmapPlanes[0]];
  CustomStarfieldRowBytes=forceDiv(CustomStarfieldWid,16);
  for   (j=0; j<CustomStarfieldHei; j++) {
    for (i=0; i<CustomStarfieldWid; i++) {
      CustomStarfieldBitmapPlanes[0][j*CustomStarfieldRowBytes+i]=bmpBytes[bmpDataOffSet+3L*(j*CustomStarfieldWid+i)+2L];
      CustomStarfieldBitmapPlanes[1][j*CustomStarfieldRowBytes+i]=bmpBytes[bmpDataOffSet+3L*(j*CustomStarfieldWid+i)+1L];
      CustomStarfieldBitmapPlanes[2][j*CustomStarfieldRowBytes+i]=bmpBytes[bmpDataOffSet+3L*(j*CustomStarfieldWid+i)+0L]; }}

  //  Release the temporary objects.
  [tempImage release];

  //  Success.
  return YES; }



@end



//  Create the bitmap for the custom starfield image (replacing any pre-existing bitmap).

bool createStarfieldBitmap(NSBitmapImageRep **bitmap, int4 wid, int4 hei) {

  //  Kill any pre-existing bitmap.
  if (*bitmap) {
    [(*bitmap) release]; *bitmap=nil; }

  *bitmap=[NSBitmapImageRep alloc]; if (!(*bitmap)) return NO;

  [(*bitmap)
  initWithBitmapDataPlanes:nil
  pixelsWide              :wid
  pixelsHigh              :hei
  bitsPerSample           :  8
  samplesPerPixel         :  3
  hasAlpha                : NO
  isPlanar                :YES
  colorSpaceName          :NSDeviceRGBColorSpace
  bytesPerRow             :forceDiv(wid,16)
  bitsPerPixel            :  8];//...is this correct, or should it be 24?  (also in Monochroma)

  //  Success.
  return YES; }



//  Display a frame number preded by the word “Frame”.

void showStatusFrame(NSTextField *field, int4 frame) {
  [field setStringValue:[NSString stringWithFormat:@"Frame %u",frame]]; }



//  Convience function used for calculating how far the render has progressed into the current stage.  (Note that the
//  current stage is determined by the value of RenderStage -- see the enumeration of its values in definitions.h.)

double stageFrac() {
  return (MovieTime-StageTime)/(StageTimeNext-StageTime); }



//  Create main bitmap for the preview image.

void createBitmap() {

  PreviewBitmapWid=ImageWid;
  PreviewBitmapHei=ImageHei;
  PreviewBitmap   =[NSBitmapImageRep alloc];

  [PreviewBitmap
  initWithBitmapDataPlanes:nil
  pixelsWide              :PreviewBitmapWid
  pixelsHigh              :PreviewBitmapHei
  bitsPerSample           :  8
  samplesPerPixel         :  3
  hasAlpha                : NO
  isPlanar                :YES
  colorSpaceName          :NSDeviceRGBColorSpace
  bytesPerRow             :forceDiv(PreviewBitmapWid,16L)
  bitsPerPixel            :  8]; }



//  Create the bitmap for the Distributor backdrop image.

void createBackdropBitmap(BYTE *bmp) {

  int4  *sizePtr=(int4 *) &bmp[BMP_SIZE_LOC] ;

  BackdropBitmap        =[NSBitmapImageRep alloc];
  DistributorBackdropWid=(int4) getULongLittleEndian((BYTE *) &sizePtr[0]);
  DistributorBackdropHei=(int4) getULongLittleEndian((BYTE *) &sizePtr[1]);

  [BackdropBitmap
  initWithBitmapDataPlanes:nil
  pixelsWide              :DistributorBackdropWid
  pixelsHigh              :DistributorBackdropHei
  bitsPerSample           :  8
  samplesPerPixel         :  3
  hasAlpha                : NO
  isPlanar                :YES
  colorSpaceName          :NSDeviceRGBColorSpace
  bytesPerRow             :forceDiv(DistributorBackdropWid,16)
  bitsPerPixel            :  8]; }



//  Create the bitmap for the Distributor image’s front searchlight tower.

void createTowerBitmap() {

  TowerBitmapWid= 64;
  TowerBitmapHei=144;
  TowerBitmap   =[NSBitmapImageRep alloc];

  [TowerBitmap
  initWithBitmapDataPlanes:nil
  pixelsWide              :TowerBitmapWid
  pixelsHigh              :TowerBitmapHei
  bitsPerSample           :  8
  samplesPerPixel         :  3
  hasAlpha                : NO
  isPlanar                :YES
  colorSpaceName          :NSDeviceRGBColorSpace
  bytesPerRow             :forceDiv(TowerBitmapWid,16L)
  bitsPerPixel            :  8]; }



//  Create the bitmap for the Distributor image’s front searchlight tower.
//
//  Warning:  Must call the function createTowerBitmap before calling this function.

void createTowerMaskBitmap() {

  TowerMaskBitmap=[NSBitmapImageRep alloc];

  [TowerMaskBitmap
  initWithBitmapDataPlanes:nil
  pixelsWide              :TowerBitmapWid
  pixelsHigh              :TowerBitmapHei
  bitsPerSample           :  8
  samplesPerPixel         :  1
  hasAlpha                : NO
  isPlanar                :YES
  colorSpaceName          :NSDeviceBlackColorSpace
  bytesPerRow             :forceDiv(TowerBitmapWid,16L)
  bitsPerPixel            :  8]; }



//  Create the bitmap for the Distributor image’s rear-searchlight mask.

void createSearchlightMaskBitmap() {

  SearchlightMaskBitmap=[NSBitmapImageRep alloc];

  [SearchlightMaskBitmap
  initWithBitmapDataPlanes:nil
  pixelsWide              :DistributorBackdropWid
  pixelsHigh              :DistributorBackdropHei
  bitsPerSample           :  8
  samplesPerPixel         :  1
  hasAlpha                : NO
  isPlanar                :YES
  colorSpaceName          :NSDeviceBlackColorSpace
  bytesPerRow             :forceDiv(DistributorBackdropWid,16)
  bitsPerPixel            :  8]; }



//  Release extra image buffers used in the Film Company full-sequence (not preview) render.

void releaseFilmCoBuffers() {
  if (FilmCoBufferGreen ) free(FilmCoBufferGreen );
  if (FilmCoBufferBlue  ) free(FilmCoBufferBlue  );
  if (FilmCoBufferOrange) free(FilmCoBufferOrange);
  if (FilmCoBufferYellow) free(FilmCoBufferYellow);
  FilmCoBufferGreen =nil;  CrawlBufferStars=nil;       StarsImageReady=NO;
  FilmCoBufferBlue  =nil; DistributorBuffer=nil; DistributorImageReady=NO;
  FilmCoBufferOrange=nil;       IntroBuffer=nil;       IntroImageReady=NO;
  FilmCoBufferYellow=nil; }



//  Release the bell-curve arrays used in the Film Company full-sequence (not preview) render.

void releaseBellCurveArrays() {
  if (FilmCoBellH) free(FilmCoBellH);
  if (FilmCoBellV) free(FilmCoBellV);
  FilmCoBellH=nil;
  FilmCoBellV=nil; }



//  Create an OS image structure for the main, preview image.

void createImage() {

  //  Create a copy of the bitmap.
  if (PreviewBitmapCopy) [PreviewBitmapCopy release];
  PreviewBitmapCopy=[PreviewBitmap copy];

  //  Even though PreviewBitmap was only copied, not changed, these statements are necessary to allow the app to work correctly in OS X 10.6 (Snow Leopard).
  [PreviewBitmap getBitmapDataPlanes:&Planes[0]]; PreviewRgb=&Planes[0];

  //  Create the image, and assign the bitmap copy to it.
  TheImage=[NSImage new]; [TheImage addRepresentation:PreviewBitmapCopy]; }



//  Create an OS image structure for the Distributor backdrop image.

void createDistributorBackdropImage(id controller) {

  NSBundle  *theBundle=[NSBundle mainBundle] ;

  DistBackdropData=[NSData dataWithContentsOfFile:[theBundle pathForResource:@"TCF Backdrop" ofType:@"bmp"]];
  if (!DistBackdropData) [controller failApp:"TCF Backdrop failed to load."];
  [DistBackdropData retain]; }



//  Used by restoreUndoDataToUI to restore the contents of one text field.
//
//  Note:  This is very similar to restoreView, except that it operates on an NSTextField, not an NSTextView.
//
//  Important:  The string of int4s pointed to by “text” is altered by this function.

void restoreField(NSTextField *field, int4 *text, int4 *contents, bool firstTab, NSTabView *tabs, id controller) {
  copyTextLong(contents,text,TEXT_FIELD_MAX);
  convertLongStrToUtf8(text); [field setStringValue:[NSString stringWithUTF8String:(BYTE *) text]];
  if (firstTab) [tabs selectFirstTabViewItem:nil];
  else          [tabs selectLastTabViewItem :nil];
  [field selectText:nil]; [controller userContentChanged:nil]; }



//  Used by restoreUndoDataToUI to restore the contents of one text field.
//
//  Note:  This is very similar to restoreField, except that it operates on an NSTextView, not an NSTextField.
//
//  Important:  The string of int4s pointed to by “text” is altered by this function.

void restoreView(NSTextView *field, int4 *text, int4 *contents, bool firstTab, NSTabView *tabs, id controller, NSWindow *wind) {
  copyTextLong(contents,text,TEXT_FIELD_MAX);
  convertLongStrToUtf8(text); [field setString:[NSString stringWithUTF8String:(BYTE *) text]];
  if (firstTab) [tabs selectFirstTabViewItem:nil];
  else          [tabs selectLastTabViewItem :nil];
  [field selectAll:nil]; [wind makeFirstResponder:field]; [controller userContentChanged:nil]; }



//  Convenience function used by controlTextDidChange to create an entry in the Undo array.
//
//  Note:  This is identical to makeViewUndo, except that it operates from an NSTextField, not an NSTextView.

void makeFieldUndo(NSTextField *field, short type, int4 *contents) {

  int4  newText[TEXT_FIELD_MAX+1] ;

  copyTextUtf8ToLongStr(newText,(BYTE *) [[field stringValue] UTF8String],TEXT_FIELD_MAX);
  pushUndo(type,0,0,contents,newText);      copyTextLong(contents,newText,TEXT_FIELD_MAX); }



//  Convenience function used by textDidChange to create an entry in the Undo array.
//
//  Note:  This is identical to makeFieldUndo, except that it operates from an NSTextView, not an NSTextField.

void makeViewUndo(NSTextView *field, short type, int4 *contents) {

  int4  newText[TEXT_FIELD_MAX+1] ;

  copyTextUtf8ToLongStr(newText,(BYTE *) [[field string] UTF8String],TEXT_FIELD_MAX);
  pushUndo(type,0,0,contents,newText); copyTextLong(contents,newText,TEXT_FIELD_MAX); }



//  Determines whether a specified text field (or text view) has input focus.

bool hasFocus(id theField) {
  return   [[[theField window] firstResponder] isKindOfClass:[NSTextView class]]
  &&        [[theField window] fieldEditor:NO forObject:nil]!=nil
  && ( (id) [[theField window] firstResponder]          ==theField
  ||  [(id) [[theField window] firstResponder] delegate]==theField); }



//  Pass this function the amount of time (in seconds) that the Title logo has been present, and it returns
//  the distance that the logo should be from the camera.
//
//  This function is designed to make the Title logo start with zero velocity, accelerate up to TITLE_SPEED
//  over a time of TITLE_ACCEL_TIME, then to stop accelerating and continue moving away at a constant speed of
//  TITLE_SPEED.
//
//  Note:  During the acceleration phase, the velocity of the logo is determined by this formula:
//
//           velocity  =  (TITLE_SPEED / TITLE_ACCEL_TIME) * time
//
//         Since titleDistByTime returns distance, not velocity, it must use the reverse-derivative (integral)
//         of the above formula (arbitrarily augmented with TITLE_START_DIST), which is:
//
//           distance  =  TITLE_START_DIST  +  (.5 * TITLE_SPEED / TITLE_ACCEL_TIME) * (time squared)

double titleDistByTime(double time) {

  double  accelTime, constVelTime ;

  //  Separate the passed-in time into acceleration time and constant-speed time.
  if    (time<TITLE_ACCEL_TIME) {
    accelTime=            time; constVelTime=                   0.; }
  else {
    accelTime=TITLE_ACCEL_TIME; constVelTime=time-TITLE_ACCEL_TIME; }

  //  Calculate the distance, based on two different formulas.
  return TITLE_START_DIST+.5*TITLE_SPEED/TITLE_ACCEL_TIME*accelTime*accelTime   //  (     acceleration formula)
  +      TITLE_SPEED*constVelTime; }                                            //  (constant velocity formula)



//  Used by the method updateImageAndAdvanceSequence to set various Title/Crawl-related control variables.

void setCrawlState(int4 renderStage, double stageFrac, id controller) {

  //  Error checking.
  if (stageFrac<0. || stageFrac>1.) [controller failApp:"Invalid stageFrac passed to setCrawlState()."];

  //  Set the crawl state.
  if (renderStage==          TITLE_PULL_BACK) {
    if (!StarsImageReady && AutoStarfield) {
      TitleDist=1.; TitleOpacity=0.; }
    else {
      TitleDist=titleDistByTime(stageFrac*getSequenceTime(TITLE_PULL_BACK)); TitleOpacity=1.; }
    CrawlBottomEdge=0.;
    CrawlOpacity   =0.; }
  if (renderStage==CRAWL_AND_TITLE_PULL_BACK) {
    CrawlOpacity   =1.;
    TitleDist=
    titleDistByTime(getSequenceTime(TITLE_PULL_BACK          )
    +     stageFrac*getSequenceTime(CRAWL_AND_TITLE_PULL_BACK));
    TitleOpacity   =1.;
    CrawlBottomEdge=-stageFrac*getSequenceTime(CRAWL_AND_TITLE_PULL_BACK)*CrawlSpeed; }
  if (renderStage==CRAWL_AND_TITLE_FADE_DOWN) {
    CrawlOpacity   =1.;
    TitleDist=
    titleDistByTime(getSequenceTime(TITLE_PULL_BACK          )
    +               getSequenceTime(CRAWL_AND_TITLE_PULL_BACK)
    +     stageFrac*getSequenceTime(CRAWL_AND_TITLE_FADE_DOWN));
    TitleOpacity   =1.-stageFrac;
    CrawlBottomEdge=-          getSequenceTime(CRAWL_AND_TITLE_PULL_BACK)*CrawlSpeed
    -                stageFrac*getSequenceTime(CRAWL_AND_TITLE_FADE_DOWN)*CrawlSpeed; }
  if (renderStage==CRAWL_TO_ALL_TEXT_VISIBLE) {
    CrawlOpacity   =1.; TitleDist=1.; TitleOpacity=0.;
    CrawlBottomEdge=-          getSequenceTime(CRAWL_AND_TITLE_PULL_BACK)*CrawlSpeed
    -                          getSequenceTime(CRAWL_AND_TITLE_FADE_DOWN)*CrawlSpeed
    -                stageFrac*getSequenceTime(CRAWL_TO_ALL_TEXT_VISIBLE)*CrawlSpeed; }
  if (renderStage==CRAWL_OFF_TO_DISTANCE) {
    CrawlOpacity   =1.; TitleDist=1.; TitleOpacity=0.;
    CrawlBottomEdge=-          getSequenceTime(CRAWL_AND_TITLE_PULL_BACK)*CrawlSpeed
    -                          getSequenceTime(CRAWL_AND_TITLE_FADE_DOWN)*CrawlSpeed
    -                          getSequenceTime(CRAWL_TO_ALL_TEXT_VISIBLE)*CrawlSpeed
    -                stageFrac*getSequenceTime(CRAWL_OFF_TO_DISTANCE    )*CrawlSpeed; }
  if (renderStage==CRAWL_FADE_DOWN) {
    CrawlOpacity   =1.-stageFrac; TitleDist=1.; TitleOpacity=0.;
    CrawlBottomEdge=-          getSequenceTime(CRAWL_AND_TITLE_PULL_BACK)*CrawlSpeed
    -                          getSequenceTime(CRAWL_AND_TITLE_FADE_DOWN)*CrawlSpeed
    -                          getSequenceTime(CRAWL_TO_ALL_TEXT_VISIBLE)*CrawlSpeed
    -                          getSequenceTime(CRAWL_OFF_TO_DISTANCE    )*CrawlSpeed
    -                stageFrac*getSequenceTime(CRAWL_FADE_DOWN          )*CrawlSpeed; }
  if (renderStage==JUST_STARS) {
    CrawlOpacity=0.; TitleDist=1.; TitleOpacity=0.; }}



//  Builds a BMP file of the current image, then write it to a file.
//
//  For BMP format details used here, see:  http://fortunecity.com/skyscraper/windows/364/bmpffrmt.html
//
//  Returns error description, or nil if everything went fine.

BYTE *createAndWriteBmpFile(NSString *folderPath, int4 frameNumber) {

  BYTE            buff[IMAGE_WID_MAX*3L+3L], *planeR, *planeG, *planeB, fileName[21] ;
  int4            j, frameCalc, fileNameI, rowBytes=(int4) [PreviewBitmap bytesPerRow]  ;
  unsigned int4   i, k, rowLen, fileSize ;
  NSString       *filePath ;
  NSFileHandle   *theFile  ;
  NSData         *rowData  ;

  //  Verify that the destination folder exists.
  if (![[NSFileManager defaultManager] fileExistsAtPath:[folderPath stringByExpandingTildeInPath]]) {
    return "The destination folder does not exist."; }

  //  Calculate the row length and file size.
  rowLen  =   (ImageWid*3L+3L)/4L; rowLen*=4L;
  fileSize=54L+ImageHei*rowLen+2L;

  //  Get an NSData object for the data buffer.
  rowData=[NSData dataWithBytesNoCopy:&buff[0] length:rowLen freeWhenDone:NO];
  if (!rowData) return "Unable to allocate NSData object when writing file.";

  //  Construct the file name (which will be appended to the folder path).
  copyText(&fileName[0],"/Frame0000000000.bmp",sizeof(fileName)/sizeof(fileName[0])-1);
  fileNameI=15; frameCalc=RenderFrame;
  while (frameCalc) {
    fileName[fileNameI--]='0'+frameCalc%10; frameCalc/=10; }

  //  Create the (empty) file.
  filePath=[folderPath stringByAppendingString:[NSString stringWithUTF8String:&fileName[0]]];
  if (![[NSFileManager defaultManager] createFileAtPath:[filePath stringByExpandingTildeInPath]
  contents:nil attributes:nil]) {
    return "Unable to create file."; }

  //  Open the file for writing.
  theFile=[NSFileHandle fileHandleForWritingAtPath:[filePath stringByExpandingTildeInPath]];
  if (!theFile) return "Unable to open file for writing.";

  //  Construct and write the BMP header.
  for (i=0L; i<54; i++) buff[i]='\0';
  buff[ 0]='B';
  buff[ 1]='M';
  buff[10]= 54;
  buff[14]= 40;
  buff[26]=  1;
  buff[28]= 24;
  putULongLittleEndian(&buff[ 2],fileSize);
  putULongLittleEndian(&buff[18],(unsigned int4) ImageWid);
  putULongLittleEndian(&buff[22],(unsigned int4) ImageHei);
  [theFile writeData:[NSData dataWithBytes:&buff[0] length:54]];

  //  Construct the image data one row at a time, and write it to the file.
  for (j=ImageHei-1; j>=0; j--) {
    planeR=&Planes[0][j*rowBytes];
    planeG=&Planes[1][j*rowBytes];
    planeB=&Planes[2][j*rowBytes]; k=0L;
    for (i=0; i<ImageWid; i++) {
      buff[k++]=planeB[i];
      buff[k++]=planeG[i];
      buff[k++]=planeR[i]; }
    while (k<rowLen) buff[k++]='\0';
    [theFile writeData:rowData]; }

  //  Close the file.
  [theFile closeFile];

  //  Success -- return no error message.
  return nil; }



//  Used by the functions parseSwtsgFile and fileHasXmlHeader to read one byte from a user-content file.
//
//  Returns one byte of the file, or a null byte if the file ran out of data.  (If the file actually
//  contains null bytes, then this function will simply return them.  The file shouldn’t contain them.)

BYTE getByte(NSFileHandle *theFile) {

  NSData  *theData ;

  theData=[theFile readDataOfLength:1];
  if (![theData length]) return '\0';
  return *(BYTE *) [theData bytes]; }



//  Determines whether the specified file starts with the XML header, which is:
//    <?xml version="1.0" encoding="UTF-8"?>
//
//  This function plays a little fast-and-loose, which allows normal flexibility in
//  the spacing and parameter ordering of the header -- but also theoretically could
//  cause a very rare file to be counted as XML even though it didsn’t really have
//  the correct header, just something that weirdly resembled it.
//
//  Returns NO if unable to find the header -- for any reason.
//
//  Note:  This function is used by parseSwtsgFile to determine whether a user file
//         is to be interpreted as UTF-8 or as plain, ASCII-like Mac Roman text.
//         The function getxattr is *not* used, even though saveUserContent does use
//         setxattr to let other apps know that the file is UTF-8 encoded.

bool fileHasXmlHeader(NSString *xmlFilePath) {

  BYTE            c, *utf8tag="<?xml encoding=UTF-8?" ;
  NSFileHandle   *theFile ;
  unsigned int4   i=0     ;

  //  Open the file.
  theFile=[NSFileHandle fileHandleForReadingAtPath:xmlFilePath]; if (!theFile) return NO;

  //  Determine whether the file starts with the XML header.
  c=getByte(theFile);
  while (c && c!='>') {
    if (c==utf8tag[i] && utf8tag[i]) i++;
    c=getByte(theFile); }

  //  Close the file.
  [theFile closeFile];

  return c && !utf8tag[i]; }



//  Parse an XML user-content file.  (Also recognizes pre-1.1.5 files that are not true XML nor UTF-8.)
//
//  This XML-parsing function safely ignores unrecognized tags, but may be confused if those tags
//  are not structured correctly.  Returns NO if something looks wrong (in which case the buffers
//  itemA, itemB, etc. may have been changed!).
//
//  Passed-in parameters:
//    NSString *xmlFilePath = Full path+name to file containing XML that needs to be read and parsed.
//    BYTE     *AnItem      = The array where you want AnItem stored.
//    int4      maxAnItem   = Max AnItem length including terminator -- if =4, AnItem will receive at
//                               most 3 chars plus the terminator byte (0).
//
//  Returns an error-description string, or nil (not an empty string) if everything went fine.
//
//  Note:  valueI is initialized only to prevent a compiler warning -- it shouldn’t need to be.

BYTE *parseSwtsgFile(
NSString *xmlFilePath,
int4 *ioDistributor     , int4 maxDistributor     ,
int4 *ioDistributorInfo , int4 maxDistributorInfo ,
int4 *ioFilmCompany     , int4 maxFilmCompany     ,
int4 *ioFilmCompanyInc  , int4 maxFilmCompanyInc  ,
int4 *ioIntro           , int4 maxIntro           ,
int4 *ioTitle           , int4 maxTitle           ,
int4 *ioCrawl           , int4 maxCrawl           ,
int4 *ioScreen          , int4 maxScreen          ,
int4 *ioStarfieldAuto   , int4 maxStarfieldAuto   ,
int4 *ioScreenWidth     , int4 maxScreenWidth     ,
int4 *ioScreenHeight    , int4 maxScreenHeight    ,
int4 *ioScreenHeightUsed, int4 maxScreenHeightUsed,
int4 *ioFrameRate       , int4 maxFrameRate       ,
int4 *ioFrameRateFps    , int4 maxFrameRateFps    ,
int4 *ioDestination     , int4 maxDestination     ,
int4 *ioStarfield       , int4 maxStarfield       ) {

  //  Set up the parsing variables.  (Note:  These can be in any order; it’s just
  //  a list of things that need to be noticed wherever they occur.)
  //
  //  Important:  If you change these, be sure to adjust the value of MAX_TERM_LEN
  //              if the lengthiest scan term is lengthier than it was before.
  //
  //  Note:  These scan-terms must be straight C strings, not UTF-8.  (However,
  //         the file may contain UTF-8 text inbetween its XML tags, provided
  //         that file begins with the standard XML header.)
  //
  BYTE *scanTerm[]={
    "<!--", "-->", "&amp;", "&lt;", "&gt;",
    "<StarWarsTSG>"     , "</StarWarsTSG>"     ,
    "<Distributor>"     , "</Distributor>"     ,
    "<DistributorInfo>" , "</DistributorInfo>" ,
    "<FilmCompany>"     , "</FilmCompany>"     ,
    "<FilmCompanyInc>"  , "</FilmCompanyInc>"  ,
    "<Intro>"           , "</Intro>"           ,
    "<Title>"           , "</Title>"           ,
    "<Crawl>"           , "</Crawl>"           ,
    "<Screen>"          , "</Screen>"          ,
    "<StarfieldAuto>"   , "</StarfieldAuto>"   ,
    "<ScreenWidth>"     , "</ScreenWidth>"     ,
    "<ScreenHeight>"    , "</ScreenHeight>"    ,
    "<ScreenHeightUsed>", "</ScreenHeightUsed>",
    "<FrameRate>"       , "</FrameRate>"       ,
    "<FrameRateFps>"    , "</FrameRateFps>"    ,
    "<Destination>"     , "</Destination>"     ,
    "<Starfield>"       , "</Starfield>"         }
  ;
  #define        MAX_TERM_LEN  19   //  (length of "</ScreenHeightUsed>", not including its terminator)
  BYTE           c, value[TEXT_FIELD_MAX+MAX_TERM_LEN+1], foundTerm[MAX_TERM_LEN+1], closeTerm[MAX_TERM_LEN+1] ;
  int4           i, state=0, scanPos[sizeof(scanTerm)/sizeof(scanTerm[0])], valueI=0 ;
  int4           termCount          =sizeof(scanTerm)/sizeof(scanTerm[0]) ;
  bool           inComment=NO, isUtf8=fileHasXmlHeader(xmlFilePath) ;
  NSFileHandle  *theFile ;

  *ioDistributor     =0; for (i=0; i<termCount; i++) scanPos[i]=0;
  *ioDistributorInfo =0;
  *ioFilmCompany     =0;
  *ioFilmCompanyInc  =0;
  *ioIntro           =0;
  *ioCrawl           =0;
  *ioScreen          =0;
  *ioStarfieldAuto   =0;
  *ioScreenWidth     =0;
  *ioScreenHeight    =0;
  *ioScreenHeightUsed=0;
  *ioFrameRate       =0;
  *ioFrameRateFps    =0;
  *ioDestination     =0;
  *ioStarfield       =0;

  //  Ensure backward compatibility with older versions of the app that didn’t feature the Title or StarfieldAuto fields.
  copyTextUtf8ToLongStr(ioTitle        ,"Star Wars",maxTitle        );
  copyTextUtf8ToLongStr(ioStarfieldAuto,"1"        ,maxStarfieldAuto);

  //  Open the file.
  theFile=[NSFileHandle fileHandleForReadingAtPath:xmlFilePath];
  if (!theFile) return "Unable to open file for reading.";

  //  Parse the XML-like data.
  c=getByte(theFile);
  while (c) {
    foundTerm[0]='\0';
    for (i=0; i<termCount; i++) {
      if (c!=scanTerm[i][scanPos[i]]) scanPos[i]=0;
      if (c==scanTerm[i][scanPos[i]]) {   //  (“else” is intentionally not used in this line!)
        scanPos[i]++;
        if (!scanTerm[i][scanPos[i]]) {
          copyText(&foundTerm[0],scanTerm[i],MAX_TERM_LEN); scanPos[i]=0; }}}
    if (inComment) {
      if    (strEqual(&foundTerm[0],"-->" )) inComment= NO; }
    else if (strEqual(&foundTerm[0],"<!--")) inComment=YES;
    else if (state==0) {   //  (seeking set)
      if    (strEqual(&foundTerm[0],"<StarWarsTSG>")) state++; }
    else if (state==1) {   //  (seeking item or end-of-set)
      if    (strEqual(&foundTerm[0], "<Distributor>"     )) {
        copyText     (&closeTerm[0],"</Distributor>"     ,MAX_TERM_LEN); state++; valueI=0; }
      if    (strEqual(&foundTerm[0], "<DistributorInfo>" )) {
        copyText     (&closeTerm[0],"</DistributorInfo>" ,MAX_TERM_LEN); state++; valueI=0; }
      if    (strEqual(&foundTerm[0], "<FilmCompany>"     )) {
        copyText     (&closeTerm[0],"</FilmCompany>"     ,MAX_TERM_LEN); state++; valueI=0; }
      if    (strEqual(&foundTerm[0], "<FilmCompanyInc>"  )) {
        copyText     (&closeTerm[0],"</FilmCompanyInc>"  ,MAX_TERM_LEN); state++; valueI=0; }
      if    (strEqual(&foundTerm[0], "<Intro>"           )) {
        copyText     (&closeTerm[0],"</Intro>"           ,MAX_TERM_LEN); state++; valueI=0; }
      if    (strEqual(&foundTerm[0], "<Title>"           )) {
        copyText     (&closeTerm[0],"</Title>"           ,MAX_TERM_LEN); state++; valueI=0; }
      if    (strEqual(&foundTerm[0], "<Crawl>"           )) {
        copyText     (&closeTerm[0],"</Crawl>"           ,MAX_TERM_LEN); state++; valueI=0; }
      if    (strEqual(&foundTerm[0], "<Screen>"          )) {
        copyText     (&closeTerm[0],"</Screen>"          ,MAX_TERM_LEN); state++; valueI=0; }
      if    (strEqual(&foundTerm[0], "<StarfieldAuto>"   )) {
        copyText     (&closeTerm[0],"</StarfieldAuto>"   ,MAX_TERM_LEN); state++; valueI=0; }
      if    (strEqual(&foundTerm[0], "<ScreenWidth>"     )) {
        copyText     (&closeTerm[0],"</ScreenWidth>"     ,MAX_TERM_LEN); state++; valueI=0; }
      if    (strEqual(&foundTerm[0], "<ScreenHeight>"    )) {
        copyText     (&closeTerm[0],"</ScreenHeight>"    ,MAX_TERM_LEN); state++; valueI=0; }
      if    (strEqual(&foundTerm[0], "<ScreenHeightUsed>")) {
        copyText     (&closeTerm[0],"</ScreenHeightUsed>",MAX_TERM_LEN); state++; valueI=0; }
      if    (strEqual(&foundTerm[0], "<FrameRate>"       )) {
        copyText     (&closeTerm[0],"</FrameRate>"       ,MAX_TERM_LEN); state++; valueI=0; }
      if    (strEqual(&foundTerm[0], "<FrameRateFps>"    )) {
        copyText     (&closeTerm[0],"</FrameRateFps>"    ,MAX_TERM_LEN); state++; valueI=0; }
      if    (strEqual(&foundTerm[0], "<Destination>"     )) {
        copyText     (&closeTerm[0],"</Destination>"     ,MAX_TERM_LEN); state++; valueI=0; }
      if    (strEqual(&foundTerm[0], "<Starfield>"       )) {
        copyText     (&closeTerm[0],"</Starfield>"       ,MAX_TERM_LEN); state++; valueI=0; }
      if    (strEqual(&foundTerm[0],"</StarWarsTSG>"     ))              state--; }
    else if (state==2) {   //  (sucking item data, and seeking end-of-item)
      if (valueI<sizeof(value)/sizeof(value[0])-1) value[valueI++]=c;
      if (strEqual(&foundTerm[0],"&amp;")) {
        valueI-=4;                       }
      if (strEqual(&foundTerm[0],"&lt;" )) {
        valueI-=3; value[valueI-1]= '<'; }
      if (strEqual(&foundTerm[0],"&gt;" )) {
        valueI-=3; value[valueI-1]= '>'; }
      if (strEqual(&foundTerm[0],&closeTerm[0])) {
        //  Strip the close-term off the captured value.  (This is correct even if valueI maxed out during data-sucking.)
        i=0; while (closeTerm[i++]) valueI--;
        //  Terminate the value.
        value[valueI]='\0';
        //  Copy the captured value into the appropriate buffer.
        if (isUtf8) {
          if      (strEqual(&closeTerm[0],"</Distributor>"     )) copyTextUtf8ToLongStr        (ioDistributor     ,&value[0],maxDistributor     );
          else if (strEqual(&closeTerm[0],"</DistributorInfo>" )) copyTextUtf8ToLongStr        (ioDistributorInfo ,&value[0],maxDistributorInfo );
          else if (strEqual(&closeTerm[0],"</FilmCompany>"     )) copyTextUtf8ToLongStr        (ioFilmCompany     ,&value[0],maxFilmCompany     );
          else if (strEqual(&closeTerm[0],"</FilmCompanyInc>"  )) copyTextUtf8ToLongStr        (ioFilmCompanyInc  ,&value[0],maxFilmCompanyInc  );
          else if (strEqual(&closeTerm[0],"</Intro>"           )) copyTextUtf8ToLongStr        (ioIntro           ,&value[0],maxIntro           );
          else if (strEqual(&closeTerm[0],"</Title>"           )) copyTextUtf8ToLongStr        (ioTitle           ,&value[0],maxTitle           );
          else if (strEqual(&closeTerm[0],"</Crawl>"           )) copyTextUtf8ToLongStr        (ioCrawl           ,&value[0],maxCrawl           );
          else if (strEqual(&closeTerm[0],"</Screen>"          )) copyTextUtf8ToLongStr        (ioScreen          ,&value[0],maxScreen          );
          else if (strEqual(&closeTerm[0],"</StarfieldAuto>"   )) copyTextUtf8ToLongStr        (ioStarfieldAuto   ,&value[0],maxStarfieldAuto   );
          else if (strEqual(&closeTerm[0],"</ScreenWidth>"     )) copyTextUtf8ToLongStr        (ioScreenWidth     ,&value[0],maxScreenWidth     );
          else if (strEqual(&closeTerm[0],"</ScreenHeight>"    )) copyTextUtf8ToLongStr        (ioScreenHeight    ,&value[0],maxScreenHeight    );
          else if (strEqual(&closeTerm[0],"</ScreenHeightUsed>")) copyTextUtf8ToLongStr        (ioScreenHeightUsed,&value[0],maxScreenHeightUsed);
          else if (strEqual(&closeTerm[0],"</FrameRate>"       )) copyTextUtf8ToLongStr        (ioFrameRate       ,&value[0],maxFrameRate       );
          else if (strEqual(&closeTerm[0],"</FrameRateFps>"    )) copyTextUtf8ToLongStr        (ioFrameRateFps    ,&value[0],maxFrameRateFps    );
          else if (strEqual(&closeTerm[0],"</Destination>"     )) copyTextUtf8ToLongStr        (ioDestination     ,&value[0],maxDestination     );
          else if (strEqual(&closeTerm[0],"</Starfield>"       )) copyTextUtf8ToLongStr        (ioStarfield       ,&value[0],maxStarfield       ); }
        else {   //  (This “else” is to correctly read pre-v1.1.5 files that aren’t UTF-8.)
          if      (strEqual(&closeTerm[0],"</Distributor>"     )) convertMacStrToUnicodeLongStr(ioDistributor     ,&value[0],maxDistributor     );
          else if (strEqual(&closeTerm[0],"</DistributorInfo>" )) convertMacStrToUnicodeLongStr(ioDistributorInfo ,&value[0],maxDistributorInfo );
          else if (strEqual(&closeTerm[0],"</FilmCompany>"     )) convertMacStrToUnicodeLongStr(ioFilmCompany     ,&value[0],maxFilmCompany     );
          else if (strEqual(&closeTerm[0],"</FilmCompanyInc>"  )) convertMacStrToUnicodeLongStr(ioFilmCompanyInc  ,&value[0],maxFilmCompanyInc  );
          else if (strEqual(&closeTerm[0],"</Intro>"           )) convertMacStrToUnicodeLongStr(ioIntro           ,&value[0],maxIntro           );
          else if (strEqual(&closeTerm[0],"</Title>"           )) convertMacStrToUnicodeLongStr(ioTitle           ,&value[0],maxTitle           );
          else if (strEqual(&closeTerm[0],"</Crawl>"           )) convertMacStrToUnicodeLongStr(ioCrawl           ,&value[0],maxCrawl           );
          else if (strEqual(&closeTerm[0],"</Screen>"          )) convertMacStrToUnicodeLongStr(ioScreen          ,&value[0],maxScreen          );
          else if (strEqual(&closeTerm[0],"</StarfieldAuto>"   )) convertMacStrToUnicodeLongStr(ioStarfieldAuto   ,&value[0],maxStarfieldAuto   );
          else if (strEqual(&closeTerm[0],"</ScreenWidth>"     )) convertMacStrToUnicodeLongStr(ioScreenWidth     ,&value[0],maxScreenWidth     );
          else if (strEqual(&closeTerm[0],"</ScreenHeight>"    )) convertMacStrToUnicodeLongStr(ioScreenHeight    ,&value[0],maxScreenHeight    );
          else if (strEqual(&closeTerm[0],"</ScreenHeightUsed>")) convertMacStrToUnicodeLongStr(ioScreenHeightUsed,&value[0],maxScreenHeightUsed);
          else if (strEqual(&closeTerm[0],"</FrameRate>"       )) convertMacStrToUnicodeLongStr(ioFrameRate       ,&value[0],maxFrameRate       );
          else if (strEqual(&closeTerm[0],"</FrameRateFps>"    )) convertMacStrToUnicodeLongStr(ioFrameRateFps    ,&value[0],maxFrameRateFps    );
          else if (strEqual(&closeTerm[0],"</Destination>"     )) convertMacStrToUnicodeLongStr(ioDestination     ,&value[0],maxDestination     );
          else if (strEqual(&closeTerm[0],"</Starfield>"       )) convertMacStrToUnicodeLongStr(ioStarfield       ,&value[0],maxStarfield       ); }
        //  Step back out of the “sucking item data” state, and continue reading the file.
        state--; }}
    c=getByte(theFile); }

  //  Close the file.
  [theFile closeFile];

  //  Return an error message if anything went wrong.
  if (state) return "Unable to find parsable contents in file.";

  //  Success -- return no error.
  return nil; }
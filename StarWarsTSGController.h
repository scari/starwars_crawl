//  Star Wars TSG
//  Darel Rex Finley, 2006-2011

#import "definitions.h"

#import <Cocoa/Cocoa.h>

@interface StarWarsTSGController : NSObject

{
  IBOutlet NSTabView           *                TabView;
  IBOutlet NSTextField         *       DistributorField;
  IBOutlet NSTextField         *          DistInfoField;
  IBOutlet NSTextField         *            FilmCoField;
  IBOutlet NSTextField         *         FilmCoIncField;
  IBOutlet NSTextView          *             IntroField;
  IBOutlet NSTextField         *             TitleField;
  IBOutlet NSTextView          *             CrawlField;
  IBOutlet NSTextField         *             WidthField;
  IBOutlet NSTextField         *            HeightField;
  IBOutlet NSTextField         *        HeightUsedField;
  IBOutlet NSTextField         *         FrameRateField;
  IBOutlet NSTextField         *       DestinationField;
  IBOutlet NSTextField         *         StarfieldField;
  IBOutlet NSButton            *  StarfieldAutoCheckbox;
  IBOutlet NSButton            *  StarfieldChooseButton;
  IBOutlet NSButton            *DestinationChooseButton;
  IBOutlet NSButton            *               GoButton;
  IBOutlet NSTextField         *           ErrorMessage;
  IBOutlet NSWindow            *       DisclaimerWindow;
  IBOutlet NSWindow            *          ControlWindow;
  IBOutlet NSWindow            *            ImageWindow;
  IBOutlet NSImageView         *           TheImageView;
  IBOutlet NSProgressIndicator *            ProgressBar;
  IBOutlet NSImageView         *              ColorBar1;
  IBOutlet NSImageView         *              ColorBar2;
  IBOutlet NSTextField         *             StatusText;
  IBOutlet NSMatrix            * ScreenSizeRadioButtons;
  IBOutlet NSMatrix            *  FrameRateRadioButtons; }

#if COMPRESS_FONTS
- (void                       ) compressBitmapFont           :(BYTE           *) fontFile fontInfo:(int4 *) fontInfo fontInfoSize:(int4) fontInfoSize;
#endif
- (void                       ) enableControls               :(bool            ) enable  ;
- (IBAction                   ) handleDisclaimerAgreed       :(id              ) sender  ;
- (IBAction                   ) setScreenSize                :(id              ) sender  ;
- (IBAction                   ) setFrameRate                 :(id              ) sender  ;
- (IBAction                   ) setStarfieldAuto             :(id              ) sender  ;
- (IBAction                   ) chooseFramesDestinationFolder:(id              ) sender  ;
- (IBAction                   ) chooseStarfieldFile          :(id              ) sender  ;
- (IBAction                   ) handleOpen                   :(id              ) sender  ;
- (IBAction                   ) handleGoStopButton           :(id              ) sender  ;
- (IBAction                   ) handleSave                   :(id              ) sender  ;
- (IBAction                   ) handleSaveAs                 :(id              ) sender  ;
- (IBAction                   ) controlTextDidChange         :(id              ) sender  ;
- (IBAction                   ) handleUndo                   :(id              ) sender  ;
- (IBAction                   ) handleRedo                   :(id              ) sender  ;
- (void                       ) restoreUndoDataToUI          :(short           ) type data:(short) data text:(int4 *) text;
- (void                       ) userContentChanged           :(NSNotification *) note    ;
- (bool                       ) saveUserContent                                          ;
- (bool                       ) readFile                     :(NSString       *) fullPath;
- (NSApplicationTerminateReply) applicationShouldTerminate   :(NSApplication  *) sender  ;
- (bool                       ) validateMenuItem             :(NSMenuItem     *) theItem ;
- (bool                       ) application                  :(NSApplication  *) theApp openFile:(NSString *) fullFilePath;
- (void                       ) copyControlValuesToVars                                  ;
- (NSString                  *) windowTitleFromPath          :(NSString       *) path    ;
- (void                       ) launchRenderingThreads       :(int4            ) subPixels;
- (void                       ) killRenderingThreads                                     ;
- (void                       ) canFailApp                   :(BYTE           *) errorMessage;
- (void                       ) failApp                      :(BYTE           *) errorMessage;
- (void                       ) alert                        :(BYTE           *) alertMessage;
- (int4                       ) ask                          :(BYTE           *) alertMessage   no:(BYTE *) textNO yes:(BYTE *) textYES;
- (int4                       ) ask3                         :(BYTE           *) alertMessage zero:(BYTE *) text0  one:(BYTE *) text1   two:(BYTE *) text2;
- (void                       ) createTowerImages                                        ;
- (void                       ) createSearchlightMaskImage                               ;
- (NSString                  *) xmlSafe                      :(NSString       *) str     ;
- (void                       ) loadCbfDistInfo                                          ;
- (void                       ) loadCbfIntro                                             ;
- (void                       ) loadCbfCrawl                                             ;
- (bool                       ) loadCustomStarfield                                      ;
- (bool                       ) desktopWarning                                           ;
- (void                       ) checkForFieldFocusChanges                                ;
- (void                       ) moveToNextResponder:(id) sender                          ;
- (void                       ) moveToPrevResponder:(id) sender                          ;
- (void                       ) startImageUpdating                                       ;
- (void                       ) updateImageAndAdvanceSequence                            ;
- (void                       ) textDidChange:(NSNotification *) aNotification           ;

@end

bool    createStarfieldBitmap         (NSBitmapImageRep **bitmap, int4 wid, int4 hei);
void    showStatusFrame               (NSTextField *field, int4 frame);
double  stageFrac                     ();
void    createBitmap                  ();
void    createBackdropBitmap          (BYTE *bmp);
void    createTowerBitmap             ();
void    createTowerMaskBitmap         ();
void    createSearchlightMaskBitmap   ();
void    releaseFilmCoBuffers          ();
void    releaseBellCurveArrays        ();
void    createImage                   ();
void    createDistributorBackdropImage(id controller);
void    restoreField                  (NSTextField *field, int4 *text, int4 *contents, bool firstTab, NSTabView *tabs, id controller);
void    restoreView                   (NSTextView *field, int4 *text, int4 *contents, bool firstTab, NSTabView *tabs, id controller, NSWindow *wind);
void    makeFieldUndo                 (NSTextField *field, short type, int4 *contents);
void    makeViewUndo                  (NSTextView *field, short type, int4 *contents);
bool    hasFocus                      (id theField);
double  titleDistByTime               (double time);
void    setCrawlState                 (int4 renderStage, double stageFrac, id controller);
BYTE   *createAndWriteBmpFile         (NSString *folderPath, int4 frameNumber);
BYTE    getByte                       (NSFileHandle *theFile);
void    convertCStrToUnicodeLongStr   (int4 *dst, BYTE *src, int4 max);
bool    fileHasXmlHeader              (NSString *xmlFilePath);

BYTE *parseSwtsgFile(NSString *xmlFilePath,
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
int4 *ioStarfield       , int4 maxStarfield       );
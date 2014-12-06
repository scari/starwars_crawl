//  Star Wars TSG
//  Darel Rex Finley, 2006-2011



//  Special, app-control constants.
#define  THREAD_DIAGNOSTIC       NO    //  If YES, shows the value of RenderThreads in the title bar of the control window.
#define  RENDERING_THREAD_COUNT   0    //  Dictates the number of rendering threads -- or if zero, allows the number of processors (cores) to determine the number of rendering threads.
#define  GREEN_CYAN_DIAGNOSTIC    0    //  0 or 1.  If 1, reveals which areas of the image are being re-rendered and which aren’t.  WARNING:  This option makes the app unstable when changing image sizes.  It is for developer use only.
#define  COMPRESS_FONTS           0    //  0 or 1.  If 1, the app does nothing but compresses the three bitmap fonts, then exits.  (See the file “Bitmap Font Compression”.)



//  Convenience definition used throughout the entire project.
#define  BYTE  unsigned char
#define  int4  int



//  Subpixel antialiasing constants.
#define  OVERSCAN_LO                1                //  ( 1x 1 =   1 subpixel )
#define  OVERSCAN_HI                4                //  ( 4x 4 =  16 subpixels)
#define  OVERSCAN_MAX              16                //  (16x16 = 256 subpixels)
#define  TITLE_CHAR_OVERSCAN_RES  (42*OVERSCAN_HI)   //  (value determined experimentally)



//  Arbitrary number of points to break a spline into, for kerning
//  purposes (i.e. for determining how close one character is to another).
#define  KERNING_SPLINE_POINTS  20



//  BMP-format parsing constants.
#define  BMP_HEADERSIZE_GREY  54L+1024L
#define  BMP_HEADERSIZE_RGB   54L
#define  BMP_SIZE_LOC         18L



//  Number of characters theoretically supportable by 4-byte UTF-8.  (Largest possible 21-bit value, plus one.)
#define  UTF8_CHARS  2097152

//  Extended attribute values for telling other apps that a user file is encoded as UTF-8.
#define  UTF8_ATTR_NAME   "com.apple.TextEncoding"
#define  UTF8_ATTR_VALUE  "UTF-8;134217984"



//  Maximum value of a signed int4.
#define  MAX_INT4  2147483647



//  Constants for controlling how much text may be entered in the various fields.

#define  TEXT_LIMIT_REACHED     "Text limit reached; end of text will be truncated."
#define  TEXT_FIELD_MAX         5000   //  Must be at least as large as any of the MAX_CHARS_XXX constants below.

#define  MAX_CHARS_DISTRIBUTOR    16*5   //  (Each of these is five times as large as needed to hold original movie text.)
#define  MAX_CHARS_DIST_INFO      26*5
#define  MAX_CHARS_FILM_CO         9*5
#define  MAX_CHARS_FILM_CO_INC     3*5
#define  MAX_CHARS_INTRO          48*5
#define  MAX_CHARS_TITLE           9*5
#define  MAX_CHARS_CRAWL         506*5

#define  MAX_CHARS_OPTION         10
#define  MAX_CHARS_CHECKBOX        1
#define  MAX_CHARS_FIELD_INT      10
#define  MAX_CHARS_FIELD_FLOAT    50
#define  MAX_CHARS_FIELD_PATH   1000



//  “Undo” constants.
#define  UNDO_MAX                 50
#define  MULTI_STRING_DELIMITER  '|'



//  Numeric limits on user-entered image-size and frame-rate specifications.

#define  IMAGE_WID_MIN        320
#define  IMAGE_WID_MAX       3840
#define  IMAGE_HEI_MIN        136
#define  IMAGE_HEI_MAX       2160
#define  IMAGE_HEI_USED_MIN   102

#define  MOVIE_FRAME_RATE_MIN    6
#define  MOVIE_FRAME_RATE_MAX  600



//  Used variously throughout project.
//
//  (Note:  The Y axis goes up, not down, in this structure; T>B.)

struct DoubleRect_ {
  double  L ;
  double  T ;
  double  R ;
  double  B ; }
;
typedef struct DoubleRect_  DoubleRect_ ;



//  Stores the movie-screen position of one Distributor Info character.  (See the file “Coordinate Systems”.)

struct DistInfoChar_ {
  double  L ;
  double  R ; }
;
typedef struct DistInfoChar_  DistInfoChar_ ;



//  Stores the movie-screen position of one Film Company character.  (See the file “Coordinate Systems”.)

struct FilmCoChar_ {
  double  left     ;
  double  right    ;
  double  bottomHi ;
  double  bottomLo ; }
;
typedef struct FilmCoChar_  FilmCoChar_ ;



//  Stores the movie-screen position of one Film Company Inc character.  (See the file “Coordinate Systems”.)

struct  FilmCoIncChar_ {
  double  left  ;
  double  right ;
  double  topLo ; }
;
typedef struct FilmCoIncChar_  FilmCoIncChar_ ;



//  Stores the movie-screen position (and other data) of one Title character.  (See the file “Coordinate Systems”.)

struct TitleChar_ {
  int4         uni    ;
  DoubleRect_  rect   ;
  double       Bextra ;     //  for font descenders
  double       Textra ;     //  for font  ascenders
  char         rowL   ;     //  leftmost in its row
  char         rowT   ;     //  in topmost row
  char         rowR   ;     //  rightmost in its row
  char         rowB   ; }   //  in bottommost row
;
typedef struct TitleChar_  TitleChar_ ;



//  Stores the movie-screen position (and other data) of one Crawl character.  (See the file “Coordinate Systems”.)

struct CrawlChar_ {
  int4         uni  ;
  DoubleRect_  rect ; }
;
typedef struct CrawlChar_  CrawlChar_ ;



//  Stores data about one undoable user action.

struct UndoItem_ {
  short  type                      ;   //  See below section “Values for UndoItem_ struct’s ‘type’ member”.
  short  dataOld                   ;
  short  dataNew                   ;
  char   saveAfter                 ;   //  YES if the user file was saved after performing this undoable action.
  int4   textOld[TEXT_FIELD_MAX+1] ;
  int4   textNew[TEXT_FIELD_MAX+1] ; }
;
typedef struct UndoItem_  UndoItem_ ;



//  See the file “Raster Segments” for more info.
//
//  Note:  If a segment’s length (right-left) is non-positive, that segment is currently unused.

#define  MAX_RENDER_SEGS_PER_ROW  4
#define  LO_HI_VALS               2

struct RasterRow_ {
  int4  left [LO_HI_VALS][MAX_RENDER_SEGS_PER_ROW] ;
  int4  right[LO_HI_VALS][MAX_RENDER_SEGS_PER_ROW] ; }
;
typedef struct RasterRow_  RasterRow_ ;



//  Used by renderThread to keep calculations as local as possible (see comments there).

struct RenderRow_ {
  BYTE        r[IMAGE_WID_MAX] ;     //  RGB values as calculated by drawXxxPixel functions
  BYTE        g[IMAGE_WID_MAX] ;
  BYTE        b[IMAGE_WID_MAX] ;
  int4        y                ;     //  to which image row this RenderRow_ refers
  RasterRow_  raster           ; }   //  describes which pixels in this row need to be rendered
;
typedef struct RenderRow_  RenderRow_ ;


//  Data extracted from the Distributor font.  (Rendering speed is improved by having this data at-the-ready.)

struct  DistributorGlyph_ {
  double  width ;
  int4    start ; }
;
typedef struct DistributorGlyph_  DistributorGlyph_ ;



//  Data extracted from the Film Company font.  (Rendering speed is improved by having this data at-the-ready.)

struct FilmCoGlyph_ {
  double  width      ;
  double  yMult      ;
  double  sectionLo  ;
  double  sectionHi  ;
  double  kern       ;
  char    flatTop    ;
  char    flatBottom ;
  char    hitsTop    ;
  char    overPeak   ;
  char    bottomSect ;
  int4    start      ; }
;
typedef struct FilmCoGlyph_  FilmCoGlyph_ ;



//  Data extracted from the Title font.  (Rendering speed is improved by having this data at-the-ready.)

struct  TitleGlyph_ {
  double  width     ;
  double  top       ;     //  Top edge of glyph; not less than TITLE_GLYPH_RANGE.
  double  bot       ;     //  Bottom edge of glyph; not greater than 0.
  int4    start     ;
  int4    ribbonTLi ;     //  (   top  left; -1 if none; relative to .start)
  int4    ribbonTRi ;     //  (   top right; -1 if none; relative to .start)
  int4    ribbonBLi ;     //  (bottom  left; -1 if none; relative to .start)
  int4    ribbonBRi ; }   //  (bottom right; -1 if none; relative to .start)
;
typedef struct TitleGlyph_  TitleGlyph_ ;



//  Data about one row of text in the Distributor logo.

struct  DistributorRow_ {
  double  wid    ;
  double  hei    ;
  double  top    ;
  double  scaleX ;
  double  rise   ;
  double  desc   ;
  int4    start  ;
  int4    end    ; }
;
typedef struct DistributorRow_  DistributorRow_ ;



//  Data about one row of text in the Crawl.

struct CrawlRow_ {
  int4  start ;
  int4  end   ; }
;
typedef struct CrawlRow_  CrawlRow_ ;



//  Data about one star in the starfield (that sits behind the Title and Crawl).

struct Star_ {
  double  x     ;
  double  y     ;
  double  size2 ; }   //  (size squared)
;
typedef struct Star_  Star_ ;



//  Data about one substar (substars are used to speed up rendering by avoiding most stars in the starfield).

struct  SubStar_ {
  double  start ;
  double  end   ; }
;
typedef struct SubStar_  SubStar_ ;



//  Data about one Film Company sparkle.

struct Sparkle_ {
  double  x     ;     //  Movie-screen X-coordinate.  (See the file “Coordinate Systems”.)
  double  y     ;     //  Movie-screen Y-coordinate.  (See the file “Coordinate Systems”.)
  double  start ; }   //  Time when this sparkle first appears.  0-1, as measured in the entire time the Film Co logo is visible.
;
typedef struct Sparkle_  Sparkle_ ;



//  Special math constants.

#define  MATH_E                2.71828183
#define  CIRCLE_RADIANS        6.28318531   //  See http://alienryderflex.com/pi.html
#define  BELL_REACH            3.33         //  The point at which a standard bell curve diminishes to about 1/256.
#define  ARC_FRAC               .41421356   //  sqrt(2)-1
#define  INF             9999999.           //  (Ideally, this should be defined as the maximum value a double hold.)



//  Character constants.

#define     RETURN_CHAR  13
#define    NEWLINE_CHAR  10
#define        TAB_CHAR   9
#define  SHIFT_TAB_CHAR  25



//  Movie screen coordinate constants.  (See the file “Coordinate Systems”.)
#define  MOVIE_SCREEN_WID  2.35
#define  MOVIE_SCREEN_HEI  1.



//  Distance at which Title logo should be displayed when previewing user input (not rendering sequence).

#define  TITLE_PREVIEW_DIST  1.5   //  1.0 = distance at which title just touches edge of movie screen



//  Constants for generating brushed-metal look in Film Company logo’s core.  (This
//  doesn’t really look that much like the authentic Lucasfilm logo, but works OK anyway.)
#define  METAL_SCRATCH_WIDTH_ADJUSTER  150.
#define  METAL_SCRATCH_LEVEL_LO        158
#define  METAL_SCRATCH_LEVEL_HI        255



#define  USER_CONTENT_FILE_EXTENSION  ".swtsg"   //  (also hard-coded in “Info.plist”, in more than one place, and in the name of the document-icon .icns file)



//  Polygon-data tags -- used by point-in-polygon function(s).
//
//  See file “Polygon Constants” for a detailed description of how these tags work.

enum {

  POLY_TAG_MIN=-1000,   //  (POLY_TAG_MIN and POLY_TAG_MAX are not polygon tags; just convenience demarcators of where the tags range)

  SPLINE, ARC, ARC_AUTO, NEW_LOOP, NEW_POLY, NO_CORE, END_POLY,

  POLY_TAG_MAX }
;



//  Used to keep track of which preview image is being displayed (in the variable WhichPreview).

enum {
  NONE, DISTRIBUTOR_LOGO, FILMCO_LOGO, INTRO_TEXT, TITLE_LOGO, TEXT_CRAWL, GENERATING }
;



//  Distributor constants.
#define  DISTRIBUTOR_CHAR_SEP          .125
#define  DISTRIBUTOR_ROW_SEP           .03667
#define  DISTRIBUTOR_EXTRA_ROW_SEP     .055
#define  DISTRIBUTOR_MAX_ROWS        10
#define  DISTRIBUTOR_ROW_ADJUST_TOP   2.3
#define  DISTRIBUTOR_ROW_ADJUST_BOT   1.

//  These constants help to speed up Distributor logo rendering
//  by defining a rectangle outside of which the logo can never be.
#define  DISTRIBUTOR_LOGO_RECT_L  -.38
#define  DISTRIBUTOR_LOGO_RECT_T   .47333
#define  DISTRIBUTOR_LOGO_RECT_R   .71667
#define  DISTRIBUTOR_LOGO_RECT_B  -.34

//  Distributor text colors.

#define         TEXT_BRIGHT_R  192
#define         TEXT_BRIGHT_G  180
#define         TEXT_BRIGHT_B   65

#define            TEXT_DIM_R  187
#define            TEXT_DIM_G  159
#define            TEXT_DIM_B   59

#define         TEXT_SHADOW_R  139
#define         TEXT_SHADOW_G   88
#define         TEXT_SHADOW_B    0

//  Note:  The SIDE_DARK color should be the same as PLATE_SIDE (below).
#define           SIDE_DARK_R    6
#define           SIDE_DARK_G    0
#define           SIDE_DARK_B    0

#define           SIDE_GLOW_R  146
#define           SIDE_GLOW_G   68
#define           SIDE_GLOW_B    0

#define           SIDE_SNOW_R   64
#define           SIDE_SNOW_G   64
#define           SIDE_SNOW_B   64

#define  PLATE_BOTTOM_LIGHT_R  161
#define  PLATE_BOTTOM_LIGHT_G  120
#define  PLATE_BOTTOM_LIGHT_B    0

#define   PLATE_BOTTOM_DARK_R   56
#define   PLATE_BOTTOM_DARK_G   15
#define   PLATE_BOTTOM_DARK_B    0

//  Note:  The PLATE_SIDE color should be the same as SIDE_DARK (above).
#define          PLATE_SIDE_R    6
#define          PLATE_SIDE_G    0
#define          PLATE_SIDE_B    0

#define         BEVEL_COLOR_R  255
#define         BEVEL_COLOR_G  170
#define         BEVEL_COLOR_B   50

//  Distributor text extrusion constants.

#define  SCREEN_VAN_X         1.89
#define  SCREEN_VAN_Y        - .82

#define  NOT_EVEN_CLOSE    -999.
#define  EXTRUSION_LENGTH      .45

//  Distributor dais-masking constants.

#define  DAIS_BOTTOM  -.98

//  Distributor bevelling constants.
#define  BEVEL_SIZE         .008
#define  BEVEL_OFFSET_FRAC  .667   //  Anything more than .7071 -- i.e. 1/sqrt(2) -- can cause the bevel width to go negative in places.

//  Distributor plate extension constants.

#define        PLATE_EXTEND_LR  .01666667
#define        PLATE_EXTEND_FB  .00208333
#define  EXTRA_PLATE_EXTEND_LR  .04444444
#define  EXTRA_PLATE_EXTEND_FB  .00416667

//  Front-tower area.
#define  FRONT_TOWER_L   .513333
#define  FRONT_TOWER_T  -.178991
#define  FRONT_TOWER_R   .61
#define  FRONT_TOWER_B  -.397324

#define  DISTRIBUTOR_FADE_BASE        .137

#define  DISTRIBUTOR_BEVEL_STRENGTH  3.5

#define  DISTRIBUTOR_PREVIEW_TIME    6.

#define  DISTRIBUTOR_VIEW_DIST       5.
#define  DISTRIBUTOR_TEXT_SLOPE       .11
#define  DISTRIBUTOR_X_PRE_MULT      3.
#define  DISTRIBUTOR_Y_PRE_MULT      3.03
#define  DISTRIBUTOR_X_PRE_OFFSET     .043333
#define  DISTRIBUTOR_Y_PRE_OFFSET     .68
#define  DISTRIBUTOR_MAG             4.3
#define  DISTRIBUTOR_Y_POST_OFFSET   2.6



//  Distributor-info constants.

#define  DIST_INFO_TOP  -.36667
#define  DIST_INFO_BOT  -.42

#define  DIST_INFO_SPACER_FRAC    .05     //  (controls size of gap between characters)
#define  DIST_INFO_WIDTH_MULT     .85     //  (arbitrary constant used to adjust aspect ratio of Distributor Info characters)
#define  DIST_INFO_LCASE_FRAC     .85     //  (how much smaller the lower-case characters are, as a fraction of uppercase)
#define  DIST_INFO_BASELINE_FRAC  .2137   //  (assists in shrinking glyph without moving the font baseline)

#define  DIST_INFO_R  179
#define  DIST_INFO_G  132
#define  DIST_INFO_B   29



#define  NODE_ROW_MAX  500



//  Film-company constants.

#define  FILMCO_GLYPH_SCALE  16.

#define  FILMCO_CHAR_SEP  .25

#define  FILMCO_TEXT_WIDTH            1.153
#define  FILMCO_TEXT_TOP_OVERPEAK      .173
#define  FILMCO_TEXT_TOP_ACCENTPEAK    .215
#define  FILMCO_TEXT_TOP               .169
#define  FILMCO_TEXT_BOTTOM          - .169
#define  FILMCO_TEXT_BOT_ACCENTPEAK  - .215

#define  FILMCO_BORDER_OUTER  .667
#define  FILMCO_BORDER_INNER  .333

#define  FILMCO_BELL_DIVISOR_H  38.4
#define  FILMCO_BELL_DIVISOR_V  16.34

#define  FILMCO_INC_TEXT_WIDTH_FRAC  .24   //  (a fraction of FILMCO_TEXT_WIDTH)

#define  FILMCO_DEFAULT_TOTALWIDTH      16.5
#define  FILMCO_INC_DEFAULT_TOTALWIDTH  10.5

#define  FILMCO_ARC_CENTER_Y  - .8333   //  (There is no FILMCO_ARC_CENTER_X because it is zero.)
#define  FILMCO_ARC_ENCROACH    .435

#define  FILMCO_BORDER_REACH  .0133   //  (measured in movie-screen coordinate system -- see the file “Coordinate Systems”)

#define  FILMCO_DECENDER_FRAC  .28125

#define  FILMCO_PREVIEW_R  116
#define  FILMCO_PREVIEW_G  255
#define  FILMCO_PREVIEW_B   59

#define  FILMCO_0_AURA_R    85
#define  FILMCO_0_AURA_G   255
#define  FILMCO_0_AURA_B    28

#define  FILMCO_0_AURA2_R  198
#define  FILMCO_0_AURA2_G  255
#define  FILMCO_0_AURA2_B  179

#define  FILMCO_0_BAND_R   127
#define  FILMCO_0_BAND_G   187
#define  FILMCO_0_BAND_B   128

#define  FILMCO_1_CORE_R   109
#define  FILMCO_1_CORE_G   130
#define  FILMCO_1_CORE_B   255

#define  FILMCO_2_CORE_R   255
#define  FILMCO_2_CORE_G   163
#define  FILMCO_2_CORE_B    74

#define  FILMCO_3_CORE_R   255
#define  FILMCO_3_CORE_G   231
#define  FILMCO_3_CORE_B   123

#define  FILMCO_0_BLUR_BRIGHTEN  2.
#define  FILMCO_1_KNOCKDOWN       .4
#define  FILMCO_2_RAMPUP         1.87
#define  FILMCO_3_RAMPUP         1.38

#define  FILMCO_SPARKLES            20
#define  FILMCO_SPARKLE_MAX_RADIUS    .25   //  (as measured in the “movie screen” coordinate system)
#define  FILMCO_SPARKLE_POWER        2.     //  (determines how sharply a sparkle flashes at its peak value)
#define  FILMCO_SPARKLES_BEGIN      10.47   //  (seconds from start of entire movie)
#define  FILMCO_SPARKLES_END        15.5    //  (seconds from start of entire movie)
#define  FILMCO_SPARKLE_DURATION     2.     //  (seconds, from sparkle appearance to disappearance)

//  Values of filmCoRenderMode:
enum {
  SILHOUETTE_A=0, BLUR_A_H, BLUR_A_V,
  SILHOUETTE_B, BLUR_B_H, BLUR_B_V, SILHOUETTE_C, BLUR_C_H, BLUR_C_V, FRILL_BORDERS,
  BORDERED_OVERLAY, BLUE_CORE, ORANGE_TINT, YELLOW_TINT, BUFFERS_READY }
;



//  Intro constants.
#define  INTRO_WIDTH_MAX    1.05
#define  INTRO_CHAR_SEP      .005
#define  INTRO_LINE_HEI      .1429
#define  INTRO_LINE_SEP      .025
#define  INTRO_Y_OFFSET     -.02
#define  INTRO_ROWS_MAX   100

//  This color was obtained from the TFN website.
#define  INTRO_COLOR_R   75
#define  INTRO_COLOR_G  213
#define  INTRO_COLOR_B  238



//  How many pixel-row buffers to keep in each rendering thread.  (This is a very arbitrary number.)
#define  PIXEL_ROW_BUFFERS  8



//  Crawl constants.

#define  CRAWL_LINE_HEI  .18516129033
#define  CRAWL_LINE_SEP  .00983870967
#define  CRAWL_CHAR_SEP  .017875

#define  CRAWL_COLOR_R  255
#define  CRAWL_COLOR_G  223
#define  CRAWL_COLOR_B    0

//  These three are used by crawlFwdPerspective:
#define  CRAWL_VANISH_Y         .65
#define  CRAWL_PERSP_STRENGTH   .8
#define  CRAWL_X_STRETCH       1.25

#define  ORIGINAL_MOVIE_CRAWL_BOTTOM  -4.7125

#define  STARS                      300
#define  STAR_CUTOFF                  3.
#define  STAR_SIZE_MIN                0.
#define  STAR_SIZE_MAX                 .003
#define  STAR_BRIGHTNESS_MODERATOR     .67
#define  STAR_OVERREACH               1.02

#define  CRAWL_HI_ROWS  50   //  Prevents bad flickering of distant crawl-text.  Value determined experimentally.

#define  TITLE_GLYPH_MAX_BYTES      500
#define  TITLE_GLYPH_RANGE           54.
#define  TITLE_CORE_INSET             2.3
#define  TITLE_ROW_COUNT_MAX         20
#define  TITLE_ROW_SEP                3.
#define  TITLE_KERN_OVAL_RATIO        8.
#define  TITLE_CHAR_SEP               3.44
#define  TITLE_CHAR_T_ADJUSTMENT     17.83
#define  TITLE_GLYPH_CONNECTOR_HEI   17.6
#define  TITLE_RIBBON_L_MINIMUM      23.5
#define  TITLE_RIBBON_R_MINIMUM      11.4

#define  TITLE_START_DIST   .75
#define  TITLE_SPEED       1.
#define  TITLE_ACCEL_TIME   .5    //  Time (in seconds) it takes Title to accellerate from zero to TITLE_SPEED.



#define  IMAGE_UPDATE_DELAY  .15   //  Minimum time (in seconds) between image window updates.



//  Values for UndoItem_ struct’s “type” member.
//
//  Note:  The UNDO_SAVE value does not allow undoing a file-save.  It
//         just generates a warning when you try to undo past a file-save.
//
enum {
  UNDO_NONE               ,
  UNDO_TEXT_DISTRIBUTOR   ,
  UNDO_TEXT_DISTINFO      ,
  UNDO_TEXT_FILMCO        ,
  UNDO_TEXT_FILMCOINC     ,
  UNDO_TEXT_INTRO         ,
  UNDO_TEXT_TITLE         ,
  UNDO_TEXT_CRAWL         ,
  UNDO_OPT_SCREENSIZE     ,
  UNDO_TEXT_WIDTH         ,
  UNDO_TEXT_HEIGHT        ,
  UNDO_TEXT_HEIGHTUSED    ,
  UNDO_OPT_FRAMERATE      ,
  UNDO_TEXT_FRAMERATE     ,
  UNDO_TEXT_DESTINATION   ,
  UNDO_CHECK_STARFIELDAUTO,
  UNDO_TEXT_STARFIELD       }
;



//  “RenderStage” values.  (Note:  Must start with 0.)

enum {
  DARK_A=0, DISTRIBUTOR_FADE_UP, DISTRIBUTOR_HOLD, DISTRIBUTOR_FADE_DOWN, DARK_B, FILMCO_FADE_UP, FILMCO_SPARKLE_GREEN,
  FILMCO_SPARKLE_GREEN_TO_BLUE, FILMCO_SPARKLE_BLUE_TO_ORANGE, FILMCO_ORANGE_TO_YELLOW, FILMCO_FADE_DOWN, DARK_C,
  INTRO_FADE_UP, INTRO, INTRO_FADE_DOWN, DARK_D, TITLE_PULL_BACK, CRAWL_AND_TITLE_PULL_BACK, CRAWL_AND_TITLE_FADE_DOWN,
  CRAWL_TO_ALL_TEXT_VISIBLE, CRAWL_OFF_TO_DISTANCE, CRAWL_FADE_DOWN, JUST_STARS, SEQUENCE_END }
;
//  Star Wars TSG
//  Darel Rex Finley, 2006-2011

//=====ÒCRAWLÓ VARS START HERE

extern bool  AutoStarfield ;

extern double  CrawlSpeed ;

extern CrawlRow_  CrawlRow  [IMAGE_HEI_MAX][OVERSCAN_HI            ] ;
extern CrawlRow_  CrawlRowHi[CRAWL_HI_ROWS][OVERSCAN_HI*OVERSCAN_HI] ;
extern int4       CrawlRowSubPixels ;

extern int4         CrawlText[MAX_CHARS_CRAWL+1] ;
extern CrawlChar_   CrawlChar[MAX_CHARS_CRAWL  ] ;
extern double       CrawlOpacity ;
extern int4         CrawlChars   ;

extern BYTE  *CbfCrawl ;

extern double  CrawlBottomEdge ;

//=====ÒTITLEÓ VARS START HERE

extern double  TitleDist, TitleOpacity ;

extern int4  RasterSubPixels_Title ;

extern TitleGlyph_  *TitleGlyph  ;
extern int4          TitleGlyphs ;

extern double      TitleMovieScreenWidth, TitleGlyphToMovieScreenRatio ;
extern int4        TitleText[MAX_CHARS_TITLE+1] ;
extern TitleChar_  TitleChar[MAX_CHARS_TITLE  ] ;
extern int4        TitleChars ;

//=====STARFIELD VARS START HERE

extern Star_     Star   [STARS        ] ;
extern SubStar_  SubStar[IMAGE_HEI_MAX] ;

extern bool    StarsImageReady    ;

//=====ÒCRAWLÓ FUNCTIONS START HERE
double  calcCrawlHeightTime               (double crawlToAllTextVisible);
double  calcCrawlBottom                   ();
void    forceCrawlRowArrayCreation        ();
void    forceRasterCrawl                  (bool lo, bool hi);
void    crawlFwdPerspective               (double *x, double *y, double crawlBottom);
void    crawlRevPerspective               (double *x, double *y, double crawlBottom);
BYTE   *prepareToDrawCrawlImage           (BYTE **rgb, int4 rowBytes);
void    buildCrawlRowArray                ();
void    drawCrawlPixel                    (BYTE *r, BYTE *g, BYTE *b, int4 pixelX, int4 pixelY, int4 *cbfChar, BYTE *cbfFill, short *cbfX, short *cbfY, bool *cbfRaw, BYTE **cbfData, BYTE *starImage, bool drawStars, bool *nodeRowsReady, int4 *nodes, double *nodeX, int4 *polyTag, int4 *nodeNext, bool *inPoly, bool *inInnerPoly, BYTE *starsR, BYTE *starsG, BYTE *starsB, int4 starsWid, int4 starsHei, int4 starsRowBytes);
//=====ÒTITLEÓ FUNCTIONS START HERE
bool    titleKerningDistanceTest          (double kernDist, double kernOvalness, double *poly, double *poly2, double poly2pos, bool isSpline, double testSx, double testSy, double testSplineX, double testSplineY, double testEx, double testEy);
void    addNodesFromSplineArcPoly_Title   (double *poly, double y, int4 subPixelRow, TitleChar_ charLoc, double glyphWid, int4 polyCount, int4 *nodes, double *nodeX, int4 *polyTag, int4 ribbonTL, int4 ribbonTR, int4 ribbonBL, int4 ribbonBR, bool rowL, bool rowT, bool rowR, bool rowB, int4 uniBefore, int4 uniAfter);
int4    nextPoint_ArcAuto                 (double *poly, int4 start, int4 i);
int4    prevPoint_ArcAuto                 (double *poly, int4 start, int4 i);
void    constructTitleGlyphBlackCore      (double *src, double *dst);
BYTE   *processTitleFont                  ();
void    prepTitleText                     (int4 *text);
void    findTitleRowTopBot                (int4 start, int4 end, double *bot, double *top);
double  titleKern                         (int4 i);
void    prepareToDrawTitleImage           (BYTE **rgb, int4 rowBytes);
void    handleSplineCurveForNodesRow_Title(double y, double sX, double sY, double a, double b, double eX, double eY, int4 subPixelRow, int4 polyCount, int4 *nodes, double *nodeX, int4 *polyTag);
void    findBevelInsetPoint               (double  a, double  b, double *c, double *d, double  e, double  f, double insetDist);
//=====STARFIELD FUNCTIONS START HERE
bool    starRelevant                      (int4 starI, double y, double yUp);
double  starLev                           (double x, double y, int4 pixelY);
void    generateStarField                 ();
void    createSubStarRanges               ();
void    forceRasterTitlePreview           (bool lo, bool hi);
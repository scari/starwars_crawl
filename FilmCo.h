//  Star Wars TSG
//  Darel Rex Finley, 2006-2011

extern double   FilmCoBellDimensionH ;
extern double   FilmCoBellDimensionV ;
extern double  *FilmCoBellH          ;
extern double  *FilmCoBellV          ;
extern double   FilmCoBellRadHmax    ;
extern double   FilmCoBellRadVmax    ;
extern double   FilmCoBellRadH       ;
extern double   FilmCoBellRadV       ;

extern FilmCoGlyph_  *FilmCoGlyph  ;
extern int4           FilmCoGlyphs ;

extern double          FilmCoFade ;
extern double          FilmCoProgress, FilmCoSparkleDur ;
extern FilmCoChar_     FilmCoChar   [MAX_CHARS_FILM_CO    ] ;
extern FilmCoIncChar_  FilmCoIncChar[MAX_CHARS_FILM_CO_INC] ;
extern double          BaseArcFrillInvSlope, BaseArcFrillRad, BaseArcFrillRad2, BaseArcFrillCapX, BaseArcFrillCapY ;
extern double          BaseArcRad, BaseArcRad2, BaseArcLimitL, BaseArcLimitR, BaseArcIncRad, BaseArcIncRad2 ;
extern int4            FilmCoText   [MAX_CHARS_FILM_CO    +1] ;
extern int4            FilmCoIncText[MAX_CHARS_FILM_CO_INC+1] ;
extern int4            FilmCoChars, FilmCoIncChars ;

extern Sparkle_  Sparkle[FILMCO_SPARKLES] ;

extern BYTE  MetalRnd[5000] ;

void    constructSparkles               ();
void    generateMetalScratches          ();
void    setFilmCoProgressByMovieTime    (double movieTime);
void    forceRasterFilmCo               (bool lo, bool hi);
void    compressFilmCoGlyphSections     (double *y, double charL, double charT, double charR, double charB, double glyphWid, double sectLo, double sectHi, double yExpand);
bool    wrapLineAroundBaseArc           (double *sX, double *sY, double *bX, double *bY, double *cX, double *cY, double *eX, double *eY, double charL, double charR, double charB, double sectLo);
bool    wrapLineAroundBaseArcInc        (double *sX, double *sY, double *bX, double *bY, double *cX, double *cY, double *eX, double *eY, double charL, double charT, double charR, double charB, double sectHi);
void    wrapSplineAroundBaseArc         (double sX, double *sY, double a, double *b, double eX, double *eY, double charL, double charR, double charB, double sectLo);
void    wrapSplineAroundBaseArcInc      (double sX, double *sY, double a, double *b, double eX, double *eY, double charL, double charT, double charR, double charB, double sectHi);
double  rgbTOp                          (BYTE r, BYTE g, BYTE b);
void    handleSplineCurve               (double *minGap, double x, double y, double sX, double sY, double a, double b, double eX, double eY, double glyphWid, double sectLo, double sectHi, double yExpand, double charL, double charT, double charR, double charB, bool wrapArc, bool doCompress, bool inc, bool sSpline, bool eSpline, int4 *nodeCount);
void    handleSplineCurveForNodesRow    (double y, double sX, double sY, double a, double b, double eX, double eY, double glyphWid, double sectLo, double sectHi, double yExpand, double charL, double charT, double charR, double charB, bool wrapArc, bool doCompress, bool inc, bool sSpline, bool eSpline, int4 subPixelRow, int4 polyCount, int4 *nodes, double *nodeX, int4 *polyTag);
void    handleSplineCurveForDist        (double *minDist, double x, double y, double sX, double sY, double a, double b, double eX, double eY, double glyphWid, double sectLo, double sectHi, double yExpand, double charL, double charT, double charR, double charB, bool wrapArc, bool doCompress, bool inc, bool sSpline, bool eSpline);
int4    pointInSplineArcPoly            (double *minGap, double *poly, double x, double y, double charL, double charT, double charR, double charB, double glyphWid, bool wrapArc, double sectLo, double sectHi, double yExpand, bool overPeak, bool inc);
void    addNodesFromSplineArcPoly_FilmCo(double *poly, double y, double charL, double charT, double charR, double charB, double glyphWid, bool wrapArc, double sectLo, double sectHi, double yExpand, bool overPeak, bool inc, int4 subPixelRow, int4 *polyCount, int4 *nodes, double *nodeX, int4 *polyTag);
void    handleArcAutoForDist            (double *minDist, double x, double y, double aX, double aY, double bX, double bY, double cX, double cY, double dX, double dY, double charL, double charT, double charR, double charB, double charWid, double glyphWid, double yExpand, double sectLo, double sectHi, bool inc);
double  distToFilmCoChar                (double *poly, double x, double y, double charL, double charT, double charR, double charB, double glyphWid, bool wrapArc, double sectLo, double sectHi, double yExpand, bool overPeak, bool inc);
BYTE   *processFilmCoFont               ();
void    prepFilmCoText                  (int4 *text);
void    prepFilmCoIncText               (int4 *text);
bool    charMoreThanHalfOffLeftSideOfArc(int4 i);
void    prepareToDrawFilmCoImage        (BYTE **rgb, int4 rowBytes, double blurFrac);
void    buildFilmCoBellCurve            (double rad, double *array);
bool    shouldFilmCoCharArc             (int4 i);
void    makeMirrorNodes                 (double x, int4 setNum, int4 subPixelRow, int4 *nodes, double *nodeX, int4 *polyTag);
void    addNodesForCurlyFrill           (double pad, double y, double flexEdge, int4 setNum, int4 subPixelRow, int4 *nodes, double *nodeX, int4 *polyTag);
void    addNodesForBow                  (double pad, double y, int4 setNum, int4 subPixelRow, int4 *nodes, double *nodeX, int4 *polyTag);
void    drawFilmCoPreviewPixel          (BYTE *r, BYTE *g, BYTE *b, int4 pixelX, int4 pixelY, bool *nodeRowsReady, int4 *nodes, double *nodeX, int4 *polyTag, int4 *nodeNext, bool *inPoly);
void    drawFilmCoPixel                 (BYTE *r, BYTE *g, BYTE *b, int4 pixelX, int4 pixelY, int4 mode, int4 stage, BYTE *bufGreen, BYTE *bufBlue, BYTE *bufOrange, BYTE *bufYellow, bool *nodeRowsReady, int4 *nodes, double *nodeX, int4 *polyTag, int4 *nodeNext, bool *inPoly);
void    blurFilmCoPixels                (BYTE *r, BYTE *g, BYTE *b, int4 mode, double rad);
void    handleArcAuto                   (double *minGap, double x, double y, double aX, double aY, double bX, double bY, double cX, double cY, double dX, double dY, double charL, double charT, double charR, double charB, double charWid, double glyphWid, double yExpand, double sectLo, double sectHi, bool inc, int4 *nodeCount);
void    handleArcAutoForNodesRow        (double y, double aX, double aY, double bX, double bY, double cX, double cY, double dX, double dY, double charL, double charT, double charR, double charB, double charWid, double glyphWid, double yExpand, double sectLo, double sectHi, bool inc, int4 subPixelRow, int4 polyCount, int4 *nodes, double *nodeX, int4 *polyTag);
void    killNode                        (int4 theNode, int4 subPixelRow, int4 *nodes, double *nodeX, int4 *polyTag);
void    deOverlapNodeRow                (int4 subPixelRow, int4 *nodes, double *nodeX, int4 *polyTag);
//  Star Wars TSG
//  Darel Rex Finley, 2006-2011

extern int4               DistributorBackdropWid, DistributorBackdropHei ;
extern DistributorRow_    DistributorRow[DISTRIBUTOR_MAX_ROWS] ;
extern double             VanX, VanY ;
extern BYTE             **DistributorTowerMaskLev, **DistributorTowerRgb, **DistributorBackdrop, **DistributorSearchlightMaskLev ;
extern BYTE               DistributorFade ;
extern int4               DistributorTowerMaskRowBytes, DistributorTowerRowBytes, DistributorSearchlightMaskRowBytes ;
extern int4               DistributorTowerWid, DistributorTowerHei, DistributorBackdropRowBytes ;
extern BYTE              *CbfDistInfo ;
extern int4               DistributorRows ;
extern bool               DistributorImageReady ;
extern int4               DistributorText[MAX_CHARS_DISTRIBUTOR+1] ;
extern int4               DistInfoText   [MAX_CHARS_DIST_INFO  +1] ;
extern double             StackPlate[DISTRIBUTOR_MAX_ROWS*3][9]    ;
extern double             DistributorMovieScreenSlopeMax ;

extern DistInfoChar_  DistInfoChar[MAX_CHARS_DIST_INFO] ;
extern int4           DistInfoChars ;

extern double  SearchlightAngle[SEARCHLIGHT_COUNT] ;

extern DistributorGlyph_  *DistributorGlyph  ;
extern int4                DistributorGlyphs ;

void    setSearchlightAngles         (double time);
void    setDistributorFade           (double frac);
BYTE   *processDistributorFont       ();
void    distributorFwdPerspective    (double *x, double *y);
void    distributorRevPerspective    (double *x, double *y);
void    fixDistributorCase           (int4 *text);
void    prepDistributorText          (int4 *theText);
void    prepDistInfoText             (int4 *theText);
void    prepareToDrawDistributorImage(BYTE **rgb, BYTE **backdrop, int4 rowBytes, BYTE **towerRgb, BYTE **towerMaskLev, BYTE **searchlightMaskLev, int4 towerWid, int4 towerHei, int4 towerRowBytes, int4 towerMaskRowBytes, int4 searchlightMaskRowBytes, int4 backdropRowBytes);
void    extractDistributorGlyphBounds(int4 c, DoubleRect_ *rect);
BYTE   *prepareDistInfoChars         ();
void    drawDistributorPixel         (BYTE *r, BYTE *g, BYTE *b, int4 pixelX, int4 pixelY, int4 *cbfChar, BYTE *cbfFill, short *cbfX, short *cbfY, bool *cbfRaw, BYTE **cbfData, BYTE *distributorImage, bool inRender);
void    accumulateBevelColor         (double screenX, double screenY, double sX, double sY, double eX, double eY, double *r, double *g, double *b, double frac);
void    forceRasterDistInfo          (bool lo, bool hi);
void    forceRasterSearchlight       (bool lo, bool hi);
void    searchlightCalc              (double screenX, double screenY, double *r, double *g, double *b, double fillFrac, bool isNear, bool showMovingSearchlights);
double  polyExtrusionCloseness       (double *poly, double x, double y, double vanX, double vanY, double *sine);
double  computeShadows               (double x, double y, double side, double mid);
double  computeFlood                 (double y);
double  distributorTextColorR        (double x, double y);
double  distributorTextColorG        (double x, double y);
double  distributorTextColorB        (double x, double y);
int4    pointInSplinePoly            (double *poly, double x, double y);
bool    pointInSplinePolyBevel       (double *poly, double x, double y, double bevScaleX, double bevScaleY, double *bevSx, double *bevSy, double *bevEx, double *bevEy);
int4    prevPoint                    (double *poly, int4 start, int4 i);
int4    nextPoint                    (double *poly, int4 start, int4 i);
//  Star Wars TSG
//  Darel Rex Finley, 2006-2011

extern int4    PreviewRowBytes ;
extern BYTE  **PreviewRgb      ;

extern int4  ImageWid, ImageHei, ImageHeiUsed, ImageHeiLetterBox ;

extern double  FramesPerSecond ;

extern int4        RasterSubPixels, RasterNextAvailRow ;
extern RasterRow_  Raster[IMAGE_HEI_MAX] ;
extern int4        RasterLoHi ;

void    setImageWid                 (int4 w);
void    setImageHei                 (int4 h);
void    setImageHeiUsed             (int4 h);
void    forceRasterParallelagram    (bool lo, bool hi, int4 lt, int4 rt, int4 t, int4 lb, int4 rb, int4 b);
void    forceRasterRect             (bool lo, bool hi, int4 left, int4 top, int4 right, int4 bottom);
void    forceRasterWholeImage       (bool lo, bool hi);
void    calcLetterBox               ();
void    nullRaster                  (bool lo, bool hi);
void    calcLetterBox               ();
void    addRasterSegment            (int4 *a, int4 *b, int4 newA, int4 newB);
void    handleLineSegment           (double x, double y, double sX, double sY, double eX, double eY, int4 *nodeCount);
void    handleLineSegmentForNodesRow(double y, double sX, double sY, double eX, double eY, int4 subPixelRow, int4 polyCount, int4 *nodes, double *nodeX, int4 *polyTag);
BYTE   *verifyDataTypeSizes         ();
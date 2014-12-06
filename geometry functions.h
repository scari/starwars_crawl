//  Star Wars TSG
//  Darel Rex Finley, 2006-2011

double distPointToSeg                (double x, double y, double aX, double aY, double bX, double bY);
double distSegToSeg                  (double aX, double aY, double bX, double bY, double cX, double cY, double dX, double dY);
double distPointToSpline             (double pX, double pY, double sX, double sY, double a, double b, double eX, double eY);
double distSegToSpline               (double aX, double aY, double bX, double bY, double sX, double sY, double a, double b, double eX, double eY);
double distSplineToSpline            (double sX1, double sY1, double a1, double b1, double eX1, double eY1, double sX2, double sY2, double a2, double b2, double eX2, double eY2);
bool   lineSegmentsIntersect         (double aX, double aY, double bX, double bY, double cX, double cY, double dX, double dY);
bool   linesIntersect                (double aX, double aY, double bX, double bY, double cX, double cY, double dX, double dY, double *x, double *y);
double distPointToSpline_HoningMethod(double testX, double testY, double sX, double sY, double a, double b, double eX, double eY);
void   splineSlopeNearPoint          (double testX, double testY, double sX, double sY, double a, double b, double eX, double eY, double *slopeSx, double *slopeSy, double *slopeEx, double *slopeEy);
bool   testF                         (double f, double *minF, double *minDist2, double sX, double sY, double a, double b, double eX, double eY, double testX, double testY);
void   getSplinePoint                (double sX, double sY, double a, double b, double eX, double eY, double f, double *x, double *y);
bool   inDoubleRect                  (DoubleRect_ rect, double x, double y);
double pixelToScreenX                (double pixelX);
double pixelToScreenY                (double pixelY);
double screenToPixelX                (double screenX);
double screenToPixelY                (double screenY);
void   rotatePointAroundOrigin       (double *x, double *y, double theCos, double theSin);
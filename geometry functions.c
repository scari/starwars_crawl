//  Star Wars TSG
//  Darel Rex Finley, 2006-2011



#include               "bool.h"
#include        "definitions.h"
#include  "Shared Structures.h"
#include  "numeric functions.h"
#include               "math.h"

#include "geometry functions.h"



//  Returns the shortest distance from any part of line segment a-b to point x,y.

double distPointToSeg(double x, double y, double aX, double aY, double bX, double bY) {

  double  distAB, theCos, theSin ;

  //  Translate the system so that point A is on the origin.
  bX-=aX; bY-=aY;
  x -=aX; y -=aY;

  //  Discover the length of segment A-B.
  distAB=sqrt(bX*bX+bY*bY);

  //  Rotate the system so that point B is on the positive X axis.
  theCos=bX/distAB;
  theSin=bY/distAB; rotatePointAroundOrigin(&x,&y,theCos,-theSin);

  //  Determine the distance.
  if   (x>=0. && x<=distAB) {
    if (y>=0.) {
      return  y; }
    else {
      return -y; }}
  if (x>distAB) x-=distAB;
  return sqrt(x*x+y*y); }



//  Returns the shortest distance from any part of line segment a-b to any part of line segment c-d.

double distSegToSeg(double aX, double aY, double bX, double bY, double cX, double cY, double dX, double dY) {

  double  bestDist, dist ;

  if (lineSegmentsIntersect(aX,aY,bX,bY,cX,cY,dX,dY)) return 0.;

  bestDist=distPointToSeg(cX,cY,aX,aY,bX,bY);
  dist    =distPointToSeg(dX,dY,aX,aY,bX,bY); if (dist<bestDist) bestDist=dist;
  dist    =distPointToSeg(aX,aY,cX,cY,dX,dY); if (dist<bestDist) bestDist=dist;
  dist    =distPointToSeg(bX,bY,cX,cY,dX,dY); if (dist<bestDist) bestDist=dist;

  return bestDist; }



//  This function just tests an arbitrary number of points across the spline curve, and returns the shortest
//  distance it finds.  It is not as sophisticated as the iterative, honing-in technique used in the functions
//  prepareToDrawFilmCoImage and distPointToSpline_HoningMethod.

double distPointToSpline(double pX, double pY, double sX, double sY, double a, double b, double eX, double eY) {

  double  i, x, y, x1, y1, x2, y2, dx, dy, frac, dist, bestDist=INF ;

  for (i=0.; i<=(double) KERNING_SPLINE_POINTS; i++) {
    frac=i/(double) KERNING_SPLINE_POINTS;
    x1=sX+( a-sX)*frac;
    y1=sY+( b-sY)*frac;
    x2= a+(eX- a)*frac;
    y2= b+(eY- b)*frac;
    x =x1+(x2-x1)*frac; dx=pX-x;
    y =y1+(y2-y1)*frac; dy=pY-y; dist=sqrt(dx*dx+dy*dy); if (dist<bestDist) bestDist=dist; }

  return bestDist; }



double distSegToSpline(
double aX, double aY, double bX, double bY,
double sX, double sY, double a, double b, double eX, double eY) {

  double  i, x, y, x1, y1, x2, y2, frac, dist, bestDist=INF ;

  for (i=0.; i<=(double) KERNING_SPLINE_POINTS; i++) {
    frac=i/(double) KERNING_SPLINE_POINTS;
    x1=sX+( a-sX)*frac;
    y1=sY+( b-sY)*frac;
    x2= a+(eX- a)*frac;
    y2= b+(eY- b)*frac;
    x =x1+(x2-x1)*frac;
    y =y1+(y2-y1)*frac; dist=distPointToSeg(x,y,aX,aY,bX,bY); if (dist<bestDist) bestDist=dist; }

  return bestDist; }



double distSplineToSpline(
double sX1, double sY1, double a1, double b1, double eX1, double eY1,
double sX2, double sY2, double a2, double b2, double eX2, double eY2) {

  double  i, x, y, x1, y1, x2, y2, frac, dist, bestDist=INF ;

  for (i=0.; i<=(double) KERNING_SPLINE_POINTS; i++) {
    frac=i/(double) KERNING_SPLINE_POINTS;
    x1=sX1+( a1-sX1)*frac;
    y1=sY1+( b1-sY1)*frac;
    x2= a1+(eX1- a1)*frac;
    y2= b1+(eY1- b1)*frac;
    x = x1+( x2- x1)*frac;
    y = y1+( y2- y1)*frac; dist=distPointToSpline(x,y,sX2,sY2,a2,b2,eX2,eY2); if (dist<bestDist) bestDist=dist; }

  return bestDist; }



//  Returns YES if the segment a-b intersects segment c-d -- otherwise NO.
//
//  For details on how this works, see the webpage at:  http://alienryderflex.com/intersect

bool lineSegmentsIntersect(
double aX, double aY,
double bX, double bY,
double cX, double cY,
double dX, double dY) {

  double  distAB, theCos, theSin, ABpos ;

  //  Translate the system so that point A is on the origin.
  bX-=aX; bY-=aY;
  cX-=aX; cY-=aY;
  dX-=aX; dY-=aY;

  //  Discover the length of segment A-B.
  distAB=sqrt(bX*bX+bY*bY);

  //  Rotate the system so that point B is on the positive X axis.
  theCos=bX/distAB;
  theSin=bY/distAB;
  rotatePointAroundOrigin(&cX,&cY,theCos,-theSin);
  rotatePointAroundOrigin(&dX,&dY,theCos,-theSin);

  //  Return NO if segment C-D doesn’t cross line A-B.
  if (cY<0. && dY<0. || cY>=0. && dY>=0.) return NO;

  //  Apply the discovered position to the line A-B in the original coordinate system.
  ABpos=dX+(cX-dX)*dY/(dY-cY);

  //  Return YES only if segment C-D crosses line A-B within segment A-B.
  return ABpos>=0. && ABpos<=distAB; }



//  Returns the line intersection point in x,y, unless that point can’t be determined, in which case returns NO.
//
//  Note:  This function does not check for zero-length line segments -- do not call it with such segments.
//
//  For details on how this works, see the webpage http://alienryderflex.com/intersect

bool linesIntersect(
double aX, double aY,
double bX, double bY,
double cX, double cY,
double dX, double dY,
double *x, double *y) {

  double  distAB, theCos, theSin, abPos ;

  //  Translate the system so that point A is on the origin.
  bX-=aX; bY-=aY;
  cX-=aX; cY-=aY;
  dX-=aX; dY-=aY;

  //  Discover the length of segment A-B.
  distAB=sqrt(bX*bX+bY*bY);

  //  Rotate the system so that point B is on the positive X axis.
  theCos=bX/distAB;
  theSin=bY/distAB;
  rotatePointAroundOrigin(&cX,&cY,theCos,-theSin);
  rotatePointAroundOrigin(&dX,&dY,theCos,-theSin);

  //  Fail if the lines are parallel.
  if (cY==dY) return NO;

  //  Discover the position of the intersection point across line A-B.
  abPos=dX+(cX-dX)*dY/(dY-cY);

  //  Apply the discovered position to line A-B in the original coordinate system.
  *x=aX+abPos*theCos;
  *y=aY+abPos*theSin;

  //  Success.
  return YES; }



//  The math to really do this right is ugly, so instead this just finds a good approximation by
//  testing multiple points across the spline, then hones in to a good approximation of the answer.

double distPointToSpline_HoningMethod(
double testX, double testY, double sX, double sY, double a, double b, double eX, double eY) {

  double  f, minDist2=0., minF=-1., step=.1 ;   //  (minDist2 initialized only to prevent compiler warning)
  int4    i ;

  //  Sample several points to get a rough answer.
  for (f=0.; f<=1.; f+=step) testF(f,&minF,&minDist2,sX,sY,a,b,eX,eY,testX,testY);

  //  Improve the answer by honing in a few times with decreasing half-steps.
  for (i=0; i<7; i++) {
    step*=.5;
    if (!testF(minF+step,&minF,&minDist2,sX,sY,a,b,eX,eY,testX,testY)) {
      testF   (minF-step,&minF,&minDist2,sX,sY,a,b,eX,eY,testX,testY); }}

  return sqrt(minDist2); }



//  This function is a close relative of distPointToSpline_HoningMethod (see that function’s comments), but instead of returning
//  a distance, this function returns two points that indicate the slope of the spline at its nearest point to the test point.

void splineSlopeNearPoint(
double testX, double testY, double sX, double sY, double a, double b, double eX, double eY,
double *slopeSx, double *slopeSy, double *slopeEx, double *slopeEy) {

  double  f, minDist2=0., minF=-1., step=.1 ;   //  (minDist2 initialized only to prevent compiler warning)
  int4    i ;

  //  Sample several points to get a rough answer.
  for (f=0.; f<=1.; f+=step) testF(f,&minF,&minDist2,sX,sY,a,b,eX,eY,testX,testY);

  //  Improve the answer by honing in a few times with decreasing half-steps.
  for (i=0; i<7; i++) {
    step*=.5;
    if (!testF(minF+step,&minF,&minDist2,sX,sY,a,b,eX,eY,testX,testY)) {
      testF   (minF-step,&minF,&minDist2,sX,sY,a,b,eX,eY,testX,testY); }}

  //  Return two points that reasonably represent the slop of the spline at
  //  the point that was found to be (approximately) closest to the test point.
  getSplinePoint(sX,sY,a,b,eX,eY,minF-step,slopeSx,slopeSy);
  getSplinePoint(sX,sY,a,b,eX,eY,minF+step,slopeEx,slopeEy); }



//  Used only by the functions distPointToSpline_HoningMethod and splineSlopeNearPoint.
//
//  Returns YES if a shorter distance was found (which will be stored in minF,minDist2).

bool testF(double f, double *minF, double *minDist2,
double sX, double sY, double a, double b, double eX, double eY, double testX, double testY) {

  double  x, y, dX, dY, dist2 ;

  if (f<0. || f>1.) return NO;

  getSplinePoint(sX,sY,a,b,eX,eY,f,&x,&y);
  dX=x-testX;
  dY=y-testY; dist2=dX*dX+dY*dY;

  if ((*minF)<0. || (*minDist2)>dist2) {
    (  *minF)=f;    (*minDist2)=dist2; return YES; }
  return NO; }



//  Returns an x,y point on a spline curve as specified by a fraction
//  (“frac”) of how far across that curve the point is desired to be.

void getSplinePoint(double sX, double sY, double a, double b, double eX, double eY, double frac, double *x, double *y) {

  double  c, d, e, f ;

  c =sX+frac*(a-sX);
  d =sY+frac*(b-sY);
  e = a-frac*(a-eX);
  f = b-frac*(b-eY);
  *x= c+frac*(e- c);
  *y= d+frac*(f- d); }



//  Important:  The Y axis goes up, not down -- i.e. T>B.

bool inDoubleRect(DoubleRect_ rect, double x, double y) {
  return x>=rect.L
  &&     y< rect.T
  &&     x< rect.R
  &&     y>=rect.B; }


//  Conversion functions for going from image-pixel coordinates to movie-screen coordinates or vice-versa.
//  See the file “Coordinate Systems”.

double pixelToScreenX(double  pixelX) {
  return    pixelX                            /(double) ImageWid    *MOVIE_SCREEN_WID-MOVIE_SCREEN_WID/2. ; }

double pixelToScreenY(double  pixelY) {
  return -((pixelY-(double) ImageHeiLetterBox)/(double) ImageHeiUsed*MOVIE_SCREEN_HEI-MOVIE_SCREEN_HEI/2.); }

double screenToPixelX(double screenX) {
  return                       (screenX+MOVIE_SCREEN_WID/2.)/MOVIE_SCREEN_WID*(double) ImageWid                                ; }

double screenToPixelY(double screenY) {
  return (double) ImageHei-1.-((screenY+MOVIE_SCREEN_HEI/2.)/MOVIE_SCREEN_HEI*(double) ImageHeiUsed+(double) ImageHeiLetterBox); }



//  Rotates a point around the origin by a specified angle.
//
//  To rotate backwards, flip the sign of theSin (i.e. multiply it by -1 before passing it).

void rotatePointAroundOrigin(double *x, double *y, double theCos, double theSin) {
  double  newX=(*x)*theCos-(*y)*theSin ;
  *y          =(*y)*theCos+(*x)*theSin ; *x=newX; }
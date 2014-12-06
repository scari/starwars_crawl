//  Star Wars TSG
//  Darel Rex Finley, 2006-2011



#include                    "bool.h"

#include "DistributorSearchlights.h"



double SearchlightX          [SEARCHLIGHT_COUNT]={   //  X-coordinate of the point around which the searchlight rotates.
  -.378, -.133, .143, .56              }
;
double SearchlightY          [SEARCHLIGHT_COUNT]={   //  Y-coordinate of the point around which the searchlight rotates.
  -.66667, -.66667, -.66667, -.198991  }
;
double SearchlightAmplitude  [SEARCHLIGHT_COUNT]={   //  How far the searchlight swings from its center angle (1 = all the way around the circle).
  .0486, .0833, .0472, 0               }
;
double SearchlightCenterAngle[SEARCHLIGHT_COUNT]={   //  Angle around which the searchlight swings (0 = straight to the right, .25 = straight up, .5 = straight to the left, etc.).
  .285, .25, .214, .3                  }
;
double SearchlightPeriod     [SEARCHLIGHT_COUNT]={   //  Swing’s full cycle time in seconds.
  6.67, 6.67, 6.67, 1                  }
;
double SearchlightPhase      [SEARCHLIGHT_COUNT]={   //  Where in the swing the searchlight starts (1 = a full cycle of back-and-forth motion).
  .401, .0558, .825, 0                 }
;
bool SearchlightNear         [SEARCHLIGHT_COUNT]={   //  Whether or not the searchlight beam is in front of the Distributor logo.
  NO, NO, NO, YES                      }
;
double SearchlightInnerSize  [SEARCHLIGHT_COUNT]={   //  How wide the searchlight beam is.
  .011, .011, .011, .02667             }
;
double SearchlightOuterSize  [SEARCHLIGHT_COUNT]={   //  How wide the searchlight beam’s aura is.
  .0277667, .0277667, .0277667, .06667 }
;
double SearchlightExpansion  [SEARCHLIGHT_COUNT]={   //  How fast the searchlight beam expands in width as it shoots out from its source.
  .0075, .0075, .0075, .0125           }
;
double SearchlightIntensity  [SEARCHLIGHT_COUNT]={   //  How bright the searchlight beam is.
  .125, .125, .125, .167               }
;
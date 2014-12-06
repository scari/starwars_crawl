//  Star Wars TSG
//  Darel Rex Finley, 2006-2011



//  Note:  This subclass is needed because the controlTextDidBeginEditing notification does not fire until the
//         user starts typing.  There does not appear to be any notification for a text field gaining input
//         focus; hence this subclass is used to artificially create one.
//
//         Another way to do it is to have an NSTimer fire regularly (say, ten times per second) and perform the
//         same check as in becomeFirstResponder below, but that would cause the app to use a small amount of
//         processor even when totally idle, which is frowned upon.



#import         "StarWarsTSGController.h"
#import                   "definitions.h"

#import "NSTextFieldWithFocusDetection.h"



@implementation NSTextFieldWithFocusDetection



- (BOOL) becomeFirstResponder {

  BOOL  superResult=[super becomeFirstResponder] ;

  [[self delegate] checkForFieldFocusChanges];

  return superResult; }



@end
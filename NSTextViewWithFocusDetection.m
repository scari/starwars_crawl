//  Star Wars TSG
//  Darel Rex Finley, 2006-2011



//  Note:  This subclass is needed because the controlTextDidBeginEditing notification does not fire until the
//         user starts typing.  There does not appear to be any notification for a text field gaining input
//         focus; hence this subclass is used to artificially create one.
//
//         Another way to do that is to have an NSTimer fire regularly (say, ten times per second) and perform the
//         same check as in becomeFirstResponder below, but that would cause the app to use a small amount of
//         processor even when totally idle, which is frowned upon.
//
//  Note:  Another reason for this subclass is to intercept tab-key presses and make them move-to-next-responder
//         instead of inserting a tab character into the text itself.
//
//         Another way to do that is to use NSTextView’s setFieldEditor method -- but then the Return key would
//         not insert a Return character into the text (which it needs to do).



#import        "StarWarsTSGController.h"
#import                  "definitions.h"

#import "NSTextViewWithFocusDetection.h"



@implementation NSTextViewWithFocusDetection



//  Note:  This method calls the superclass *before* notifying the delegate, to ensure that the change in
//         field focus has occurred and can be properly detected by the delegate.

- (BOOL) becomeFirstResponder {

  BOOL  superResult=[super becomeFirstResponder] ;

  [[self delegate] checkForFieldFocusChanges];

  return superResult; }



- (void) keyDown:(NSEvent *) theEvent {

  if (![[theEvent charactersIgnoringModifiers] length]) return;

  if ( [[theEvent charactersIgnoringModifiers] characterAtIndex:0]==      TAB_CHAR) {
    [[self delegate] moveToNextResponder:self];
    return; }
  if ( [[theEvent charactersIgnoringModifiers] characterAtIndex:0]==SHIFT_TAB_CHAR) {
    [[self delegate] moveToPrevResponder:self];
    return; }

  //  Handle the keypress normally.
  [super keyDown:theEvent]; }



@end
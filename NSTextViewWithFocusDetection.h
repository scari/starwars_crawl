//  Star Wars TSG
//  Darel Rex Finley, 2006-2011

#import <Cocoa/Cocoa.h>

@interface NSTextViewWithFocusDetection : NSTextView

{
  }

- (BOOL) becomeFirstResponder;
- (void) keyDown:(NSEvent *) theEvent;

@end
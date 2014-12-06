//  Star Wars TSG
//  Darel Rex Finley, 2006-2011



#include             "bool.h"
#include              "nil.h"
#include      "definitions.h"
#include "string functions.h"

#include             "undo.h"



//  Variables that hold information about the undoable actions.
//
//  Note:  The Undo array is a circular, wraparound array.  UndoStart and UndoEnd indicate where it starts and
//         ends.

short      UndoStart=0, UndoIndex=0, UndoEnd=0, UndoCount=0, RedoCount=0 ;
UndoItem_  Undo[UNDO_MAX] ;



//  Clears out the Undo array and prepares it for use.

void resetUndoArray() {
  UndoStart=0; UndoIndex=0; UndoEnd=0; UndoCount=0; RedoCount=0; }



//  Push an undoable action onto the Undo array.

void pushUndo(short type, short dataOld, short dataNew, int4 *textOld, int4 *textNew) {

  short  i ;

  //  If the undoable action appears to be a continuation of the previous undoable action, just merge
  //  them and exit.
  if (UndoCount && !dataOld && !dataNew) {
    i=UndoIndex-1; if (i<0) i=UNDO_MAX-1;
    if (Undo[i].type==type && !Undo[i].dataOld && !Undo[i].dataNew && !Undo[i].saveAfter
    &&    int4StrEqual(&Undo[i].textNew[0],textOld)
    &&   !int4StrEqual(&Undo[i].textOld[0],textNew)) {
      copyTextLong(&Undo[i].textNew[0],textNew,TEXT_FIELD_MAX);
      return; }}

  //  Put information about the undoable action into the undo array.
  Undo[UndoIndex].type     =type   ;
  Undo[UndoIndex].saveAfter=NO     ;
  Undo[UndoIndex].dataOld  =dataOld; copyTextLong(&Undo[UndoIndex].textOld[0],textOld,TEXT_FIELD_MAX);
  Undo[UndoIndex].dataNew  =dataNew; copyTextLong(&Undo[UndoIndex].textNew[0],textNew,TEXT_FIELD_MAX);

  //  Update the pointers and counts.
  if (++UndoIndex==UNDO_MAX) UndoIndex=0;
  UndoEnd=UndoIndex; RedoCount=0;
  if (++UndoCount> UNDO_MAX) {
    UndoCount--; UndoStart=UndoEnd; }}



//  Pull an undoable action off the Undo array.

void pullUndo(short *type, short *data, int4 *text) {

  //  If there are no actions to pull, return a “none” signifier.
  if (!UndoCount) {
    *type=UNDO_NONE; return; }

  //  Update the pointers and counts.
  UndoCount--; RedoCount++; if (--UndoIndex<0) UndoIndex=UNDO_MAX-1;

  //  Retrieve from the undo array the requested information about the undoable action.
  *type=Undo[UndoIndex].type   ;
  *data=Undo[UndoIndex].dataOld; copyTextLong(text,&Undo[UndoIndex].textOld[0],TEXT_FIELD_MAX); }



//  Pull a redoable action off the Undo array.

void pullRedo(short *type, short *data, int4 *text) {

  //  If there are no redoable actions to pull, return a “none” signifier.
  if (!RedoCount) {
    *type=UNDO_NONE; return; }

  //  Retrieve from the undo array the requested information about the redoable action.
  *type=Undo[UndoIndex].type   ;
  *data=Undo[UndoIndex].dataNew; copyTextLong(text,&Undo[UndoIndex].textNew[0],TEXT_FIELD_MAX);

  //  Update the pointers and counts.
  UndoCount++; RedoCount--; if (++UndoIndex==UNDO_MAX) UndoIndex=0; }



//  Flag the last undoable action to indicate that the file was saved immediately after that action.

void pushSaveMarkerOntoUndoArray() {

  short  i ;

  //  Do nothing if there is no undoable action on the array.
  if (!UndoCount) return;

  //  Get index of undo item that is ready to be undone.
  i=UndoIndex-1; if (i<0) i=UNDO_MAX-1;

  //  Tag the undoable action with a save-marker.
  Undo[i].saveAfter=YES; }



//  Find out if the undoable action that is ready to be undone has its save-after flag set.  Returns NO if not,
//  or if there is no undoable action available to be undone.

bool lastUndoActionHasSaveAfter() {

  short  i ;

  //  Return NO if there is no undoable action on the array.
  if (!UndoCount) return NO;

  //  Get index of undo item that is ready to be undone.
  i=UndoIndex; if (--i<0) i=UNDO_MAX-1;

  //  Return the status of the save-marker flag.
  return Undo[i].saveAfter; }
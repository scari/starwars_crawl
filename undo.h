//  Star Wars TSG
//  Darel Rex Finley, 2006-2011

extern short      UndoStart, UndoIndex, UndoEnd, UndoCount, RedoCount ;
extern UndoItem_  Undo[UNDO_MAX] ;

void resetUndoArray             ();
void pushUndo                   (short type, short dataOld, short dataNew, int4 *textOld, int4 *textNew);
void pullUndo                   (short *type, short *data, int4 *text);
void pullRedo                   (short *type, short *data, int4 *text);
void pushSaveMarkerOntoUndoArray();
bool lastUndoActionHasSaveAfter ();
//  Star Wars TSG
//  Darel Rex Finley, 2006-2011

extern double  SequenceTimes[] ;

extern int4  SequenceTimes_elementCount ;

BYTE   *verifySequenceTimes(double *totalTime);
double  getSequenceTime    (int4 i);
void    setSequenceTime    (int4 i, double val);
//  Star Wars TSG
//  Darel Rex Finley, 2006-2011

bool correctBitmapFontEndianness   (BYTE *font, int4 fontLen);
void correctBitmapFontMissingGlyphs(BYTE *font);
int4 cbfHeight                     (BYTE *font);
int4 cbfWidth                      (BYTE *font, int4 *c);
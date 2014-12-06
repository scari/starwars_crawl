//  Star Wars TSG
//  Darel Rex Finley, 2006-2011

void fillImage                           (short r, short g, short b);
void copyImageToBuffer                   (BYTE *buf);
void copyBufferToImageWithSubtractiveFade(BYTE *buf, BYTE subtractVal);
void copyBufferToImage                   (BYTE *buf);
void copyBufferToImageWithMultFade       (BYTE *buf, double fadeFrac);
void sortNodeRow                         (int4 subPixelRow, int4 *nodes, double *nodeX, int4 *polyTag);
bool emptyRenderRowBuffers               (RenderRow_ *renderRow, int4 pixelBuffer, int4 pixelBufferRasterSeg, int4 pixelX, int4 rasterLoHi);
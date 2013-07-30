#ifndef BLOCKBUSTER_CONVERT_H
#define BLOCKBUSTER_CONVERT_H

#include "frames.h"
/* Conversion utilities from convert.c */
Image *ConvertImageToFormat(const Image *image, ImageFormat *canvasFormat);
Image *ScaleImage(const Image *image, 
                  int srcX, int srcY, int srcWidth, int srcHeight,
                  int zoomedWidth, int zoomedHeight);


#endif

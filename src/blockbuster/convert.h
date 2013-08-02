#ifndef BLOCKBUSTER_CONVERT_H
#define BLOCKBUSTER_CONVERT_H

#include "frames.h"
/* Conversion utilities from convert.c */
ImagePtr ConvertImageToFormat(ImagePtr image, ImageFormat *canvasFormat);
ImagePtr ScaleImage(ImagePtr image, 
                  int srcX, int srcY, int srcWidth, int srcHeight,
                  int zoomedWidth, int zoomedHeight);


#endif

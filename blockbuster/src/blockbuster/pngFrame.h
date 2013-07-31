#ifndef PNG_FRAME_H
#define PNG_FRAME_H 1

#include "frames.h"
#include <png.h>

FrameList *pngGetFrameList(const char *filename);

int PrepPng(const char *filename, 
              FILE **fPtr, png_structp *readStructPtr, 
              png_infop *infoStructPtr);
//============================================================
struct PNGFrameInfo: public FrameInfo {
  
  virtual int LoadImage(ImageFormat *fmt, 
                        const Rectangle *region, 
                        int lod) {
    return 0; 
  }

   int PrepPng(const char *filename, 
              FILE **fPtr, png_structp *readStructPtr, 
               png_infop *infoStructPtr) {
     return 0; 
   }
 

}; 



#endif

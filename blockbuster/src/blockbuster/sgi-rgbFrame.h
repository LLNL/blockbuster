#ifndef SGIFRAME_H
#define SGIFRAME_H
#include "frames.h"

FrameList *sgirgbGetFrameList(const char *filename);

//============================================================
struct SGIFrameInfo: public FrameInfo {
  
  virtual int LoadImage(ImageFormat *fmt, 
                        const Rectangle *region, 
                        int lod) {
    return 0; 
  }

 
}; 

#endif

#ifndef BBPNM_H
#define BBPNM_H
#include "frames.h"

//============================================================
struct PNMFrameInfo: public FrameInfo {
  
  virtual int LoadImage(ImageFormat */*fmt*/, 
                        const Rectangle */*region*/, 
                        int /*lod*/) {
    return 0; 
  }
  
}; 


FrameList *pnmGetFrameList(const char *filename);
#endif

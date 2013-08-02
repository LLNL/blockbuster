#ifndef BBPNM_H
#define BBPNM_H
#include "frames.h"

//============================================================
FrameListPtr pnmGetFrameList(const char *filename);

struct PNMFrameInfo: public FrameInfo {
  PNMFrameInfo(string fname); 
  
  virtual int LoadImage(ImagePtr , ImageFormat */*fmt*/, 
                        const Rectangle */*region*/, 
                        int /*lod*/);
  
}; 


#endif

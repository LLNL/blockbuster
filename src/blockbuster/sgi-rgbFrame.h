#ifndef SGIFRAME_H
#define SGIFRAME_H
#include "frames.h"

FrameListPtr sgirgbGetFrameList(const char *filename);

//============================================================
struct SGIFrameInfo: public FrameInfo {
 // ----------------------------------------------
  SGIFrameInfo(std::string fname); 

 // ----------------------------------------------
  virtual int LoadImage(ImagePtr, ImageFormat */*fmt*/, 
                        const Rectangle */*region*/, 
                        int /*lod*/);

  // ----------------------------------------------
  SGIFrameInfo(const SGIFrameInfo &other) :FrameInfo(other) {
    return; 
  }
  
  // ----------------------------------------------
  virtual SGIFrameInfo* Clone()  const{
    return new SGIFrameInfo(*this); 
  }
 
}; 

#endif

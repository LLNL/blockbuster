#ifndef PNG_FRAME_H
#define PNG_FRAME_H 1

#include "frames.h"
#include <png.h>

FrameListPtr pngGetFrameList(const char *filename);

//============================================================
struct PNGFrameInfo: public FrameInfo {
  
  // ----------------------------------------------
  PNGFrameInfo(string fname);
  
  // ----------------------------------------------
  PNGFrameInfo(const PNGFrameInfo &other) :FrameInfo(other) {
    return; 
  }
  
  // ----------------------------------------------
  virtual PNGFrameInfo* Clone()  const{
    return new PNGFrameInfo(*this); 
  }
  
  // ----------------------------------------------
  int PrepPng(string filename, 
              FILE **/*fPtr*/, png_structp */*readStructPtr*/, 
              png_infop */*infoStructPtr*/) ;
  

  // ----------------------------------------------
  virtual int LoadImage(ImagePtr, ImageFormat */*fmt*/, 
                        const Rectangle */*region*/, 
                        int /*lod*/);



}; 



#endif

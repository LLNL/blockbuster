#ifndef BBSM_H
#define BBSM_H
#include "frames.h"
#include "sm/smBase.h"

FrameListPtr smGetFrameList(const char *filename);

typedef boost::shared_ptr<struct SMFrameInfo> SMFrameInfoPtr; 

typedef boost::shared_ptr<smBase> smBasePtr; 

//============================================================
struct SMFrameInfo: public FrameInfo {
  
  SMFrameInfo(string fname, int w, int h, int d, int lod, uint32_t smframe, smBasePtr sm): FrameInfo(fname, w,h,d,lod,smframe), mSM(sm) {
    return; 
  }
  
  virtual ~SMFrameInfo() {
    return; 
  }
  
  virtual int LoadImage(ImagePtr image, ImageFormat */*fmt*/, 
                        const Rectangle */*region*/, 
                        int /*lod*/);

  // ----------------------------------------------
  SMFrameInfo(const SMFrameInfo &other) :
    FrameInfo(other), mSM(other.mSM) {
    return; 
  }
  
  // ----------------------------------------------
  virtual SMFrameInfo* Clone()  const{
    return new SMFrameInfo(*this); 
  }

  smBasePtr mSM; 
 
}; 


#endif

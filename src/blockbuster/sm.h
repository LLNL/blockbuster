#ifndef BBSM_H
#define BBSM_H
#include "frames.h"
#include "sm/smBase.h"

FrameList *smGetFrameList(const char *filename);

typedef boost::shared_ptr<struct SMFrameInfo> SMFrameInfoPtr; 


//============================================================
struct SMFrameInfo: public FrameInfo {
  SMFrameInfo(int w, int h, int d, int lod, string fname, uint32_t frame, boost::shared_ptr<smBase> sm):
    FrameInfo(w,h,d,lod,fname, frame), mSM(sm) {
    return; 
  }
  
  virtual ~SMFrameInfo() {
    return; 
  }
  
  virtual int LoadImage(ImageFormat *fmt, 
                        const Rectangle *region, 
                        int lod) {
    return 0; 
  }

  boost::shared_ptr<smBase> mSM; 
 
}; 


#endif

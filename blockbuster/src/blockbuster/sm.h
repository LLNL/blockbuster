#ifndef BBSM_H
#define BBSM_H
#include "frames.h"
#include "sm/smBase.h"

FrameList *smGetFrameList(const char *filename);

typedef boost::shared_ptr<struct SMFrameInfo> SMFrameInfoPtr; 

//============================================================
struct SMImage: public Image {
  SMImage(boost::shared_ptr<smBase>sm): Image(), mSM(sm){
    mTypeName = "SM"; 
    return; 
  }

  virtual ~SMImage() {
    return; 
  }
    
  virtual int LoadImage(FrameInfoPtr,
                        ImageFormat *, 
                        const Rectangle *, 
                        int ) {
    return 0; 
  }

  boost::shared_ptr<smBase> mSM; 
}; 
 
//============================================================
struct SMFrameInfo: public FrameInfo {
  SMFrameInfo(int w, int h, int d, int lod, string fname, uint32_t frame, boost::shared_ptr<smBase> sm):
    FrameInfo(w,h,d,lod,fname, frame), mSM(sm) {
    mImage = new SMImage(sm); 
    return; 
  }
  
  virtual ~SMFrameInfo() {
    return; 
  }
  
  boost::shared_ptr<smBase> mSM; 
 
}; 


#endif

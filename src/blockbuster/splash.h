#ifndef BLOCKBUSTER_SPLASH_H
#define BLOCKBUSTER_SPLASH_H

#include "frames.h"

/* Splash Screen functions from splash.c */
extern FrameListPtr splashScreenFrameListPtr;

struct SplashFrameInfo: public FrameInfo {
  SplashFrameInfo(): FrameInfo() {
    return; 
  }

  int LoadImage(ImagePtr image,
                ImageFormat *, const Rectangle *, int );
  
  // ----------------------------------------------
  SplashFrameInfo(const SplashFrameInfo &other) :FrameInfo(other) {
    return; 
  }
  
  // ----------------------------------------------
  virtual SplashFrameInfo* Clone()  const{
    return new SplashFrameInfo(*this); 
  }
}; 



#endif

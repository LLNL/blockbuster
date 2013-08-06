#ifndef BLOCKBUSTER_SPLASH_H
#define BLOCKBUSTER_SPLASH_H

#include "frames.h"

/* Splash Screen functions from splash.c */
extern FrameListPtr splashScreenFrameListPtr;

struct SplashFrameInfo: public FrameInfo {
  int LoadImage(ImagePtr image,
                ImageFormat *, const Rectangle *, int );
  
}; 



#endif

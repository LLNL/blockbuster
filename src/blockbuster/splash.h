#ifndef BLOCKBUSTER_SPLASH_H
#define BLOCKBUSTER_SPLASH_H


/* Splash Screen functions from splash.c */
extern FrameListPtr splashScreenFrameListPtr;

struct SplashFrameInfo: public FrameInfo {
  int LoadImage(ImagePtr image,
                ImageFormat *, const Rectangle *, int );
  
}; 



#endif

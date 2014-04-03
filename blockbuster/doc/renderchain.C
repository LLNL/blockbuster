#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <string>
#include <GL/gl.h>
#include <GL/glx.h>
#include <iostream> 

#include <X11/extensions/XTest.h>
#include <X11/extensions/Xinerama.h>

using namespace std; 
void errexit(std::string msg) {
  cerr << "ERROR: " << msg << endl; 
  exit(1); 
}

int main(int argc, char *argv[]) {
  char *displayName = getenv("DISPLAY"); 
  // BeginXWindowInit
  Display *mDisplay = XOpenDisplay(displayName);

  // ChooseVisual

  static int attributes[] = {
    GLX_USE_GL, GLX_RGBA, GLX_DOUBLEBUFFER,
    GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1,
    None
  }, stereoAttributes[] = {
    GLX_USE_GL, GLX_RGBA, GLX_DOUBLEBUFFER, GLX_STEREO,
    GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1,
    None
  };

  XVisualInfo *mVisualInfo = glXChooseVisual(mDisplay, 0, attributes);
  if (mVisualInfo == NULL) {
    errexit("cannot find a GLX visual on to create new OpenGL window"); 
  }
  XVisualInfo *mStereoVisualInfo = NULL; 
  GLboolean mHaveStereo; 
  mStereoVisualInfo = glXChooseVisual(mDisplay, 0, stereoAttributes);
  if (mStereoVisualInfo == NULL) {
    errexit("could not create stereo visual on display");  
  }

  // FinishXWindowInit
  Screen *screen= ScreenOfDisplay(mDisplay, 0);
  XSetWindowAttributes windowAttrs;
  unsigned long windowAttrsMask;
  Window mParentWindow = RootWindow(mDisplay, 0); 
  Colormap mColormap = XCreateColormap(mDisplay, mParentWindow,
                              mVisualInfo->visual, AllocNone);
  windowAttrsMask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
  windowAttrs.background_pixel = 0x0;
  windowAttrs.border_pixel = 0x0;
  windowAttrs.colormap = mColormap;
  windowAttrs.event_mask = StructureNotifyMask | ExposureMask;

  windowAttrs.event_mask |= (KeyPressMask | ButtonPressMask |
                             ButtonReleaseMask | ButtonMotionMask);

  windowAttrs.override_redirect = True; // does not seem to do anything. :-(

  Window mWindow = XCreateWindow(mDisplay, mParentWindow,
                                 20,20, 500, 500,
                          20, mVisualInfo->depth, InputOutput,
                          mVisualInfo->visual, windowAttrsMask, &windowAttrs);

  XSizeHints sizeHints;
  sizeHints.flags = USSize;
  sizeHints.width = sizeHints.base_width = 500; 
  sizeHints.height = sizeHints.base_height = 500; 
  sizeHints.flags |= PMinSize;
  sizeHints.min_width = 500;
  sizeHints.min_height = 500;

  sizeHints.flags |= USPosition;

  XSetStandardProperties(mDisplay, mWindow, "My Window", "My Window", 
                         None, (char **)NULL, 0, &sizeHints);

  XMapWindow(mDisplay, mWindow);
  XSync( mDisplay, false ); 


  // FinishRendererInit  
  GLXContext mContext = glXCreateContext(mDisplay, mVisualInfo,
                              NULL, GL_TRUE);
  XSync( mDisplay, false ); 
  if (!mContext) {
    errexit("Could not create basic gl context"); 
  }
  GLXContext mStereoContext = glXCreateContext(mDisplay, mStereoVisualInfo,
                                    NULL, GL_TRUE);
  XSync( mDisplay, false ); 
  if (!mStereoContext) {
    errexit("Could not create stereo gl context"); 
  }

  if (!glXMakeCurrent(mDisplay, mWindow, mStereoContext)) { // SUCCESS
    errexit("could not enable stereo context"); 
  }
  XSync( mDisplay, false ); 
  if (!glXMakeCurrent(mDisplay, mWindow, mContext)) {  // FAILURE -- ABORTS
    errexit("could not enable mono context"); 
  }
  XSync( mDisplay, false ); 
  if (!glXMakeCurrent(mDisplay, mWindow, mStereoContext)) {
    errexit("could not re-enable stereo context"); 
  }
  XSync( mDisplay, false ); 

  return 0; 
}

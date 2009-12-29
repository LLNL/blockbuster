#ifndef BB_RENDERER_H
#define BB_RENDERER_H
#include "cache.h"
#include "settings.h"
struct Canvas; 
/* Base class for all other renderers, defining the API required */ 
// factory function

class NewRenderer {
 public:
  static NewRenderer *CreateRenderer(ProgramOptions*, Canvas*);
  NewRenderer(ProgramOptions *opt, Canvas *canvas);

  virtual ~NewRenderer() {
    return; 
  } // replaces DestroyRenderer from Canvas

  // Renderers:  x11, gl, gl_stereo, gltexture, and dmx
  // NewRenderers:  x11Renderer, glRenderer, glStereoRenderer, glTextureRenderer, dmxRenderer  
  
  /* Functions that are function pointers in the Canvas class right now */ 
  /* The fundamental operation of the Renderer is to render.        This might be assigned gl_Render (gl.cpp, gl_Initialize), x11_Render (x11.cpp: x11_initialize()), or dmx_Render (dmxglue.cpp, dmx_Initialize()).  The assignment is done 
   */
  // from Canvas class 
  virtual void Render(int frameNumber,
                      const Rectangle *imageRegion,
                      int destX, int destY, float zoom, int lod) = 0;
  // from Canvas class 
  virtual void SetFrameList(FrameList *frameList) {
    mFrameList = frameList; 
  }
    
  // from Canvas class 
  virtual void Preload(uint32_t frameNumber,
                       const Rectangle *imageRegion, uint32_t levelOfDetail);


  //void *rendererPrivateData; // from Canvas class -- shouldn't need this
  
  /* This becomes the constructor: */ 
  //virtual MovieStatus Initialize(struct Canvas *canvas, const ProgramOptions *options) = 0; */
  /*  This becomes the destructor: 
   */
  // virtual void DestroyRenderer(struct Canvas *canvas) = 0;
  
  // from xwindow.cpp: RendererSpecificGlue
  /* XVisualInfo *ChooseVisual(Display *display, int screenNumber);
     MovieStatus FinishInitialization(Canvas *canvas, const ProgramOptions *options);
     void DestroyGlue(Canvas *canvas);
     void DrawString(Canvas *canvas, int row, int column, const char *str);
     void BeforeRender(Canvas *canvas);
     void AfterRender(Canvas *canvas);
     void SwapBuffers(Canvas *canvas);
  */

 protected:   
  
  // these are good to have around to reduce arguments: 
  Canvas *mCanvas; 
  ProgramOptions *mOptions; 


  FrameList *mFrameList;
  ImageCache *mCache; // if not using DMX
} ;

#endif

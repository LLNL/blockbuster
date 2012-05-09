#ifndef BB_RENDERER_H
#define BB_RENDERER_H
#include "cache.h"
#include "settings.h"
#include "xwindow.h"

struct Canvas; 
/* Base class for all other renderers, defining the API required */ 
// factory function
// Renderers: x11Renderer(broken), glRenderer, glStereoRenderer, glTextureRenderer, dmxRenderer

class Renderer: public XWindow {
 public:
  static Renderer *CreateRenderer(ProgramOptions*, Canvas*, Window);
  Renderer(ProgramOptions *opt, Canvas *canvas, Window parentWindow, QString name);

  virtual XVisualInfo *ChooseVisual(void) = 0;
  void FinishInit(ProgramOptions *opt, Canvas *canvas, Window parentWindow);
  virtual void FinishRendererInit(ProgramOptions *opt, Canvas *canvas, Window parentWindow) =0; 
  virtual ~Renderer() {
    DestroyImageCache(); 
    return; 
  } // replaces DestroyRenderer from Canvas

  // the following depend on whether DMX is being used: 
  virtual void DestroyImageCache(void)
  {
    if (mCache) delete mCache; 
    mCache = NULL; 
    return; 
  }

  virtual Image *GetImage(uint32_t frameNumber,
                  const Rectangle *newRegion, uint32_t levelOfDetail){
    return mCache->GetImage(frameNumber, newRegion, levelOfDetail); 
  }

  virtual void ReleaseImage(Image *image) {
    mCache->ReleaseImage(image); 
  }
  
  // The fundamental operation of the Renderer is to render.     
  virtual void Render(int frameNumber,
                      const Rectangle *imageRegion,
                      int destX, int destY, float zoom, int lod) = 0;

  virtual void SetFrameList(FrameList *frameList) ;
    
  // from Canvas class 
  virtual void Preload(uint32_t frameNumber,
                       const Rectangle *imageRegion, uint32_t levelOfDetail);

  // this calls Preload.  It's not overloaded for DMX therefore. 
  void Preload(uint32_t frameNumber, uint32_t preloadFrames, 
               int playDirection, uint32_t minFrame, uint32_t maxFrame,
               const Rectangle *imageRegion, uint32_t levelOfDetail);
  
 public:
  QString mName; 
  FrameList *mFrameList;
  ImageCache *mCache; // if not using DMX

 protected:   
  // these are good to have around to reduce arguments: 
  Canvas *mCanvas; 
  ProgramOptions *mOptions; 


} ;

#endif

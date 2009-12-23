#ifndef GLRENDERER_H
#define GLRENDERER_H yes

#include "Renderer.h" // not "Renderers.h"

/* Base GL rendering class for all other GL renderers */ 
class glRenderer: public NewRenderer {
 public:
  glRenderer(ProgramOptions *opt, Canvas *canvas):
    NewRenderer(opt, canvas) {
    return; 
  }
  virtual ~glRenderer() {
    return; 
  }
  virtual void HandleOptions(int &argc, char *argv[]);

  virtual void Render(int frameNumber,
                      const Rectangle *imageRegion,
                      int destX, int destY, float zoom, int lod);  
   
} ;

class glStereoRenderer: public glRenderer {
  glStereoRenderer(ProgramOptions *opt, Canvas *canvas):
    glRenderer(opt, canvas) {
    return; 
  }
  ~glStereoRenderer() {}
  void Render(int frameNumber,
              const Rectangle *imageRegion,
              int destX, int destY, float zoom, int lod);
};



class glTextureRenderer: public glRenderer {
  glTextureRenderer(ProgramOptions *opt, Canvas *canvas):
    glRenderer(opt, canvas) {
    return; 
  }
  ~glTextureRenderer() {}
  
  // this is different for textures: 
  void HandleOptions(int &argc, char *argv[]);
  
}; 


#endif

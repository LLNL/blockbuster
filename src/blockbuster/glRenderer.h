#ifndef GLRENDERER_H
#define GLRENDERER_H yes

#include "Renderer.h" // not "Renderers.h"
#include <GL/gl.h>
#include <GL/glx.h>

/* Base GL rendering class for all other GL renderers */ 
class glRenderer: public NewRenderer {
 public:
  glRenderer(ProgramOptions *opt, Canvas *canvas, QString name="gl");

  virtual ~glRenderer() ;
  // this is a no-op for glRenderers: 
  // virtual void HandleOptions(int &argc, char *argv[]);

  virtual void Render(int frameNumber,
                      const Rectangle *imageRegion,
                      int destX, int destY, float zoom, int lod);  
   
  bool mXSynchronize; // for debugging, presumably. 

  // from WindowInfo struct in xwindow.cpp  
  GLXContext context;
  GLuint fontBase;
} ;

class glStereoRenderer: public glRenderer {
 public:
  glStereoRenderer(ProgramOptions *opt, Canvas *canvas):
    glRenderer(opt, canvas, "gl_stereo") {
    return; 
  }
  virtual ~glStereoRenderer() {}
  virtual void Render(int frameNumber,
              const Rectangle *imageRegion,
              int destX, int destY, float zoom, int lod);
};



class glTextureRenderer: public glRenderer {
 public:
  glTextureRenderer(ProgramOptions *opt, Canvas *canvas);
  virtual ~glTextureRenderer() {}
  
  
}; 


#endif

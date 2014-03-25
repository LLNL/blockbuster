#ifndef GLRENDERER_H
#define GLRENDERER_H yes

#include "Renderer.h" // not "Renderers.h"
#include <GL/gl.h>
#include <GL/glx.h>



/* Base GL rendering class for all other GL renderers */ 
class glRenderer: public Renderer {
 public:
  glRenderer(ProgramOptions *opt):
    Renderer(opt) {
    mName = "gl";   
    return; 
  }
    
    //======================================================================
    virtual ~glRenderer() {
      glXDestroyContext(mDisplay, context);
      return; 
    }


  // ======================================================================
    virtual void BeginRendererInit(void) {
      return; 
    }

  // ======================================================================
  virtual void FinishRendererInit(void);

  virtual XVisualInfo *ChooseVisual(void);  
  virtual void DrawString(int row, int column, const char *str);

  virtual void RenderActual(int frameNumber,
                            RectanglePtr imageRegion,
                            int destX, int destY, float zoom, int lod);  
  virtual void SwapBuffers(void);
   
  bool mXSynchronize; // for debugging, presumably. 

  // from WindowInfo struct in xwindow.cpp  
  GLXContext context;
  GLuint fontBase;
  vector<float> timeSamples; 
} ;

// ==================================================================
// glStereoRenderer
// ==================================================================
class glStereoRenderer: public glRenderer {
 public:
  glStereoRenderer(ProgramOptions *opt):
    glRenderer(opt) {
    mName = "gl_stereo";  
    mStereo = true; 
    return; 
  }
  virtual ~glStereoRenderer() {}


  virtual XVisualInfo *ChooseVisual(void);
  // ======================================================================
   void RenderActual(int frameNumber,  RectanglePtr imageRegion,
              int destX, int destY, float zoom, int lod);

};

// ==================================================================
// glTextureRenderer
// ==================================================================

typedef boost::shared_ptr<struct TextureObject> TextureObjectPtr;

struct TextureObject {
  TextureObject(): 
    texture(0), width(0), height(0), anyLoaded(false) {
    return ; 
  }
  GLuint texture;
  GLuint width, height; /* level zero */
  Rectangle valid[MAX_IMAGE_LEVELS];
  GLboolean anyLoaded;   /* is any part of this texture valid/loaded? */
  //GLuint age;
  FrameInfoPtr frameInfo;  /* back pointer */
} ;


#define MAX_TEXTURES 20

class glTextureRenderer: public glRenderer {
 public:
  glTextureRenderer(ProgramOptions *opt):
    glRenderer(opt) {
    mName ="gltexture"; 
    return; 
  }

  virtual ~glTextureRenderer() {}

  // ======================================================================
  virtual void BeginRendererInit(void); 

  int32_t MinPowerOf2(int x); 
  void RenderActual(int frameNumber, RectanglePtr imageRegion,
                    int destX, int destY, float zoom, int lod);
  TextureObjectPtr GetTextureObject(int frameNumber);
  void UpdateProjectionAndViewport(int newWidth, int newHeight);

  int maxTextureWidth, maxTextureHeight;
  GLenum texFormat, texIntFormat;
  vector<TextureObjectPtr> mTextures;

}; 


#endif

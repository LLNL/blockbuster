#ifndef GLRENDERER_H
#define GLRENDERER_H yes

#include "Renderer.h" // not "Renderers.h"
#include <GL/gl.h>
#include <GL/glx.h>



/* Base GL rendering class for all other GL renderers */ 
class glRenderer: public Renderer {
 public:
  glRenderer(ProgramOptions *opt,  Window parentWindow,
             BlockbusterInterface *gui, QString name="gl");

  virtual void FinishRendererInit(ProgramOptions *opt);
  virtual ~glRenderer() ;

  virtual XVisualInfo *ChooseVisual(void);  
  virtual void DrawString(int row, int column, const char *str);

  virtual void Render(int frameNumber,
                      const Rectangle *imageRegion,
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
  glStereoRenderer(ProgramOptions *opt,  Window parentWindow,
                   BlockbusterInterface *gui, QString name="gl_stereo"):
  glRenderer(opt, parentWindow, gui, name)    {
    return; 
  }
  virtual ~glStereoRenderer() {}
  virtual XVisualInfo *ChooseVisual(void);
  void Render(int frameNumber, const Rectangle *imageRegion,
              int destX, int destY, float zoom, int lod);
};

// ==================================================================
// glTextureRenderer
// ==================================================================

typedef boost::shared_ptr<struct TextureObject> TextureObjectPtr;

struct TextureObject {
    GLuint texture;
    GLuint width, height; /* level zero */
    Rectangle valid[MAX_IMAGE_LEVELS];
    GLboolean anyLoaded;   /* is any part of this texture valid/loaded? */
    GLuint age;
  FrameInfoPtr frameInfo;  /* back pointer */
} ;


#define MAX_TEXTURES 20

class glTextureRenderer: public glRenderer {
 public:
  glTextureRenderer(ProgramOptions *opt,  Window parentWindow,
                    BlockbusterInterface *gui, QString name="gltexture"); 
  virtual ~glTextureRenderer() {}

  int32_t MinPowerOf2(int x); 
  void Render(int frameNumber, const Rectangle *imageRegion,
              int destX, int destY, float zoom, int lod);
  TextureObjectPtr GetTextureObject(int frameNumber);
  void UpdateProjectionAndViewport(int newWidth, int newHeight);

  int maxTextureWidth, maxTextureHeight;
  GLenum texFormat, texIntFormat;
  vector<TextureObjectPtr> mTextures;

}; 


#endif

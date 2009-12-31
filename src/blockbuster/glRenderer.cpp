/* Copyright (c) 2003 Tungsten Graphics, Inc.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files ("the
 * Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:  The above copyright notice, the Tungsten
 * Graphics splash screen, and this permission notice shall be included
 * in all copies or substantial portions of the Software.  THE SOFTWARE
 * IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT
 * SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "xwindow.h"
#include "blockbuster_gl.h"
#include "util.h"
#include "cache.h"
#include "frames.h"
#include "errmsg.h"
#include "errmsg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include "timer.h"
#include "canvas.h"

#include "glRenderer.h"

glRenderer::glRenderer(ProgramOptions *opt, Canvas *canvas, QString name):
  NewRenderer(opt, canvas, name) {

  // from glFinishInitialization: 
  Bool rv;
  Font id = canvas->mXWindow->fontInfo->fid;
  unsigned int first = canvas->mXWindow->fontInfo->min_char_or_byte2;
  unsigned int last = canvas->mXWindow->fontInfo->max_char_or_byte2;
  //  glRenderer *renderer = dynamic_cast<glRenderer*>(canvas->mRenderer); 
  
  /* All GL rendering in X11 requires a glX context. */
  context = glXCreateContext(canvas->mXWindow->display, canvas->mXWindow->visInfo,
                                       NULL, GL_TRUE);
  if (!context) {
    ERROR("couldn't create GLX context");
    return ;
  }
  
  rv = glXMakeCurrent(canvas->mXWindow->display, canvas->mXWindow->window, context);
  if (rv == False) {
    ERROR("couldn't make graphics context current");
    glXDestroyContext(canvas->mXWindow->display, context);
    return ;
  }
  
  DEBUGMSG("GL_RENDERER = %s", (char *) glGetString(GL_RENDERER));
  
  /* OpenGL display list font bitmaps */
  fontBase = glGenLists((GLuint) last + 1);
  if (!fontBase) {
    ERROR("Unable to allocate display lists for fonts");
    glXDestroyContext(canvas->mXWindow->display, context);
    return ;
  }
  
  glXUseXFont(id, first, last - first + 1, fontBase + first);
  glListBase(fontBase);
  
  /* Specify our required format.  For OpenGL, always assume we're
   * getting 24-bit RGB pixels.
   */
  canvas->requiredImageFormat.scanlineByteMultiple = 1;
  canvas->requiredImageFormat.rowOrder = ROW_ORDER_DONT_CARE;
  canvas->requiredImageFormat.byteOrder = MSB_FIRST;
  canvas->requiredImageFormat.bytesPerPixel = 3;
  
  return; 
}

//=============================================================
glRenderer::~glRenderer() {
  glXDestroyContext(mCanvas->mXWindow->display, context);
  return; 
}

//=============================================================
void glRenderer::Render(int frameNumber, 
                        const Rectangle *imageRegion,
                        int destX, int destY, float zoom, int lod){
  int lodScale;
  int localFrameNumber;
  Rectangle region = *imageRegion;
  Image *image;
  int saveSkip;
  DEBUGMSG("gl_Render begin, frame %d, %d x %d  at %d, %d  zoom=%f  lod=%d", 
           frameNumber,
           imageRegion->width, imageRegion->height,
           imageRegion->x, imageRegion->y, zoom, lod);

  Canvas *canvas = NULL;  // POISON -- placeholder to compile, will segfault
  /*
   * Compute possibly reduced-resolution image region to display.
   */
  if (canvas->frameList->stereo) {
        localFrameNumber = frameNumber *2; /* we'll display left frame only */
  }
  else {
        localFrameNumber = frameNumber;
  }

 
  if (lod > canvas->frameList->getFrame(localFrameNumber)->maxLOD) {
    ERROR("Error in glRenderer::Render:  lod is greater than max\n"); 
    abort(); 
  }

  lodScale = 1 << lod;

 
  {
    int rr;

    rr = ROUND_TO_MULTIPLE(imageRegion->x,lodScale);
    if(rr > imageRegion->x) {
      region.x = rr - lodScale;
    }
    region.x /= lodScale;
   
    rr = ROUND_TO_MULTIPLE(imageRegion->y,lodScale);
    if(rr > imageRegion->y) {
      region.y = rr - lodScale;
    }
    region.y /= lodScale;

    rr = ROUND_TO_MULTIPLE(imageRegion->width,lodScale);
    if(rr > imageRegion->width) {
      region.width = rr - lodScale;
    }
    region.width /= lodScale;

    rr = ROUND_TO_MULTIPLE(imageRegion->height,lodScale);
    if(rr > imageRegion->height) {
      region.height = rr - lodScale;
    }

    region.height /= lodScale;
  }
  
  zoom *= (float) lodScale;


  TIMER_PRINT("Pull the image from our cache "); 
  image = canvas->imageCache->GetImage(localFrameNumber, &region, lod);
  TIMER_PRINT("Got image"); 
  if (image == NULL) {
    /* error has already been reported */
    return;
  }

  saveSkip =  image->height - (region.y + region.height);
 

  bb_assert(region.x >= 0);
  bb_assert(region.y >= 0);
  /*bb_assert(region.x + region.width <= image->width); */
  /*bb_assert(region.y + region.height <= image->height);*/

  glViewport(0, 0, canvas->width, canvas->height);


  /* only clear the window if we have to */
  if (destX > 0 || destY > 0 ||
      region.width * zoom < canvas->width ||
      region.height * zoom < canvas->height) {
    glClearColor(0.0, 0.0, 0.0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
  }

  DEBUGMSG(QString("Frame %1 row order is %2\n").arg(frameNumber).arg(image->imageFormat.rowOrder)); 
  
  if (image->imageFormat.rowOrder == BOTTOM_TO_TOP) {
    /*
     * Do adjustments to flip Y axis.
     * Yes, this is tricky to understand.
     */
    destY = canvas->height - static_cast<int32_t>((image->height * zoom) + destY) + static_cast<int32_t>(region.y * zoom);
    
    if (destY < 0) {
      region.y = static_cast<int32_t>(-destY / zoom);
      destY = 0;
    }
    else {
      region.y = 0;
    }
    glPixelZoom(zoom, zoom);
  }
  else {    
    destY = canvas->height - destY - 1;
    glPixelZoom(zoom, -zoom);
  }
  TIMER_PRINT("before draw"); 
  /*fprintf(stderr,"Region %d %d %d %d : LodScale %d : Zoom %f\n",region.x,region.y,region.width,region.height,lodScale,zoom);*/
  
  //glRasterPos2i(destX, destY); 
  // use glBitMap to set raster position
  glBitmap(0, 0, 0, 0, destX, destY, NULL);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, image->width);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, saveSkip);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, region.x);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 
                canvas->requiredImageFormat.scanlineByteMultiple);

  DEBUGMSG("Buffer for frame %d is %dw x %dh, region is %dw x %dh, destX = %d, destY = %d\n", frameNumber, image->width, image->height, region.width, region.height, destX, destY); 

  if (region.width > image->width || region.height > image->height ||
      region.width < 0 || region.height < 0 ||
      region.width*region.height > image->width*image->height) {
    DEBUGMSG("Abort before glDrawPixels due to programming error.  Sanity check failed.\n"); 
    abort(); 
  } else {    
    glDrawPixels(region.width, region.height,
                 GL_RGB, GL_UNSIGNED_BYTE,
                 image->imageData);
    DEBUGMSG("Done with glDrawPixels\n"); 
  }

  // move the raster position back to 0,0
  glBitmap(0, 0, 0, 0, -destX, -destY, NULL);
 //  glRasterPos2i(0,0); 
  
  /* This is bad, we are managing the cache in the render thread.  Sigh.  Anyhow, have to release the image, or the cache will fill up */  
  //canvas->imageCache->ReleaseImage(image);
  TIMER_PRINT("glRenderer::Render end"); 
}




void glStereoRenderer::Render(int frameNumber,
                              const Rectangle *imageRegion,
                              int destX, int destY, float zoom, int lod)
{
  int lodScale;
  int localFrameNumber;
  Rectangle region = *imageRegion;
  Image *image;
  int saveSkip;
  int saveDestX;
  int saveDestY;
  
  Canvas *canvas = NULL;  //POISON!  WILL SEGFAULT -- REMOVE 
  DEBUGMSG("glStereoRenderer::Render(frame %d) region @ (%d, %d) size %d x %d render at %d, %d, with zoom=%f  lod=%d, stereo = %d", frameNumber, 
           imageRegion->x, imageRegion->y,
           imageRegion->width, imageRegion->height,
           destX, destY, zoom, lod, 
           (int)(canvas->frameList->stereo));
  
  /*
   * Compute possibly reduced-resolution image region to display.
   */
 
  if (canvas->frameList->stereo) {
        localFrameNumber = frameNumber *2; 
        /* start with left frame*/
        glDrawBuffer(GL_BACK_LEFT);
  }
  else {
        localFrameNumber = frameNumber;
  }

  
  bb_assert(lod <= canvas->frameList->getFrame(localFrameNumber)->maxLOD);
  lodScale = 1 << lod;

  {
    int rr;

    rr = ROUND_TO_MULTIPLE(imageRegion->x,lodScale);
    if(rr > imageRegion->x) {
      region.x = rr - lodScale;
    }
    region.x /= lodScale;
   
    rr = ROUND_TO_MULTIPLE(imageRegion->y,lodScale);
    if(rr > imageRegion->y) {
      region.y = rr - lodScale;
    }
    region.y /= lodScale;

    rr = ROUND_TO_MULTIPLE(imageRegion->width,lodScale);
    if(rr > imageRegion->width) {
      region.width = rr - lodScale;
    }
    region.width /= lodScale;

    rr = ROUND_TO_MULTIPLE(imageRegion->height,lodScale);
    if(rr > imageRegion->height) {
      region.height = rr - lodScale;
    }
    region.height /= lodScale;
  }
  
  zoom *= (float) lodScale;

  /* Pull the image from our cache */
  image = canvas->imageCache->GetImage(localFrameNumber, &region, lod);

  if (image == NULL) {
    /* error has already been reported */
    return;
  }

  saveSkip =  image->height - (region.y + region.height);
  saveDestX = destX;
  saveDestY = destY;
 

  bb_assert(region.x >= 0);
  bb_assert(region.y >= 0);
  /*bb_assert(region.x + region.width <= image->width);*/
  /*bb_assert(region.y + region.height <= image->height);*/

  glViewport(0, 0, canvas->width, canvas->height);


  /* only clear the window if we have to */
  if (destX > 0 || destY > 0 ||
      region.width * zoom < canvas->width ||
      region.height * zoom < canvas->height) {
    glClearColor(0.0, 0.0, 0.0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
  }

  if (image->imageFormat.rowOrder == BOTTOM_TO_TOP) {
    /*
     * Do adjustments to flip Y axis.
     * Yes, this is tricky to understand.
     * And we're not going to help you. 
     */
    destY = canvas->height - static_cast<int32_t>((image->height * zoom + destY)) + static_cast<int32_t>(region.y * zoom);
    if (destY < 0) {
      region.y = static_cast<int32_t>(-destY / zoom);
      destY = 0;
    }
    else {
      region.y = 0;
    }
    /* RasterPos is (0,0).  Offset it by (destX, destY) */
    
    DEBUGMSG("BOTTOM_TO_TOP: glDrawPixels(%d, %d, GL_RGB, GL_UNSIGNED_BYTE, data)\n",  region.width, region.height); 
    glPixelZoom(zoom, zoom);
  }
  else {
    DEBUGMSG("TOP_TO_BOTTOM: glDrawPixels(%d, %d, GL_RGB, GL_UNSIGNED_BYTE, data)\n",   region.width, region.height); 
    destY = canvas->height - destY - 1;
    /* RasterPos is (0,0).  Offset it by (destX, destY) */
    glPixelZoom(zoom, -zoom);
  }
  glBitmap(0, 0, 0, 0, destX, destY, NULL);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, image->width);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, saveSkip);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, region.x);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 
                canvas->requiredImageFormat.scanlineByteMultiple);
  glDrawPixels(region.width, region.height,
               GL_RGB, GL_UNSIGNED_BYTE,
               image->imageData);
  
  /* Offset raster pos by (-destX, -destY) to put it back to (0,0) */
  glBitmap(0, 0, 0, 0, -destX, -destY, NULL);
  
  /* Have to release the image, or the cache will fill up */
  //canvas->imageCache->ReleaseImage(image);
  
  if(canvas->frameList->stereo) {
    glDrawBuffer(GL_BACK_RIGHT);
    localFrameNumber++;
    
    /* Pull the image from our cache */
    image = canvas->imageCache->GetImage(localFrameNumber, &region, lod);
    if (image == NULL) {
      /* error has already been reported */
      return;
    }
    
    glViewport(0, 0, canvas->width, canvas->height);
    
    /* only clear the window if we have to */
    if (saveDestX > 0 || saveDestY > 0 ||
        region.width * zoom < canvas->width ||
        region.height * zoom < canvas->height) {
      glClearColor(0.0, 0.0, 0.0, 0);
      glClear(GL_COLOR_BUFFER_BIT);
    }
    
    
    if (image->imageFormat.rowOrder == BOTTOM_TO_TOP) {
      
      glPixelZoom(zoom, zoom);
      
    }
    else {
      
      DEBUGMSG("Image order is %d\n", image->imageFormat.rowOrder); 
      /* RasterPos is (0,0).  Offset it by (destX, destY) */
      glPixelZoom(zoom, -zoom);
    }
    
    glBitmap(0, 0, 0, 0, destX, destY, NULL);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, image->width);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, saveSkip);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, region.x);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 
                  canvas->requiredImageFormat.scanlineByteMultiple);
    glDrawPixels(region.width, region.height,
                 GL_RGB, GL_UNSIGNED_BYTE,
                 image->imageData);
    
    /* Offset raster pos by (-destX, -destY) to put it back to (0,0) */
    glBitmap(0, 0, 0, 0, -destX, -destY, NULL);
    
    /* Have to release the image, or the cache will fill up */
    //canvas->imageCache->ReleaseImage(image);
    
  }
  
  
  return; 
  
}

//===========================================================
// glTextureRenderer
// ==========================================================

glTextureRenderer::glTextureRenderer(ProgramOptions *opt, Canvas *canvas):
  glRenderer(opt, canvas, "gltexture") {
  return; 
}

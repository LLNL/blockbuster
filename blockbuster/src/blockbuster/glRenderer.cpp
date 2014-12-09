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

#include "glRenderer.h"
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



void glRenderer::FinishRendererInit(void) {
  // from glFinishInitialization: 
  Bool rv;
  Font id = mFontInfo->fid;
  unsigned int first = mFontInfo->min_char_or_byte2;
  unsigned int last = mFontInfo->max_char_or_byte2;
   
  mContext = glXCreateContext(mDisplay, mVisualInfo,
                             NULL, GL_TRUE);
  if (!mContext) {
    ERROR("couldn't create GLX context");
    return ;
  }
  
  rv = glXMakeCurrent(mDisplay, mWindow, mContext);
  if (rv == False) {
    ERROR("couldn't make graphics context current");
    glXDestroyContext(mDisplay, mContext);
    return ;
  }
  
  dbprintf(2, "GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));
  
  /* OpenGL display list font bitmaps */
  fontBase = glGenLists((GLuint) last + 1);
  if (!fontBase) {
    ERROR("Unable to allocate display lists for fonts");
    glXDestroyContext(mDisplay, mContext);
    return ;
  }
  
  glXUseXFont(id, first, last - first + 1, fontBase + first);
  glListBase(fontBase);

  // Specify our required format. Assume we're getting 24-bit RGB pixels.  
  mRequiredImageFormat.scanlineByteMultiple = 1;
  mRequiredImageFormat.rowOrder = ROW_ORDER_DONT_CARE;
  mRequiredImageFormat.byteOrder = MSB_FIRST;
  mRequiredImageFormat.bytesPerPixel = 3;

  // eliminate features that potentially could slow us down 
  glDisable(GL_ALPHA_TEST);
  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_DITHER);
  glDisable(GL_FOG);
  glDisable(GL_LIGHTING);
  glDisable(GL_LOGIC_OP);
  glDisable(GL_STENCIL_TEST);
  glDisable(GL_TEXTURE_1D);
  glDisable(GL_TEXTURE_2D);
  glPixelTransferi(GL_MAP_COLOR, GL_FALSE);
  glPixelTransferi(GL_RED_SCALE, 1);
  glPixelTransferi(GL_RED_BIAS, 0);
  glPixelTransferi(GL_GREEN_SCALE, 1);
  glPixelTransferi(GL_GREEN_BIAS, 0);
  glPixelTransferi(GL_BLUE_SCALE, 1);
  glPixelTransferi(GL_BLUE_BIAS, 0);
  glPixelTransferi(GL_ALPHA_SCALE, 1);
  glPixelTransferi(GL_ALPHA_BIAS, 0);
  
  return; 
}

// ======================================================================
void glRenderer::DoStereo(bool doStereo) {  
  if (doStereo) {
    if (!mHaveStereo) {
      WARNING("Request to do stereo not supported by current graphics context."); 
      doStereo = false;   
    }
    if (mFrameList && !mFrameList->mStereo) {
      WARNING("Stereo cannot be enabled in a non-stereo movie"); 
      doStereo = false; 
    }
  }
  if (doStereo != mDoStereo) {
    if (doStereo) {
      dbprintf(1, "Enabling Stereo\n"); 
    } else {
      dbprintf(1, "Disabling Stereo\n"); 
    }
    mDoStereo = doStereo; 
  }
  return; 
}  


//=============================================================
void glRenderer::ChooseVisual(void)
{
  DEBUGMSG("glChooseVisual"); 
  static int attributes[] = {
    GLX_USE_GL, GLX_RGBA, GLX_DOUBLEBUFFER,
    GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1,
    None
  }, stereoAttributes[] = {
    GLX_USE_GL, GLX_RGBA, GLX_DOUBLEBUFFER, GLX_STEREO,
    GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1,
    None
  };
  // mVisualInfo = glXChooseVisual(mDisplay, mScreenNumber, attributes);
  mHaveStereo = true; 
  mVisualInfo = glXChooseVisual(mDisplay, mScreenNumber, stereoAttributes);
  if (mVisualInfo == NULL) {
    cerr << "Warning: could not create stereo visual on display" << endl; 
    mDoStereo = false; 
    mHaveStereo = false; 
    mVisualInfo = glXChooseVisual(mDisplay, mScreenNumber, attributes);
    if (!mVisualInfo) {
      ERROR("cannot find a GLX visual on %s to create new OpenGL window",
            DisplayString(mDisplay));
      exit(1); 
    }
  }
  return; 

}

//=============================================================
void glRenderer::DrawString(int row, int column, const char *str)
{
  const int x = (column + 1) * mFontHeight;
  const int y = (row + 1) * mFontHeight;
  glPushAttrib(GL_CURRENT_BIT);
  glBitmap(0, 0, 0, 0, x, mWindowHeight - y - 1, NULL);
  glCallLists(strlen(str), GL_UNSIGNED_BYTE, (GLubyte *) str);
  glPopAttrib();
  return; 
}

#define RENDERDEBUG DEBUGMSG
//=============================================================

void glRenderer::RenderActual(Rectangle region){
  int lodScale;
  int localFrameNumber;
  int saveSkip;
  /* int frameNumber, 
                              RectanglePtr imageRegion,
                              int destX, int destY, float zoom, int lod
  */

  RENDERDEBUG("glRenderer::RenderActual begin, frame %d, %d x %d  at %d, %d  zoom=%f  lod=%d", 
           mCurrentFrame,
           region.width, region.height,
           region.x, region.y, mZoom, mLOD);
#ifdef RENDER_TIMING
  float t1 = GetExactSecondsDouble(), t2=0; 
#endif
  /*
   * Compute possibly reduced-resolution image region to display.
   */
  if (mDoStereo) {
    glDrawBuffer(GL_BACK_LEFT);
  } else {
    // Draw to both left & right back buffers at same time
    glDrawBuffer(GL_BACK); 
  }
    
  if (mFrameList->mStereo) {
    localFrameNumber = mCurrentFrame *2; 
  }
  else {
    localFrameNumber = mCurrentFrame;
  }

 
  lodScale = 1 << mLOD;

 
  int rr = ROUND_UP_TO_MULTIPLE(region.x,lodScale);
  if(rr > region.x) {
    region.x = rr - lodScale;
  }
  region.x /= lodScale;
  
  rr = ROUND_UP_TO_MULTIPLE(region.y,lodScale);
  if(rr > region.y) {
    region.y = rr - lodScale;
  }
  region.y /= lodScale;
  
  rr = ROUND_UP_TO_MULTIPLE(region.width,lodScale);
  if(rr > region.width) {
    region.width = rr - lodScale;
  }
  region.width /= lodScale;
  
  rr = ROUND_UP_TO_MULTIPLE(region.height,lodScale);
  if(rr > region.height) {
    region.height = rr - lodScale;
  }
  
  region.height /= lodScale;

  float scaledZoom = mZoom * (float) lodScale;


  RENDERDEBUG("glRenderer::RenderActual Pull the image from our cache "); 
  ImagePtr image =  mCache->GetImage(localFrameNumber, &region, mLOD, true), 
    rightimage;
  if (!image) {
    /* error has already been reported */
    return;
  }
  RENDERDEBUG("Got image"); 

  if(mDoStereo) {
    /* Pull the right hand image from our cache */
    localFrameNumber++;
    RENDERDEBUG("glStereoRenderer::RenderActual Pull the right buffer image from our cache "); 
    rightimage = mCache->GetImage(localFrameNumber, &region, mLOD, false);
    RENDERDEBUG("Got right buffer image"); 
    if (rightimage == NULL) {
      /* error has already been reported */
      return;
    }
  } else {
    rightimage = image; 
  }

  saveSkip =  image->height - (region.y + region.height);
 
  bb_assert(region.x >= 0);
  bb_assert(region.y >= 0);

  glViewport(0, 0, mWindowWidth, mWindowHeight);
  RENDERDEBUG("done with glViewport"); 


  /* only clear the window if we have to */
  bool needClear = false; 
  if (mImageDrawX > 0 || mImageDrawY > 0 ||
      region.width * scaledZoom < mWindowWidth ||
      region.height * scaledZoom < mWindowHeight) {
    needClear = true; 
    glClearColor(mBackgroundColor[0], mBackgroundColor[1], mBackgroundColor[2], 0);
    glClear(GL_COLOR_BUFFER_BIT);
  }

  RENDERDEBUG(QString("Done with glClearColor and glClear.  Frame %1 row order is %2").arg(mCurrentFrame).arg(image->imageFormat.rowOrder)); 
  
  if (image->imageFormat.rowOrder == BOTTOM_TO_TOP) {
    /*
     * Do adjustments to flip Y axis.
     * Yes, this is tricky to understand.  
     * The authors did not bother to explain it, either.  LOL
     */
    mImageDrawY = mWindowHeight - static_cast<int32_t>((image->height * scaledZoom) + mImageDrawY) + static_cast<int32_t>(region.y * scaledZoom);
    
    if (mImageDrawY < 0) {
      region.y = static_cast<int32_t>(-mImageDrawY / scaledZoom);
      mImageDrawY = 0;
    }
    else {
      region.y = 0;
    }
    glPixelZoom(scaledZoom, scaledZoom);
  }
  else {    
    mImageDrawY = mWindowHeight - mImageDrawY - 1;
    glPixelZoom(scaledZoom, -scaledZoom);
  }
  RENDERDEBUG("Done with glPixelZoom. Region %d %d %d %d : LodScale %d : Zoom %f",region.x,region.y,region.width,region.height,lodScale,scaledZoom);
  
  //glRasterPos2i(mImageDrawX, mImageDrawY); 
  // use glBitMap to set raster position
  glBitmap(0, 0, 0, 0, mImageDrawX, mImageDrawY, NULL);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, image->width);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, saveSkip);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, region.x);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 
                mRequiredImageFormat.scanlineByteMultiple);
  RENDERDEBUG("glPixelStorei(GL_UNPACK_ROW_LENGTH, %d)", image->width);
  RENDERDEBUG("glPixelStorei(GL_UNPACK_SKIP_ROWS,  %d)", saveSkip);
  RENDERDEBUG("glPixelStorei(GL_UNPACK_SKIP_PIXELS,  %d)",  region.x);
  RENDERDEBUG("glPixelStorei(GL_UNPACK_ALIGNMENT,  %d)", mRequiredImageFormat.scanlineByteMultiple);

  RENDERDEBUG("Buffer for frame %d is %dw x %dh, region is %dw x %dh, mImageDrawX = %d, mImageDrawY = %d", mCurrentFrame, image->width, image->height, region.width, region.height, mImageDrawX, mImageDrawY); 

  if (region.width > (int32_t)image->width || region.height > (int32_t)image->height ||
      region.width < 0 || region.height < 0 ||
      region.width*region.height > (int32_t)(image->width*image->height)) {
    RENDERDEBUG("Abort before glDrawPixels due to programming error.  Sanity check failed."); 
    abort(); 
  } 

  glDrawPixels(region.width, region.height,
               GL_RGB, GL_UNSIGNED_BYTE,
               image->Data());
  RENDERDEBUG("Done with glDrawPixels"); 
  

  // move the raster position back to 0,0
  glBitmap(0, 0, 0, 0, -mImageDrawX, -mImageDrawY, NULL);
  RENDERDEBUG("Done with glBitmap"); 

  if (mDoStereo) {
      glDrawBuffer(GL_BACK_RIGHT);
            
      glViewport(0, 0, mWindowWidth, mWindowHeight);
      
      if (mFrameList->mStereo && needClear) {
        glClearColor(mBackgroundColor[0], mBackgroundColor[1], mBackgroundColor[2], 0);
        glClear(GL_COLOR_BUFFER_BIT);
      }
      
      
      if (rightimage->imageFormat.rowOrder == BOTTOM_TO_TOP) {
        
        glPixelZoom(scaledZoom, scaledZoom);
        
      }
      else {
        
        DEBUGMSG("Image order is %d\n", rightimage->imageFormat.rowOrder); 
        /* RasterPos is (0,0).  Offset it by (mImageDrawX, mImageDrawY) */
        glPixelZoom(scaledZoom, -scaledZoom);
      }
      
      glBitmap(0, 0, 0, 0, mImageDrawX, mImageDrawY, NULL);
      glPixelStorei(GL_UNPACK_ROW_LENGTH, rightimage->width);
      glPixelStorei(GL_UNPACK_SKIP_ROWS, saveSkip);
      glPixelStorei(GL_UNPACK_SKIP_PIXELS, region.x);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 
                    mRequiredImageFormat.scanlineByteMultiple);
      RENDERDEBUG("Rendering right image to right buffer "); 
      glDrawPixels(region.width, region.height,
                   GL_RGB, GL_UNSIGNED_BYTE,
                   rightimage->Data());
      RENDERDEBUG("Done with glDrawPixels for right buffer "); 
      
      /* Offset raster pos by (-mImageDrawX, -mImageDrawY) to put it back to (0,0) */
      glBitmap(0, 0, 0, 0, -mImageDrawX, -mImageDrawY, NULL);
      RENDERDEBUG("Done with glBitmap for right buffer"); 
      
    }
    else {
      RENDERDEBUG("Not rendering to right buffer"); 
    }
#ifdef RENDER_TIMING
  t2 = GetExactSecondsDouble(); 
  timeSamples.push_back(t2-t1); 
  RENDERDEBUG("glRenderer::Render end (took %f secs (%f-%f))", t2-t1, t2, t1); 
#endif
  RENDERDEBUG("glRenderer::RenderActual end"); 
}

//***********************************************************************
void glRenderer::SwapBuffers(void) {
  glXSwapBuffers(mDisplay, mWindow);
  return; 
}


//===========================================================
// glTextureRenderer
// ==========================================================

void glTextureRenderer::BeginRendererInit(void){

  printf("using texture rendering\n"); 
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureWidth);
  maxTextureHeight = maxTextureWidth;
  
  if (maxTextureWidth >= 4096 ||
      maxTextureHeight >= 4096) {
	/* XXX NVIDIA's GeForce reports 4Kx4K but that size doesn't
	 * actually work!
	 * Furthermore, smaller textures seem to be faster when tiling.
	 */
	maxTextureWidth = 2048;
	maxTextureHeight = 2048;
  }
  INFO("Max Texture Size: %d x %d",
       maxTextureWidth,
       maxTextureHeight);
  for (uint32_t i = 0; i< 100; i++) {
    TextureObjectPtr p(new TextureObject); 
    glGenTextures( 1, &p->texture );
    p->anyLoaded = false;
    mTextures.push_back(p); 
  }
  texIntFormat = GL_RGB;
  texFormat = GL_RGB;
    
  return; 
}

//====================================================================
/* This is used to upscale texture sizes to the nearest power of 2, 
 * which is necessary in OpenGL.
 */
int32_t glTextureRenderer::MinPowerOf2(int x)
{
  int32_t rv = 1;
  while (rv > 0 && rv < x) {
	rv <<= 1;
  }
  return rv;
}



//====================================================================
/*
 * Set new projecton matrix and viewport parameters for the given
 * window size.
 */
void glTextureRenderer::UpdateProjectionAndViewport(int newWidth, int newHeight)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  /* Note: we're flipping the Y axis here */
  glOrtho(0.0, (GLdouble) newWidth,
          (GLdouble) newHeight, 0.0,
          -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glViewport(0, 0, newWidth, newHeight);
}


TextureObjectPtr glTextureRenderer::GetTextureObject(int frameNumber)
{
  // cerr << "Warning:  DEAD CODE EXECUTING? " << endl; 
  //static GLuint clock = 1;
  TextureObjectPtr texObj = 
    mFrameList->getFrame(frameNumber)->mTextureObject;
  
  if (!texObj) {
    
    /* 	//GLuint oldestAge = ~0;
	// int oldestPos = -1;
	find LRU texture object 
	for (uint32_t i = 0; i < mTextures.size(); i++) {
    if (mTextures[i]->age < oldestAge) {
    oldestAge = mTextures[i]->age;
    oldestPos = i;
    }
	}
    
	bb_assert(oldestPos >= 0);
	bb_assert(oldestAge != ~(uint32_t)0);
	texObj = mTextures[oldestPos];
    */     
    
    texObj = mTextures[frameNumber % mTextures.size()];
	/* unlink FrameInfo pointer */
    if (texObj->frameInfo && texObj->frameInfo->mTextureObject)
      texObj->frameInfo->mTextureObject.reset();
    
	/* update/init texObj fields */
    //	texObj->age = clock++;	/* XXX handle clock wrap-around! */
	texObj->frameInfo = mFrameList->getFrame(frameNumber);
    for (uint32_t i = 0; i < MAX_IMAGE_LEVELS; i++)
      texObj->valid[i].width = texObj->valid[i].height = -1;
	texObj->anyLoaded = GL_FALSE;
	mFrameList->getFrame(frameNumber)->mTextureObject = texObj;
  }
  
  return texObj;
}


void glTextureRenderer::RenderActual(Rectangle region) {
  GLfloat s0, t0, s1, t1;
  GLfloat x0, y0, x1, y1;
  int32_t lodScale;
  int localFrameNumber;

  RENDERDEBUG("glTextureRenderer::RenderActual begin, frame %d: %d, %d  %d x %d  at %d, %d  zoom=%f  lod=%d",
              mCurrentFrame, region.x, region.y,
              region.width, region.height,
              mImageDrawX, mImageDrawY, mZoom, mLOD);
  
  /*   if (glXMakeCurrent(mDisplay, mWindow, context) == False) {
    WARNING("couldn't make graphics context current before rendering");
    }*/ 
  
  UpdateProjectionAndViewport(mWindowWidth, mWindowHeight);
  glEnable(GL_TEXTURE_2D);
  
  if (mFrameList->mStereo) {
    localFrameNumber = mCurrentFrame *2; /* we'll display left frame only */
  }
  else {
    localFrameNumber = mCurrentFrame;
  }
  
  /*
   * Compute possibly reduced-resolution image region to display.
   */
  lodScale = 1 << mLOD;
  region.x = region.x / lodScale;
  region.y = region.y / lodScale;
  region.width = region.width / lodScale;
  region.height = region.height / lodScale;
  float scaledZoom = mZoom * (float) lodScale;
  
  /* Pull the image from our cache */
  ImagePtr image = mCache->GetImage( localFrameNumber, &region, mLOD, true);
  if (!image) {
    /* error has already been reported */
    return;
  }
  
  /* get texture object */
  TextureObjectPtr texObj = GetTextureObject(localFrameNumber);
  bb_assert(texObj);
  bb_assert(texObj->frameInfo == mFrameList->getFrame(localFrameNumber));
  
  /* Setup/bind the texture object */
  if (texObj->texture) {
    glBindTexture(GL_TEXTURE_2D, texObj->texture);
  }
  else {
    /* create initial texture image */
    glGenTextures(1, &texObj->texture);
    glBindTexture(GL_TEXTURE_2D, texObj->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }
  
  /* Initial texture size & contents */
  if (!texObj->width) {
    int w, h, level;
    /* compute best texture size */
    texObj->width = MIN2(MinPowerOf2(image->width), maxTextureWidth);
    texObj->height = MIN2(MinPowerOf2(image->height), maxTextureHeight);
    /* make initial image (undefined contents) */
    w = texObj->width;
    h = texObj->height;
    level = 0;
    while (w > 1 || h > 1) {
      glTexImage2D(GL_TEXTURE_2D, level, texIntFormat, 
                   w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
      if (w > 1)
        w /= 2;
      if (h > 1)
        h /= 2;
      level++;
    }
  }
  
  glClearColor(0.25, 0.25, 0.25, 0);  /* XXX temporary */
  
  /*
   * Three possible rendering paths...
   */
  if (image->width <=texObj->width / lodScale
      && image->height <= texObj->height / lodScale) {
    /* The movie image can completely fit into a single texture.
     * We'll load subtextures which exactly match the image region
     * that we need to draw.
     */
    const int origRegY = region.y; /* save this */
    
    /* make sure the sub image region is valid */
    if (!texObj->anyLoaded ||
        !RectContainsRect(&texObj->valid[mLOD], &region)) {
      /* load the texture data now */
      
      glPixelStorei(GL_UNPACK_ROW_LENGTH, image->width);
      glPixelStorei(GL_UNPACK_SKIP_PIXELS, region.x);
      glPixelStorei(GL_UNPACK_ALIGNMENT,
                    image->imageFormat.scanlineByteMultiple);
      
      if (image->imageFormat.rowOrder == TOP_TO_BOTTOM) {
        glPixelStorei(GL_UNPACK_SKIP_ROWS, region.y);
      }
      else {
        /* invert Y coord of region to load */
        int skipRows = 0;
        bb_assert(image->imageFormat.rowOrder == BOTTOM_TO_TOP);
        /* this is a bit tricky */
        if (region.y + static_cast<int32_t>(region.height) < static_cast<int32_t>(image->height)) {
          skipRows = image->height - (region.y + region.height);
        }
        glPixelStorei(GL_UNPACK_SKIP_ROWS, skipRows);
        region.y = image->height - region.y - region.height;
      }
      
      glTexSubImage2D(GL_TEXTURE_2D, mLOD,
                      region.x, region.y,
                      region.width, region.height,
                      texFormat, GL_UNSIGNED_BYTE,
                      image->Data());
      
      if (texObj->anyLoaded)
        texObj->valid[mLOD] = RectUnionRect(&texObj->valid[mLOD], &region);
      else
        texObj->valid[mLOD] = region;
      texObj->anyLoaded = GL_TRUE;
    }
    
	/* Choose active mipmap level */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, mLOD);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mLOD);
    
    /* compute texcoords and vertex coords */
    s0 = (float) region.x / (texObj->width / lodScale);
    s1 = (float) (region.x + region.width) / (texObj->width / lodScale);
    if (image->imageFormat.rowOrder == TOP_TO_BOTTOM) {
      t0 = (float) region.y / (texObj->height / lodScale);
      t1 = (float) (region.y + region.height) / (texObj->height / lodScale);
    }
    else {
      int top = image->height - origRegY;
      int bot = top - region.height;
      t0 = (float) top / (texObj->height / lodScale);
      t1 = (float) bot / (texObj->height / lodScale);
    }
    x0 = mImageDrawX;
    y0 = mImageDrawY;
    x1 = mImageDrawX + region.width * scaledZoom;
    y1 = mImageDrawY + region.height * scaledZoom;
    
    /* XXX don't clear if the polygon fills the window */
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_QUADS);
    glTexCoord2f(s0, t0);
    glVertex2f(x0, y0);
    glTexCoord2f(s0, t1);
    glVertex2f(x0, y1);
    glTexCoord2f(s1, t1);
    glVertex2f(x1, y1);
    glTexCoord2f(s1, t0);
    glVertex2f(x1, y0);
    glEnd();
  }
  else if (region.width <= static_cast<int32_t>(texObj->width) &&
           region.height <= static_cast<int32_t>(texObj->height)) {
    /* The region of interest to draw fits entirely into one texture,
     * but the full movie frame is too large to fit into one texture.
     * Load a subtexture at (0,0) for the region of interest.
     */
    glPixelStorei(GL_UNPACK_ROW_LENGTH, image->width);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, region.x);
    if (image->imageFormat.rowOrder == BOTTOM_TO_TOP) {
      int skip;
      skip = image->height - (region.y + region.height);
      glPixelStorei(GL_UNPACK_SKIP_ROWS, skip);
    }
    else {
      glPixelStorei(GL_UNPACK_SKIP_ROWS, region.y);
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT,
                  image->imageFormat.scanlineByteMultiple);
    
    glTexSubImage2D(GL_TEXTURE_2D, 0,
                    0, 0, /* pos */
                    region.width, region.height,
                    texFormat, GL_UNSIGNED_BYTE,
                    image->Data());
    /* invalidate valid region, for sake of first path, above */
    texObj->valid[mLOD].x = 0;
    texObj->valid[mLOD].y = 0;
    texObj->valid[mLOD].width = 0;
    texObj->valid[mLOD].height = 0;
    
    s0 = 0.0;
    t0 = 0.0;
    s1 = (float) region.width / texObj->width;
    t1 = (float) region.height / texObj->height;
    if (image->imageFormat.rowOrder == BOTTOM_TO_TOP) {
      /* invert texcoords */
      GLfloat temp = t0;
      t0 = t1;
      t1 = temp;
    }
    
    x0 = mImageDrawX;
    y0 = mImageDrawY;
    x1 = mImageDrawX + region.width * scaledZoom;
    y1 = mImageDrawY + region.height * scaledZoom;
    
    /* XXX don't clear if the polygon fills the window */
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_QUADS);
    glTexCoord2f(s0, t0);
    glVertex2f(x0, y0);
    glTexCoord2f(s0, t1);
    glVertex2f(x0, y1);
    glTexCoord2f(s1, t1);
    glVertex2f(x1, y1);
    glTexCoord2f(s1, t0);
    glVertex2f(x1, y0);
    glEnd();
  }
  else {
    /* We're drawing an image that's larger than the max texture size.
     * I.e. a _really_ big movie image.
     * Draw it in pieces, as a tiling of quadrilaterals.
     */
    const int tileWidth = maxTextureWidth;
    const int tileHeight = maxTextureHeight;
    int row, col, width, height;
    
    /* invalidate valid region, for sake of first path, above */
    texObj->valid[mLOD].x = 0;
    texObj->valid[mLOD].y = 0;
    texObj->valid[mLOD].width = 0;
    texObj->valid[mLOD].height = 0;
    
    glPixelStorei(GL_UNPACK_ROW_LENGTH, image->width);
    glPixelStorei(GL_UNPACK_ALIGNMENT,
                  image->imageFormat.scanlineByteMultiple);
    
    glClear(GL_COLOR_BUFFER_BIT);
    
    /* march over the sub image region, drawing a tile at a time */
    for (row = 0; row < region.height; row += tileHeight) {
      for (col = 0; col < region.width; col += tileWidth) {
        
        /* compute tile / texture size */
        if (col + tileWidth > region.width) {
          width = region.width - col;
        }
        else {
          width = tileWidth;
        }
        if (row + tileHeight > region.height) {
          height = region.height - row;
          bb_assert(height > 0);
        }
        else {
          height = tileHeight;
        }
        
        /* compute SKIP_PIXELS and load texture data */
        if (image->imageFormat.rowOrder == BOTTOM_TO_TOP) {
          int bottom = region.height - row - height;
          if (region.y + region.height 
              < static_cast<int32_t>(image->height)) {
            int d = image->height
              - (region.y + region.height);
            bottom += d;
          }
          glPixelStorei(GL_UNPACK_SKIP_ROWS, bottom);
        }
        else {
          glPixelStorei(GL_UNPACK_SKIP_ROWS, region.y + row);
        }
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, region.x + col);
        glTexSubImage2D(GL_TEXTURE_2D, 0,
                        0, 0, width, height,
                        texFormat, GL_UNSIGNED_BYTE,
                        image->Data());
        
        /* tex coords */
        s0 = 0;
        t0 = 0;
        s1 = (float) width / maxTextureWidth;
        t1 = (float) height / maxTextureHeight;
        if (image->imageFormat.rowOrder == BOTTOM_TO_TOP) {
          /* invert texcoords */
          GLfloat temp = t0;
          t0 = t1;
          t1 = temp;
        }
        
        /* vertex coords */
        x0 = mImageDrawX + col * scaledZoom;
        y0 = mImageDrawY + row * scaledZoom;
        x1 = x0 + width * scaledZoom;
        y1 = y0 + height * scaledZoom;
        
        /* draw quad */
        glBegin(GL_QUADS);
        glTexCoord2f(s0, t0);
        glVertex2f(x0, y0);
        glTexCoord2f(s0, t1);
        glVertex2f(x0, y1);
        glTexCoord2f(s1, t1);
        glVertex2f(x1, y1);
        glTexCoord2f(s1, t0);
        glVertex2f(x1, y0);
        glEnd();
        
      } /* for col */
    } /* for row */
    
    texObj->valid[mLOD].x = 0;
    texObj->valid[mLOD].y = 0;
    texObj->valid[mLOD].width = 0;
    texObj->valid[mLOD].height = 0;
  }
  
  /* debug */
  {
    int err = glGetError();
    if (err) {
      ERROR("OpenGL Error 0x%x\n", err);
    }
  }
  
  
  glDisable(GL_TEXTURE_2D);
  RENDERDEBUG("glTextureRenderer::RenderActual end"); 
 
}


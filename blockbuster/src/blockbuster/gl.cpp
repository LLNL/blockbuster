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




static int globalSync = 1;

void gl_HandleOptions(int &argc, char *argv[]) {
  
  while (argc > 1) {
    if (!strcmp(argv[1], "-h")) {
      fprintf(stderr, "Renderer: %s\n", GL_NAME);
      fprintf(stderr, "%s\n", GL_DESCRIPTION);
      fprintf(stderr, "Options: ");
      fprintf(stderr, "-h gives help\n");
      fprintf(stderr, "-s toggles XSynchronize [%s]\n",
              globalSync?"on":"off");
      exit(MOVIE_HELP);
    } else if (!strcmp(argv[1], "-s")) {
      ConsumeArg(argc, argv, 1); 
      globalSync = !globalSync;
    }
    else { 
      return; 
    }
  }
  // never reach here...
  return ; 
  
}


static void gl_Render(Canvas *canvas, int frameNumber,
                   const Rectangle *imageRegion,
                   int destX, int destY, float zoom, int lod)
{
  int lodScale;
  int localFrameNumber;
  Rectangle region = *imageRegion;
  Image *image;
  int saveSkip;
  TIMER_PRINT("gl_Render begin"); 
  
#if 0
  DEBUGMSG("gl::Render %d, %d  %d x %d  at %d, %d  zoom=%f  lod=%d",
        imageRegion->x, imageRegion->y,
        imageRegion->width, imageRegion->height,
        destX, destY, zoom, lod);
#endif

  /*
   * Compute possibly reduced-resolution image region to display.
   */
  if (canvas->frameList->stereo) {
        localFrameNumber = frameNumber *2; /* we'll display left frame only */
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


  TIMER_PRINT("Pull the image from our cache "); 
  image = GetImageFromCache(canvas->imageCache, localFrameNumber, &region, lod);
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
    TIMER_PRINT("before draw"); 
    /*fprintf(stderr,"Region %d %d %d %d : LodScale %d : Zoom %f\n",region.x,region.y,region.width,region.height,lodScale,zoom);*/

    /* RasterPos is (0,0).  Offset it by (destX, destY) */
    glBitmap(0, 0, 0, 0, destX, destY, NULL);
    glPixelZoom(zoom, zoom);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, image->width);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, saveSkip);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, region.x);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 
                  canvas->requiredImageFormat.scanlineByteMultiple);
    glDrawPixels(region.width, region.height,
                 GL_RGB, GL_UNSIGNED_BYTE,
                 image->imageData);
  }
  else {
    bb_assert(image->imageFormat.rowOrder == TOP_TO_BOTTOM);

    destY = canvas->height - destY - 1;
    /* RasterPos is (0,0).  Offset it by (destX, destY) */
    glBitmap(0, 0, 0, 0, destX, destY, NULL);
    glPixelZoom(zoom, -zoom);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, image->width);
    /*glPixelStorei(GL_UNPACK_SKIP_ROWS, region.y);*/
    glPixelStorei(GL_UNPACK_SKIP_ROWS, saveSkip);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, region.x);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 
                  canvas->requiredImageFormat.scanlineByteMultiple);
    glDrawPixels(region.width, region.height,
                 GL_RGB, GL_UNSIGNED_BYTE,
                 image->imageData);
  }

  /* Offset raster pos by (-destX, -destY) to put it back to (0,0) */
  glBitmap(0, 0, 0, 0, -destX, -destY, NULL);

  /* Have to release the image, or the cache will fill up */
  ReleaseImageFromCache(canvas->imageCache, image);
  TIMER_PRINT("gl_Render end"); 
}




static void gl_RenderStereo(Canvas *canvas, int frameNumber,
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
  if (frameNumber == 0) {
    saveDestX=-42; // just a breakpoint for debugging, you can delete this. 
  }
 

#if 0
  DEBUGMSG("gl::Render %d, %d  %d x %d  at %d, %d  zoom=%f  lod=%d",
           imageRegion->x, imageRegion->y,
           imageRegion->width, imageRegion->height,
           destX, destY, zoom, lod);
#endif

 

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
  image = GetImageFromCache(canvas->imageCache, localFrameNumber, &region, lod);

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
   
    glBitmap(0, 0, 0, 0, destX, destY, NULL);
    glPixelZoom(zoom, zoom);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, image->width);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, saveSkip);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, region.x);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 
                                  canvas->requiredImageFormat.scanlineByteMultiple);
    glDrawPixels(region.width, region.height,
                                 GL_RGB, GL_UNSIGNED_BYTE,
                                 image->imageData);
  }
  else {
    bb_assert(image->imageFormat.rowOrder == TOP_TO_BOTTOM);

    destY = canvas->height - destY - 1;
    /* RasterPos is (0,0).  Offset it by (destX, destY) */
    glBitmap(0, 0, 0, 0, destX, destY, NULL);
    glPixelZoom(zoom, -zoom);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, image->width);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, saveSkip);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, region.x);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 
                                  canvas->requiredImageFormat.scanlineByteMultiple);
    glDrawPixels(region.width, region.height,
                                 GL_RGB, GL_UNSIGNED_BYTE,
                                 image->imageData);
  }

  /* Offset raster pos by (-destX, -destY) to put it back to (0,0) */
  glBitmap(0, 0, 0, 0, -destX, -destY, NULL);

  /* Have to release the image, or the cache will fill up */
  ReleaseImageFromCache(canvas->imageCache, image);

  if(canvas->frameList->stereo) {

        glDrawBuffer(GL_BACK_RIGHT);
        localFrameNumber++;

        /* Pull the image from our cache */
        image = GetImageFromCache(canvas->imageCache, localFrameNumber, &region, lod);
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
          
          glBitmap(0, 0, 0, 0, destX, destY, NULL);
          glPixelZoom(zoom, zoom);
          
          glPixelStorei(GL_UNPACK_ROW_LENGTH, image->width);
          glPixelStorei(GL_UNPACK_SKIP_ROWS, saveSkip);
          glPixelStorei(GL_UNPACK_SKIP_PIXELS, region.x);
          glPixelStorei(GL_UNPACK_ALIGNMENT, 
                                        canvas->requiredImageFormat.scanlineByteMultiple);
          glDrawPixels(region.width, region.height,
                                   GL_RGB, GL_UNSIGNED_BYTE,
                                   image->imageData);
        }
        else {
          bb_assert(image->imageFormat.rowOrder == TOP_TO_BOTTOM);

          /*destY = canvas->height - destY - 1;*/
          /* RasterPos is (0,0).  Offset it by (destX, destY) */
          glBitmap(0, 0, 0, 0, destX, destY, NULL);
          glPixelZoom(zoom, -zoom);
         
          glPixelStorei(GL_UNPACK_ROW_LENGTH, image->width);
          glPixelStorei(GL_UNPACK_SKIP_ROWS, saveSkip);
          glPixelStorei(GL_UNPACK_SKIP_PIXELS, region.x);
          glPixelStorei(GL_UNPACK_ALIGNMENT, 
                                        canvas->requiredImageFormat.scanlineByteMultiple);
          glDrawPixels(region.width, region.height,
                                   GL_RGB, GL_UNSIGNED_BYTE,
                                   image->imageData);
        }


         /* Offset raster pos by (-destX, -destY) to put it back to (0,0) */
        glBitmap(0, 0, 0, 0, -destX, -destY, NULL);

        /* Have to release the image, or the cache will fill up */
        ReleaseImageFromCache(canvas->imageCache, image);
        
  }

 
 
    
}




/*
 * After the canvas has been created, as well as the corresponding X window,
 * this function is called to do any renderer-specific setup.
 */
 MovieStatus
gl_Initialize(Canvas *canvas, const ProgramOptions *)
{
  /* Trivial for gl renderer: */
  /* Plug in our functions into the canvas */
  canvas->Render = gl_Render;
  canvas->ImageDataAllocator = DefaultImageDataAllocator;
  canvas->ImageDataDeallocator = DefaultImageDataDeallocator;
  canvas->DestroyRenderer = NULL;

  return MovieSuccess;
}

 MovieStatus
gl_InitializeStereo(Canvas *canvas, const ProgramOptions *)
{
  /*canvas->BeforeRender = BeforeRenderStereo;*/
  canvas->rendererPrivateData = NULL; 
  canvas->BeforeRender = NULL;
  canvas->Render = gl_RenderStereo;
  canvas->ImageDataAllocator = DefaultImageDataAllocator;
  canvas->ImageDataDeallocator = DefaultImageDataDeallocator;
  canvas->DestroyRenderer = NULL;

  return MovieSuccess;
}


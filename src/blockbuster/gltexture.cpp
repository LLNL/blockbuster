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

#include "errmsg.h"
#include "gltexture.h"
#include "util.h"
#include "cache.h"
#include "errmsg.h"
#include "frames.h"
#include "errmsg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <GL/gl.h>
#include <GL/glx.h>





/* How many texure objects we'll allocate */
#define MAX_TEXTURES 20


/*
 * We keep a (small?) number of texture objects that are reused for
 * rendering images.
 */
typedef struct {
    GLuint texture;
    GLuint width, height; /* level zero */
    Rectangle valid[MAX_IMAGE_LEVELS];
    GLboolean anyLoaded;   /* is any part of this texture valid/loaded? */
    GLuint age;
    FrameInfo *frameInfo;  /* back pointer */
} TextureObject;


typedef struct {
    int maxTextureWidth, maxTextureHeight;
    GLenum texFormat, texIntFormat;
    TextureObject textures[MAX_TEXTURES];

} RenderInfo;




void gltexture_HandleOptions(int &argc, char *argv[]) {
  while (argc > 1){
    if (!strcmp(argv[1], "-h")) {
      ConsumeArg(argc, argv, 1); 
      fprintf(stderr, "Renderer: %s\n", GLTEXTURE_NAME);
      fprintf(stderr, "%s\n", GLTEXTURE_DESCRIPTION);
      fprintf(stderr, "Options: there are no options for this renderer.\n"  );
      fprintf(stderr, "-h gives help\n");
     
      exit(MOVIE_HELP);
	}
    else { 
      return ; 
    }
  }
  return; 
}


/* This is used to upscale texture sizes to the nearest power of 2, 
 * which is necessary in OpenGL.
 */
static int32_t MinPowerOf2(int x)
{
    int32_t rv = 1;
    while (rv > 0 && rv < x) {
	rv <<= 1;
    }
    return rv;
}



/*
 * Set new projecton matrix and viewport parameters for the given
 * window size.
 */
static void UpdateProjectionAndViewport(int newWidth, int newHeight)
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


static TextureObject *
GetTextureObject(RenderInfo *renderInfo, Canvas *canvas, int frameNumber)
{
    static GLuint clock = 1;
    TextureObject *texObj = (TextureObject *) canvas->frameList->getFrame(frameNumber)->canvasPrivate;

    if (!texObj) {
	/* find a free texture object */
	GLuint oldestAge = ~0;
	int oldestPos = -1;
	int i;
	/* find LRU texture object */
	for (i = 0; i < MAX_TEXTURES; i++) {
	    if (renderInfo->textures[i].age < oldestAge) {
		oldestAge = renderInfo->textures[i].age;
		oldestPos = i;
	    }
	}

	bb_assert(oldestPos >= 0);
	bb_assert(oldestPos < MAX_TEXTURES);
	bb_assert(oldestAge != ~(uint32_t)0);

	texObj = renderInfo->textures + oldestPos;

	/* unlink FrameInfo pointer */
	if (texObj->frameInfo)
	    texObj->frameInfo->canvasPrivate = NULL;

	/* update/init texObj fields */
	texObj->age = clock++;	/* XXX handle clock wrap-around! */
	texObj->frameInfo = canvas->frameList->getFrame(frameNumber);
	for (i = 0; i < MAX_IMAGE_LEVELS; i++)
	    texObj->valid[i].width = texObj->valid[i].height = -1;
	texObj->anyLoaded = GL_FALSE;
	canvas->frameList->getFrame(frameNumber)->canvasPrivate = texObj;
    }

    return texObj;
}


static void gltexture_Render(Canvas *canvas, 
                   int frameNumber,
                   const Rectangle *imageRegion,
                   int destX, int destY, float zoom, int lod)
{
    RenderInfo *renderInfo = (RenderInfo *) canvas->rendererPrivateData;
    TextureObject *texObj;
    GLfloat s0, t0, s1, t1;
    GLfloat x0, y0, x1, y1;
    int32_t lodScale;
	int localFrameNumber;
    Rectangle region;
    Image *image;

#if 0
    printf("gltexture::Render %d, %d  %d x %d  at %d, %d  zoom=%f  lod=%d\n",
	   imageRegion->x, imageRegion->y,
	   imageRegion->width, imageRegion->height,
	   destX, destY, zoom, lod);
#endif

    UpdateProjectionAndViewport(canvas->width, canvas->height);
    glEnable(GL_TEXTURE_2D);


	if (canvas->frameList->stereo) {
	  localFrameNumber = frameNumber *2; /* we'll display left frame only */
	}
	else {
	  localFrameNumber = frameNumber;
	}

    /*
     * Compute possibly reduced-resolution image region to display.
     */
	bb_assert(lod <= canvas->frameList->getFrame(localFrameNumber)->maxLOD);
    lodScale = 1 << lod;
    region.x = imageRegion->x / lodScale;
    region.y = imageRegion->y / lodScale;
    region.width = imageRegion->width / lodScale;
    region.height = imageRegion->height / lodScale;
    zoom *= (float) lodScale;

    /* Pull the image from our cache */
    image = canvas->imageCache->GetImage( localFrameNumber, &region, lod);
    if (image == NULL) {
	/* error has already been reported */
	return;
    }

    /* get texture object */
    texObj = GetTextureObject(renderInfo, canvas, localFrameNumber);
    bb_assert(texObj);
    bb_assert(texObj->frameInfo == canvas->frameList->getFrame(localFrameNumber));

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
	texObj->width = MIN2(MinPowerOf2(image->width), renderInfo->maxTextureWidth);
	texObj->height = MIN2(MinPowerOf2(image->height), renderInfo->maxTextureHeight);
	/* make initial image (undefined contents) */
        w = texObj->width;
        h = texObj->height;
	level = 0;
	while (w > 1 || h > 1) {
	    glTexImage2D(GL_TEXTURE_2D, level, renderInfo->texIntFormat, 
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
	    !RectContainsRect(&texObj->valid[lod], &region)) {
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

	    glTexSubImage2D(GL_TEXTURE_2D, lod,
			    region.x, region.y,
			    region.width, region.height,
			    renderInfo->texFormat, GL_UNSIGNED_BYTE,
			    image->imageData);

	    if (texObj->anyLoaded)
		texObj->valid[lod] = RectUnionRect(&texObj->valid[lod], imageRegion);
	    else
		texObj->valid[lod] = *imageRegion;
	    texObj->anyLoaded = GL_TRUE;
	}

	/* Choose active mipmap level */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, lod);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, lod);

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
        x0 = destX;
        y0 = destY;
        x1 = destX + region.width * zoom;
        y1 = destY + region.height * zoom;

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
			renderInfo->texFormat, GL_UNSIGNED_BYTE,
			image->imageData);
	/* invalidate valid region, for sake of first path, above */
	texObj->valid[lod].x = 0;
	texObj->valid[lod].y = 0;
	texObj->valid[lod].width = 0;
	texObj->valid[lod].height = 0;

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

	x0 = destX;
	y0 = destY;
	x1 = destX + region.width * zoom;
	y1 = destY + region.height * zoom;

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
	const int tileWidth = renderInfo->maxTextureWidth;
	const int tileHeight = renderInfo->maxTextureHeight;
	int row, col, width, height;

	/* invalidate valid region, for sake of first path, above */
	texObj->valid[lod].x = 0;
	texObj->valid[lod].y = 0;
	texObj->valid[lod].width = 0;
	texObj->valid[lod].height = 0;

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
				renderInfo->texFormat, GL_UNSIGNED_BYTE,
				image->imageData);

		/* tex coords */
		s0 = 0;
		t0 = 0;
		s1 = (float) width / renderInfo->maxTextureWidth;
		t1 = (float) height / renderInfo->maxTextureHeight;
		if (image->imageFormat.rowOrder == BOTTOM_TO_TOP) {
		    /* invert texcoords */
		    GLfloat temp = t0;
		    t0 = t1;
		    t1 = temp;
		}

		/* vertex coords */
		x0 = destX + col * zoom;
		y0 = destY + row * zoom;
		x1 = x0 + width * zoom;
		y1 = y0 + height * zoom;

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

#ifdef DEBUG
		/* debug */
		glDisable(GL_TEXTURE_2D);
		glColor3f(1, 0, 0);
		glBegin(GL_LINE_LOOP);
		glVertex2f(x0+1, y0+1);
		glVertex2f(x0+1, y1-1);
		glVertex2f(x1-1, y1-1);
		glVertex2f(x1-1, y0+1);
		glEnd();
		glEnable(GL_TEXTURE_2D);
		glColor3f(1, 1, 1);
#endif
	    } /* for col */
	} /* for row */

	texObj->valid[lod].x = 0;
	texObj->valid[lod].y = 0;
	texObj->valid[lod].width = 0;
	texObj->valid[lod].height = 0;
    }

    /* debug */
    {
        int err = glGetError();
        if (err) {
            ERROR("OpenGL Error 0x%x\n", err);
        }
    }

    /* Have to release the image, or the cache will fill up */
    canvas->imageCache->ReleaseImage( image);

    glDisable(GL_TEXTURE_2D);
}

static void gltexture_DestroyRenderer(Canvas *canvas)
{
    RenderInfo *renderInfo = (RenderInfo *) canvas->rendererPrivateData;
    free(renderInfo);
}

/*
 * After the canvas has been created, as well as the corresponding X window,
 * this function is called to do any renderer-specific setup.
 */
 MovieStatus
gltexture_Initialize(Canvas *canvas, const ProgramOptions *)
{
    RenderInfo *renderInfo;

    renderInfo = (RenderInfo *)calloc(1, sizeof(RenderInfo));
    if (!renderInfo) {
	return MovieFailure;
    }
          
    /* Get max texture size (XXX use proxy?) */
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &renderInfo->maxTextureWidth);
    renderInfo->maxTextureHeight = renderInfo->maxTextureWidth;

    if (renderInfo->maxTextureWidth >= 4096 ||
	renderInfo->maxTextureHeight >= 4096) {
	/* XXX NVIDIA's GeForce reports 4Kx4K but that size doesn't
	 * actually work!
	 * Furthermore, smaller textures seem to be faster when tiling.
	 */
	renderInfo->maxTextureWidth = 2048;
	renderInfo->maxTextureHeight = 2048;
    }
    INFO("Max Texture Size: %d x %d",
	 renderInfo->maxTextureWidth,
	 renderInfo->maxTextureHeight);

	renderInfo->texIntFormat = GL_RGB;
	renderInfo->texFormat = GL_RGB;

    /* If we're going to try to use the PixelDataRange extension, enable it
     * as well as our custom memory management
     */
    canvas->ImageDataAllocator = DefaultImageDataAllocator;
    canvas->ImageDataDeallocator = DefaultImageDataDeallocator;

    canvas->rendererPrivateData = renderInfo;

    /* plug in our functions into the canvas */
    canvas->Render = gltexture_Render;
    canvas->DestroyRenderer = gltexture_DestroyRenderer;

    return MovieSuccess;
}

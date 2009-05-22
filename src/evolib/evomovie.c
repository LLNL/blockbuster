/********************************************************************
 * $Id: evomovie.c,v 1.1 2007/06/13 18:59:29 wealthychef Exp $
 ********************************************************************/


/* ====================================================================
 * The CEI Inc. Open Software License, Version 1.0
 *
 * Copyright (c) 2001 Computational Engineering Intl. Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Computational Engineering Intl. Inc (http://www.ceintl.com)."
 *    Additionally, this acknowledgment must appear with the software
 *    itself, if and wherever such third-party acknowledgments normally 
 *    appear.
 *
 * 4. The names "CEI" and "Computational Engineering Intl. Inc." must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact support@ceintl.com.
 *
 * 5. Modified versions of this software must be plainly marked as
 *    such, and must not be misrepresented as being the original software.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL COMPUTATIONAL ENGINEERING INTL.
 * INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "evomovie.h"


#include "jpeglib.h"


#ifdef WIN32
unsigned long htonl(unsigned long val) {
  unsigned char *a;
  unsigned char tmp;
  a = (unsigned char*)&val;
  tmp = a[0]; a[0] = a[3]; a[3] = tmp;
  tmp = a[1]; a[1] = a[2]; a[2] = tmp;
  return val;
}

unsigned long ntohl(unsigned long val) {
  unsigned char *a;
  unsigned char tmp;
  a = (unsigned char*)&val;
  tmp = a[0]; a[0] = a[3]; a[3] = tmp;
  tmp = a[1]; a[1] = a[2]; a[2] = tmp;
  return val;
}
#else
#include <netinet/in.h>
#endif



#define EVO_MOVIE_FILE_VERSION 1


/* PUBLIC DEFINITIONS */
char EVOMovieErrorString[256];





/* PRIVATE TYPES */


/* This data type is pointed to by the jpegStuff member
 * in the EVOMovieType type.
 */
typedef struct {
  /* jpeg compression object */
  struct jpeg_compress_struct cinfo;
  /* jpeg decompression object */
  struct jpeg_decompress_struct dinfo;
  /* default jpeg error handler */
  struct jpeg_error_mgr jerr;
} JpegStuffType, *JpegStuffPtrType;





/* FORWARD DECLARATIONS */

/* flushes new movie index to file */
static EVOMovieReturnStatusType close_new_movie(EVOMoviePtrType movie);
static void write_jpeg_header(EVOMoviePtrType movie, unsigned int quality);
static void read_jpeg_header(EVOMoviePtrType movie);
static EVOMovieReturnStatusType write_evo_rle(FILE *fp, EVOImagePtrType image);
static EVOMovieReturnStatusType write_rle(FILE *fp, unsigned char *start,
                                          unsigned char *end);
static EVOMovieReturnStatusType read_evo_rle(FILE *fp, EVOImagePtrType image);
static EVOMovieReturnStatusType read_rle(FILE *fp, unsigned char *start,
                                         int len);
static EVOMovieReturnStatusType write_evo_jpeg(FILE *fp,
                                          struct jpeg_compress_struct *cinfo,
                                               EVOImagePtrType image);
static EVOMovieReturnStatusType read_evo_jpeg(FILE *fp,
                                          struct jpeg_decompress_struct *cinfo,
                                              EVOImagePtrType image);
static void free_movies(EVOMovieListPtrType *evoMovies);







/* create a new EVO movie file; return NULL on error */
EVOMoviePtrType evo_movie_new_file(char *fileName, unsigned int stereo,
                                   unsigned int xdim, unsigned int ydim, 
                                   unsigned int nFrames,
                                   EVOMovieFormatType movieFormat,
                                   unsigned int quality) {

  EVOMoviePtrType evoMovie;
  unsigned int val;
  unsigned int len;
  unsigned int size;


  if ((evoMovie = (EVOMoviePtrType)malloc(sizeof(EVOMovieType))) == NULL) {
    sprintf(EVOMovieErrorString, "can't malloc EVOMovieType");
    return (EVOMoviePtrType)NULL;
  }
  if ((evoMovie->jpegStuff =
    (void*)malloc(sizeof(JpegStuffType))) == NULL) {
    sprintf(EVOMovieErrorString, "can't malloc JpegStuffType");
    free(evoMovie);
    return (EVOMoviePtrType)NULL;
  }

  evoMovie->newMovie = 1;
  evoMovie->movieFormat = movieFormat;
  evoMovie->stereo = stereo;
  evoMovie->xdim = xdim;
  evoMovie->ydim = ydim;
  evoMovie->nFrames = nFrames;

  /* get arrays for image offsets/lengths */
  if (evoMovie->stereo) {
    len = evoMovie->nFrames * 2;
  } else {
    len = evoMovie->nFrames;
  }
  size = sizeof(unsigned int) * len;
  if ((evoMovie->offsets = (unsigned int*)malloc(size)) == NULL) {
    sprintf(EVOMovieErrorString, "can't allocate offsets %d", size);
    free(evoMovie->jpegStuff);
    free(evoMovie);
    return (EVOMoviePtrType)NULL;
  }
  if ((evoMovie->lengths = (unsigned int*)malloc(size)) == NULL) {
    sprintf(EVOMovieErrorString, "can't allocate lengths %d", size);
    free(evoMovie->offsets);
    free(evoMovie->jpegStuff);
    free(evoMovie);
    return (EVOMoviePtrType)NULL;
  }

  /* open the underlying file */
  if ((evoMovie->movieFP = fopen(fileName, "wb")) == NULL) {
    sprintf(EVOMovieErrorString,
      "can't open new movie file <%s>", fileName);
    free(evoMovie->lengths);
    free(evoMovie->offsets);
    free(evoMovie->jpegStuff);
    free(evoMovie);
    return (EVOMoviePtrType)NULL;
  }



  /* write the movie file header info */

  /* magic 'number' */
  fprintf(evoMovie->movieFP, "EVOM");

  /* file format version */
  val = htonl(EVO_MOVIE_FILE_VERSION);
  fwrite(&val, sizeof(unsigned int), 1, evoMovie->movieFP);

  /* image format type */
  if (evoMovie->movieFormat == MOVIERLE)
    val = htonl(0);
  else if (evoMovie->movieFormat == MOVIEJPEG)
    val = htonl(1);
  fwrite(&val, sizeof(unsigned int), 1, evoMovie->movieFP);

  /* stereo flag */
  if (evoMovie->stereo)
    val = htonl(1);
  else
    val = htonl(0);
  fwrite(&val, sizeof(unsigned int), 1, evoMovie->movieFP);

  /* x dimension */
  val = htonl(evoMovie->xdim);
  fwrite(&val, sizeof(unsigned int), 1, evoMovie->movieFP);

  /* y dimension */
  val = htonl(evoMovie->ydim);
  fwrite(&val, sizeof(unsigned int), 1, evoMovie->movieFP);

  /* number of frames */
  val = htonl(evoMovie->nFrames);
  fwrite(&val, sizeof(unsigned int), 1, evoMovie->movieFP);

  /* location of movie index table in file (current position) */
  evoMovie->movieTableLocation = 
    (unsigned int)ftell(evoMovie->movieFP) + sizeof(unsigned int);
  val = htonl(evoMovie->movieTableLocation);
  fwrite(&val, sizeof(unsigned int), 1, evoMovie->movieFP);

  /* put dummy data into movie index table, rewrite later.... */
  fwrite(evoMovie->offsets, sizeof(unsigned int), len, evoMovie->movieFP);
  fwrite(evoMovie->lengths, sizeof(unsigned int), len, evoMovie->movieFP);

  /* the next frame we'll be adding to the file */
  evoMovie->currentFrame = -1;

  /* if jpeg add the jpeg header data */
  if (evoMovie->movieFormat == MOVIEJPEG)
    write_jpeg_header(evoMovie, quality);

  if (ferror(evoMovie->movieFP) != 0) {
    sprintf(EVOMovieErrorString,
      "can't write header in new movie file <%s>", fileName);
    fclose(evoMovie->movieFP);
    free(evoMovie->lengths);
    free(evoMovie->offsets);
    free(evoMovie->jpegStuff);
    free(evoMovie);
    return (EVOMoviePtrType)NULL;
  }

  return evoMovie;

}




/* open a EVO movie file */
EVOMoviePtrType evo_movie_open_file(char *fileName) {

  unsigned int val;
  unsigned int len;
  unsigned int i;
  char tag[4];
  EVOMoviePtrType evoMovie;


  if ((evoMovie = (EVOMoviePtrType)malloc(sizeof(EVOMovieType))) == NULL) {
    sprintf(EVOMovieErrorString, "can't malloc EVOMovieType");
    return (EVOMoviePtrType)NULL;
  }
  if ((evoMovie->jpegStuff =
    (void*)malloc(sizeof(JpegStuffType))) == NULL) {
    sprintf(EVOMovieErrorString, "can't malloc JpegStuffType");
    free(evoMovie);
    return (EVOMoviePtrType)NULL;
  }

  evoMovie->newMovie = 0;

  if ((evoMovie->movieFP = fopen(fileName, "rb")) == NULL) {
    sprintf(EVOMovieErrorString,
      "can't open movie file <%s>", fileName);
    free(evoMovie->jpegStuff);
    free(evoMovie);
    return (EVOMoviePtrType)NULL;
  }

  /* check the magic number */
  fread(tag, sizeof(char), 4, evoMovie->movieFP);
  if (strncmp(tag, "EVOM", 4)) {
    sprintf(EVOMovieErrorString,
      "file <%s> is not a EVO Movie file", fileName);
    fclose(evoMovie->movieFP);
    free(evoMovie->jpegStuff);
    free(evoMovie);
    return (EVOMoviePtrType)NULL;
  }

  /* check the file format version number */
  fread(&val, sizeof(unsigned int), 1, evoMovie->movieFP);
  if (ntohl(val) != EVO_MOVIE_FILE_VERSION) {
    sprintf(EVOMovieErrorString,"wrong file version");
    fclose(evoMovie->movieFP);
    free(evoMovie->jpegStuff);
    free(evoMovie);
    return (EVOMoviePtrType)NULL;
  }

  /* get the image format */
  fread(&val, sizeof(unsigned int), 1, evoMovie->movieFP);
  val = ntohl(val);
  if (val == 0)
    evoMovie->movieFormat = MOVIERLE;
  else if (val == 1)
    evoMovie->movieFormat = MOVIEJPEG;
  else {
    sprintf(EVOMovieErrorString,"unknown image format");
    fclose(evoMovie->movieFP);
    free(evoMovie->jpegStuff);
    free(evoMovie);
    return (EVOMoviePtrType)NULL;
  }

  /* get the stereo flag */
  fread(&val, sizeof(unsigned int), 1, evoMovie->movieFP);
  val = ntohl(val);
  if (val == 0)
    evoMovie->stereo = 0;
  else 
    evoMovie->stereo = 1;
  
  /* x dimension */
  fread(&val, sizeof(unsigned int), 1, evoMovie->movieFP);
  evoMovie->xdim = ntohl(val);

  /* y dimension */
  fread(&val, sizeof(unsigned int), 1, evoMovie->movieFP);
  evoMovie->ydim = ntohl(val);

  /* number of frames */
  fread(&val, sizeof(unsigned int), 1, evoMovie->movieFP);
  evoMovie->nFrames = ntohl(val);

  /* location of movie index */
  fread(&val, sizeof(unsigned int), 1, evoMovie->movieFP);
  evoMovie->movieTableLocation = ntohl(val);

  /* jump to movie index location in the file */
  fseek(evoMovie->movieFP, evoMovie->movieTableLocation, SEEK_SET);

  if (evoMovie->stereo) {
    len = evoMovie->nFrames * 2;
  } else {
    len = evoMovie->nFrames;
  }
  if ((evoMovie->offsets =
       (unsigned int*)malloc(len*sizeof(unsigned int))) == NULL) {
    sprintf(EVOMovieErrorString,"can't malloc offsets %d", len);
    fclose(evoMovie->movieFP);
    free(evoMovie->jpegStuff);
    free(evoMovie);
    return (EVOMoviePtrType)NULL;
  }
  if ((evoMovie->lengths =
       (unsigned int*)malloc(len*sizeof(unsigned int))) == NULL) {
    sprintf(EVOMovieErrorString,"can't malloc lengths %d", len);
    fclose(evoMovie->movieFP);
    free(evoMovie->jpegStuff);
    free(evoMovie);
    return (EVOMoviePtrType)NULL;
  }

  fread(evoMovie->offsets, sizeof(unsigned int), len, evoMovie->movieFP);
  fread(evoMovie->lengths, sizeof(unsigned int), len, evoMovie->movieFP);

  for (i=0; i<len; i++) {
    evoMovie->offsets[i] = ntohl(evoMovie->offsets[i]);
    evoMovie->lengths[i] = ntohl(evoMovie->lengths[i]);
  }

  if (evoMovie->movieFormat == MOVIEJPEG)
    read_jpeg_header(evoMovie);

  /* the next frame we'll play */
  evoMovie->currentFrame = 0;
  if (ferror(evoMovie->movieFP) != 0) {
    sprintf(EVOMovieErrorString,"can't read header in movie file <%s>",
            fileName);
    fclose(evoMovie->movieFP);
    free(evoMovie->jpegStuff);
    free(evoMovie);
    return (EVOMoviePtrType)NULL;
  }


  return evoMovie;

}




/* purge movie from memory */
EVOMovieReturnStatusType evo_movie_close_file(EVOMoviePtrType movie) {

  EVOMovieReturnStatusType ret;


  ret = MOVIEOK;
  if (movie->movieFP) {
    /* got an open file */

    if (movie->newMovie)
      ret = close_new_movie(movie);

    fclose(movie->movieFP);
    movie->movieFP = (FILE*)NULL;
    if (movie->movieFormat == MOVIEJPEG)
      if (movie->newMovie)
        jpeg_destroy_compress(&(((JpegStuffPtrType)
                                 (movie->jpegStuff))->cinfo));
      else
        jpeg_destroy_decompress(&(((JpegStuffPtrType)
                                   (movie->jpegStuff))->dinfo));
  }
  movie->newMovie = 0;
  movie->stereo = 0;
  movie->xdim = 0;
  movie->ydim = 0;
  movie->nFrames = 0;
  if (movie->offsets) {
    free(movie->offsets);
    movie->offsets = (unsigned int*)NULL;
  }
  if (movie->lengths) {
    free(movie->lengths);
    movie->lengths = (unsigned int*)NULL;
  }
  free(movie->jpegStuff);
  free(movie);

  return ret;

}




/* flushes new movie index to file */
static EVOMovieReturnStatusType close_new_movie(EVOMoviePtrType movie) {

  unsigned int len;
  unsigned int i;
  
  if (movie->stereo) {
    len = movie->nFrames * 2;
  } else {
    len = movie->nFrames;
  }

  /* convert the indices to network byte order */
  for (i=0; i<len; i++) {
    movie->offsets[i] = htonl(movie->offsets[i]);
    movie->lengths[i] = htonl(movie->lengths[i]);
  }


  /* jump to movie index location in the file */
  fseek(movie->movieFP, movie->movieTableLocation, SEEK_SET);

  /* put dummy data into movie index table, rewrite later.... */
  fwrite(movie->offsets, sizeof(unsigned int), len, movie->movieFP);
  fwrite(movie->lengths, sizeof(unsigned int), len, movie->movieFP);

  if (ferror(movie->movieFP) != 0) {
    sprintf(EVOMovieErrorString, "can't write index table in new movie file");
    return MOVIEERR;
  }

  return MOVIEOK;

}




/* add an image to the new movie; images must be added sequentially */
EVOMovieReturnStatusType evo_movie_add_image(EVOMoviePtrType movie, EVOImagePtrType image) {

  EVOMovieReturnStatusType ret;
  

  movie->currentFrame++;
  movie->offsets[movie->currentFrame] = (unsigned int)ftell(movie->movieFP);

  if (movie->movieFormat == MOVIERLE)
    ret = write_evo_rle(movie->movieFP, image);
  else if (movie->movieFormat == MOVIEJPEG)
    ret = write_evo_jpeg(movie->movieFP,
                         &(((JpegStuffPtrType)(movie->jpegStuff))->cinfo),
                         image);

  movie->lengths[movie->currentFrame] = (unsigned int)ftell(movie->movieFP)
    - movie->offsets[movie->currentFrame] - 1;

  return ret;

}




/* return the image */
EVOMovieReturnStatusType evo_movie_get_image(EVOMoviePtrType movie,
                                             EVOImagePtrType image,
                                             unsigned int imageNo,
                                             EVOStereoEyeType whichEye) {

  EVOMovieReturnStatusType ret;

  if (imageNo>=movie->nFrames) {
    sprintf(EVOMovieErrorString, "frame %d out of range [0..%d]",
            imageNo, (movie->nFrames)-1);
    return MOVIEERR;
  }

  if (movie->stereo)
    if (whichEye == LEFTEYE)
      movie->currentFrame = imageNo * 2;
    else
      movie->currentFrame = imageNo * 2 + 1;
  else
    movie->currentFrame = imageNo;

  /* jump to right frame in the file */
  if (fseek(movie->movieFP, movie->offsets[movie->currentFrame], SEEK_SET)
      != 0) {
    sprintf(EVOMovieErrorString, "can't seek to frame %d in movie file",
            imageNo);
    return MOVIEERR;
  }

  if (movie->movieFormat == MOVIERLE)
    ret = read_evo_rle(movie->movieFP, image);
  else if (movie->movieFormat == MOVIEJPEG)
    ret = read_evo_jpeg(movie->movieFP,
                        &(((JpegStuffPtrType)(movie->jpegStuff))->dinfo),
                        image);

    return ret;

}




static void read_jpeg_header(EVOMoviePtrType movie) {

  struct jpeg_decompress_struct *dinfo;

  dinfo = &(((JpegStuffPtrType)(movie->jpegStuff))->dinfo);
  /* printf("MFK: be sure to add real jpeg error handler...\n"); */
  dinfo->err = jpeg_std_error(&(((JpegStuffPtrType)(movie->jpegStuff))->jerr));
  /* Now we can initialize the JPEG compression object. */
  jpeg_create_decompress(dinfo);

  jpeg_stdio_src(dinfo, movie->movieFP);

  jpeg_read_header(dinfo, FALSE);

}




static void write_jpeg_header(EVOMoviePtrType movie, unsigned int quality) {

  struct jpeg_compress_struct *cinfo;


  /*printf("MFK: be sure to add real jpeg error handler...\n"); */
  cinfo = &(((JpegStuffPtrType)(movie->jpegStuff))->cinfo);
  cinfo->err = jpeg_std_error(&(((JpegStuffPtrType)(movie->jpegStuff))->jerr));
  /* Now we can initialize the JPEG compression object. */
  jpeg_create_compress(cinfo);


  jpeg_stdio_dest(cinfo, movie->movieFP);

  /* image width and height, in pixels */
  cinfo->image_width = movie->xdim;
  cinfo->image_height = movie->ydim;
  /* # of color components per pixel */
  cinfo->input_components = 3;
  /* colorspace of input image */
  cinfo->in_color_space = JCS_RGB;

  jpeg_set_defaults(cinfo);

  jpeg_set_quality(cinfo, quality, TRUE  /* limit to baseline-JPEG values */);
  jpeg_write_tables(cinfo);

}




/* Image functions */





/* allocate a new empty image */
EVOImagePtrType evo_movie_alloc_image(unsigned int xdim, unsigned int ydim) {

  EVOImagePtrType image;

  if ((image = (EVOImagePtrType)malloc(sizeof(EVOImageType))) == NULL) {
    sprintf(EVOMovieErrorString, "can't malloc image");
    return NULL;
  }

   if ((image->pixels =
        (unsigned char*)malloc(xdim*ydim*3*sizeof(unsigned char))) == NULL) {
    sprintf(EVOMovieErrorString, "can't malloc pixels");
    free(image);
    return NULL;
  }

  image->xdim = xdim;
  image->ydim = ydim;

  return image;

}




/* free an image */
void evo_movie_free_image(EVOImagePtrType image) {

  if (image) {
    free(image->pixels);
    free(image);
  }

}




/* write image in rle format to an open evo file */
static EVOMovieReturnStatusType write_evo_rle(FILE *fp,
                                              EVOImagePtrType image) {

  unsigned char *start, *end;
  

  /* process the red pixels */
  start = image->pixels;
  end = image->pixels+(image->xdim*image->ydim*3)-3;
  if (write_rle(fp, start, end) == MOVIEERR)
    return MOVIEERR;

  /* process the green pixels */
  start = image->pixels+1;
  end = image->pixels+(image->xdim*image->ydim*3)-2;
  if (write_rle(fp, start, end) == MOVIEERR)
    return MOVIEERR;

  /* process the blue pixels */
  start = image->pixels+2;
  end = image->pixels+(image->xdim*image->ydim*3)-1;
  if (write_rle(fp, start, end) == MOVIEERR)
    return MOVIEERR;

  return MOVIEOK;

}




static EVOMovieReturnStatusType write_rle(FILE *fp, unsigned char *start,
                                          unsigned char *end) {

  unsigned char *previous, *next;
  unsigned int runCount;
  unsigned char count;
  unsigned char tmp[128];


  while (start <= end) {

    if ( ((start+3) <= end) && (*start == *(start+3)) ) {
      /* look for encoded run */
      runCount = 1;
      next = start+3;
      while ( (next <= end) && (runCount <= 127) && (*start == *next) ) {
        runCount++;
        next+=3;
      }
 
      /* set high order bit to signify encoded run vs. literal run */
      count = (unsigned char)(runCount-1) | (unsigned char)128;

      if (fwrite(&count, sizeof(unsigned char), 1, fp) != 1) {
        sprintf(EVOMovieErrorString, "write_rle can't write count to file");
        return MOVIEERR;
      }
      if (fwrite(start, sizeof(unsigned char), 1, fp) != 1) {
        sprintf(EVOMovieErrorString, "write_rle can't write value to file");
        return MOVIEERR;
      }
 
    } else {
      /* look for literal run */
      runCount = 1;
      previous = start;
      next = start+3;
      tmp[0] = *previous;
      while ( (next <= end) && (runCount <= 127) ) {
        if (*previous == *next) {
          runCount--;
          break;
        }
        tmp[runCount] = *next;
        runCount++;
        next+=3;
        previous+=3;
      }

      count = (unsigned char)(runCount-1);
      if (fwrite(&count, sizeof(unsigned char), 1, fp) != 1) {
        sprintf(EVOMovieErrorString, "write_rle can't write count to file");
        return MOVIEERR;
      }
      if (fwrite(tmp, sizeof(unsigned char), runCount, fp)
          != (unsigned int)runCount) {
        sprintf(EVOMovieErrorString, "write_rle can't write values to file");
        return MOVIEERR;
      }
     }

    start += runCount*3;
  }

  return MOVIEOK;

}




/* read image in rle format from an open evo file */
static EVOMovieReturnStatusType read_evo_rle(FILE *fp, EVOImagePtrType image) {

  unsigned char *start;
  

  /* process the red pixels */
  start = image->pixels;
  if (read_rle(fp, start, image->xdim*image->ydim) == MOVIEERR)
    return MOVIEERR;

  /* process the green pixels */
  start = image->pixels+1;
  if (read_rle(fp, start, image->xdim*image->ydim) == MOVIEERR)
    return MOVIEERR;

  /* process the blue pixels */
  start = image->pixels+2;
  if (read_rle(fp, start, image->xdim*image->ydim) == MOVIEERR)
    return MOVIEERR;

  return MOVIEOK;

}




static EVOMovieReturnStatusType read_rle(FILE *fp, unsigned char *start,
                                         int len) {

  unsigned char count;
  unsigned int runCount;
  unsigned char value;
  unsigned int i;
  unsigned char tmp[128];

  
  while (len > 0) {
    if (fread(&count, sizeof(unsigned char), 1, fp) != 1) {
      sprintf(EVOMovieErrorString, "read_rle can't read rle count from file");
      return MOVIEERR;
    }
    if (count & (unsigned char)128) {
      /* got an encoded run */
      if (fread(&value, sizeof(unsigned char), 1, fp) != 1) {
        sprintf(EVOMovieErrorString, "read_rle can't read value from file");
        return MOVIEERR;
      }
      /* strip off the encoded/literal flag bit */
      runCount = count & (unsigned char)127;
      runCount++;

      /* put the value into the buffer n times */
      for (i=0; i<runCount; i++, start+=3)
        *start = value;

    } else {
      /* got a literal run */
      runCount = count+1;
      
      if (fread(tmp, sizeof(unsigned char), runCount, fp) != runCount) {
        sprintf(EVOMovieErrorString, "read_rle can't read values from file");
        return MOVIEERR;
      }

      for (i=0; i<runCount; i++, start+=3)
        *start = tmp[i];

    }

    len -= runCount;

  } /* while len>0 */

  return MOVIEOK;

}




/* write image in rle format to an open evo file */
static EVOMovieReturnStatusType write_evo_jpeg(FILE *fp, struct jpeg_compress_struct *cinfo,
                                               EVOImagePtrType image) {

  JSAMPLE *image_buffer;        /* Points to large array of R,G,B-order data */
  JSAMPROW row_pointer[1];      /* pointer to JSAMPLE row[s] */
  int row_stride;               /* physical row width in image buffer */
  static int first=1;
  
  image_buffer = (JSAMPLE*)image->pixels;
  jpeg_start_compress(cinfo, FALSE);

  row_stride = image->xdim * 3;       /* JSAMPLEs per row in image_buffer */
  
    
  while (cinfo->next_scanline < cinfo->image_height) {
    row_pointer[0] = & image_buffer[cinfo->next_scanline * row_stride];
    (void) jpeg_write_scanlines(cinfo, row_pointer, 1);
  }


  jpeg_finish_compress(cinfo);

  return MOVIEOK;

}




/* read image in rle format from an open evo file */
static EVOMovieReturnStatusType read_evo_jpeg(FILE *fp, struct jpeg_decompress_struct *cinfo,
                                              EVOImagePtrType image) {

  JSAMPLE *image_buffer;        /* Points to large array of R,G,B-order data */
  JSAMPROW row_pointer[1];      /* pointer to JSAMPLE row[s] */
  int row_stride;               /* physical row width in image buffer */

  
  image_buffer = (JSAMPLE*)image->pixels;
  cinfo->src->fill_input_buffer(cinfo);
  jpeg_read_header(cinfo, TRUE);
  jpeg_start_decompress(cinfo);

  row_stride = image->xdim * 3; /* JSAMPLEs per row in image_buffer */
  
    
  while (cinfo->output_scanline < cinfo->output_height) {
    row_pointer[0] = & image_buffer[cinfo->output_scanline * row_stride];
    (void) jpeg_read_scanlines(cinfo, row_pointer, 1);
  }


  jpeg_finish_decompress(cinfo);

  return MOVIEOK;

}




/* MOVIE LIST FUNCTIONS */




/* open a list of movie files; returns NULL on error */
EVOMovieListPtrType evo_movie_open_files(int nFiles, char *fileNames[]) {

  EVOMovieListPtrType evoMovies;
  unsigned int i;
  unsigned int size;


  if (nFiles < 1) {
    sprintf(EVOMovieErrorString, "nFiles (%d) < 1", nFiles);
    return (EVOMovieListPtrType)NULL;
  }

  if ((evoMovies =
       (EVOMovieListPtrType)malloc(sizeof(EVOMovieListType))) == NULL) {
    sprintf(EVOMovieErrorString, "can't malloc EVOMovieListPtrType");
    return (EVOMovieListPtrType)NULL;
  }
  evoMovies->nFrames = 0;
  evoMovies->nMovieFiles = nFiles;
  size = nFiles * sizeof(EVOMoviePtrType);
  if ((evoMovies->movie = (EVOMoviePtrType*)malloc(size)) == NULL) {
    sprintf(EVOMovieErrorString, "can't malloc EVOMoviePtrType*");
    free(evoMovies);
    return (EVOMovieListPtrType)NULL;
  }

  for (i=0; i<evoMovies->nMovieFiles; i++) {
    if ((evoMovies->movie[i] = evo_movie_open_file(fileNames[i])) == NULL) {
      evoMovies->nMovieFiles = i;  /* used by free_movies to clean up */
      free_movies(&evoMovies);
      return (EVOMovieListPtrType)NULL;
    }
    if (i==0) {
      /* first movie file sets the overall characteristics */
      evoMovies->stereo = evoMovies->movie[0]->stereo;
      evoMovies->xdim = evoMovies->movie[0]->xdim;
      evoMovies->ydim = evoMovies->movie[0]->ydim;

      evoMovies->nFrames = evoMovies->movie[0]->nFrames;

    } else {
      /* make sure all the movies are similar */
      if ((evoMovies->stereo != evoMovies->movie[i]->stereo) ||
        (evoMovies->xdim != evoMovies->movie[i]->xdim) ||
        (evoMovies->ydim != evoMovies->movie[i]->ydim)) {
        evoMovies->nMovieFiles = i;  /* used by free_movies to clean up */
        free_movies(&evoMovies);
        sprintf(EVOMovieErrorString,
                "movie file %d doesn't match the size/stereo of the first", i);
        return (EVOMovieListPtrType)NULL;
      }
      evoMovies->nFrames += evoMovies->movie[i]->nFrames;
    }
  } /* for i */

  return evoMovies;

}




/* clean up a evo_movie_open_files failure */
static void free_movies(EVOMovieListPtrType *evoMovies) {

  unsigned int i;


  for (i=0; i<(*evoMovies)->nMovieFiles; i++)
    evo_movie_close_file((*evoMovies)->movie[i]);

  free((*evoMovies)->movie);

  free(*evoMovies);

}




/* add a new movie file to the animation */
EVOMovieReturnStatusType  evo_movie_add_file(EVOMovieListPtrType movies,
                                             char *fileName) {

  EVOMovieReturnStatusType ret;
  EVOMoviePtrType movie;
  int size;
  

  /* open the new movie file and make sure it matches what we
   * already have.
   */

  if ((movie = evo_movie_open_file(fileName)) == NULL) {
    return MOVIEERR;
  }

  if ((movies->stereo != movie->stereo) ||
      (movies->xdim != movie->xdim) ||
      (movies->ydim != movie->ydim)) {
    evo_movie_close_file(movie);
    /* free(movie); */
    sprintf(EVOMovieErrorString,
            "movie file doesn't match the size/stereo of the others");
    return MOVIEERR;
  }
  

  /* since it matches the others, add it to the list */
  size = (movies->nMovieFiles + 1) * sizeof(EVOMoviePtrType);
  if ((movies->movie = (EVOMoviePtrType*)realloc(movies->movie, size))
      == NULL) {
    sprintf(EVOMovieErrorString, "can't realloc EVOMoviePtrType*");
    free(movies);
    return MOVIEERR;
  }

  movies->movie[movies->nMovieFiles] = movie;
  movies->nMovieFiles++;
  movies->nFrames += movie->nFrames;
  
  
  return MOVIEOK;
  
}




/* close a list of movie files */
EVOMovieReturnStatusType evo_movie_close_files(EVOMovieListPtrType movies) {

  unsigned int i;
  EVOMovieReturnStatusType ret;

  ret = MOVIEOK;
  for (i=0; i<movies->nMovieFiles; i++)
    if (evo_movie_close_file(movies->movie[i]) != MOVIEOK)
      ret = MOVIEERR;

  free(movies->movie);

  free(movies);

  return ret;

}




/* return an image */
EVOMovieReturnStatusType evo_movies_get_image(EVOMovieListPtrType movies,
   EVOImagePtrType image, unsigned int imageNo, EVOStereoEyeType whichEye) {

  unsigned int i;
  unsigned int sum;
  EVOMovieReturnStatusType ret;


  if (imageNo>=movies->nFrames) {
    sprintf(EVOMovieErrorString, "frame %d out of range [0..%d]",
            imageNo, (movies->nFrames)-1);
    return MOVIEERR;
  }

  i=0;
  sum = 0;
  while (imageNo >= (sum+movies->movie[i]->nFrames)) {
    sum += movies->movie[i]->nFrames;
    i++;
  }
  ret = evo_movie_get_image(movies->movie[i], image, imageNo-sum, whichEye);

  return ret;

}


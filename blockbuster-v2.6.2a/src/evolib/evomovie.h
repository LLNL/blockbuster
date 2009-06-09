/********************************************************************
 * $Id: evomovie.h,v 1.1 2007/06/13 18:59:29 wealthychef Exp $
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


#ifndef EVOMOVIE_COMMON_H
#define EVOMOVIE_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */




#include <stdio.h>




typedef enum EVOStereoEye {
  LEFTEYE, RIGHTEYE, BOTHEYES
} EVOStereoEyeType;

typedef enum EVOMovieReturnStatus {
  MOVIEERR=-1, MOVIEOK=0
} EVOMovieReturnStatusType;

typedef enum EVOMovieFormat {
  MOVIERLE=0, MOVIEJPEG=1
} EVOMovieFormatType;




typedef struct {

  /* PUBLIC MEMBERS */
  
  /* true if a stereo */
  unsigned int stereo;
  /* pixels across */
  unsigned int xdim;
  /* pixels down */
  unsigned int ydim;
  /* number of frames in the movie */
  unsigned int nFrames;
  
  /* PRIVATE MEMBERS */
  
  /* pointer to movie file */
  FILE *movieFP;
  /* if true, then creating a movie */
  int newMovie;
  /* format for new movie */
  EVOMovieFormatType movieFormat;
  /* array of offsets into the movie file */
  unsigned int *offsets;
  /* array of lengths of the individual frames */
  unsigned int *lengths;
  /* location of the movie table index */
  unsigned int movieTableLocation;
  /* the frame we'll be adding next during writing */
  unsigned int currentFrame;
  /* jpeg stuff - can't have actual members here because */
  /* of type conflicts with MS Visual C++ */
  void *jpegStuff;

} EVOMovieType, *EVOMoviePtrType;




typedef struct {

  /* PUBLIC MEMBERS */

  /* true if a stereo */
  unsigned int stereo;
  /* pixels across */
  unsigned int xdim;
  /* pixels down */
  unsigned int ydim;
  /* number of total frames */
  unsigned int nFrames;

  /* PRIVATE MEMBERS */

  /* number of movie files */
  unsigned int nMovieFiles;
  /* list of movie files */
  EVOMoviePtrType *movie;

} EVOMovieListType, *EVOMovieListPtrType;




typedef struct {

  /* PUBLIC MEMBERS */

  /* pixels across */
  unsigned int xdim;
  /* pixels down */
  unsigned int ydim;
  /* pointer to the RGB pixel ordered data;
  dimensioned to be xdim*ydim*3 */
  unsigned char *pixels;

} EVOImageType, *EVOImagePtrType;




/* error string */
extern char EVOMovieErrorString[256];




/* SINGLE MOVIE FILE FUNCTIONS */

/* create a new EVO movie file; returns NULL on error */
EVOMoviePtrType evo_movie_new_file(char *fileName, unsigned int stereo,
                                   unsigned int xdim, unsigned int ydim,
                                   unsigned int nFrames,
                                   EVOMovieFormatType movieFormat,
                                   unsigned int quality);

/* open a EVO movie file; returns NULL on error */
EVOMoviePtrType evo_movie_open_file(char *fileName);

/* purge movie from memory */
EVOMovieReturnStatusType evo_movie_close_file(EVOMoviePtrType movie);




/* MULTIPLE MOVIE FILE FUNCTIONS */

/* open a list of movie files; returns NULL on error */
EVOMovieListPtrType evo_movie_open_files(int nFiles, char *fileNames[]);

/* add a new movie file to the animation */
EVOMovieReturnStatusType  evo_movie_add_file(EVOMovieListPtrType movies,
                                             char *fileName);

/* close a list of movie files */
EVOMovieReturnStatusType evo_movie_close_files(EVOMovieListPtrType movies);

/* return an image */
EVOMovieReturnStatusType evo_movies_get_image(EVOMovieListPtrType movies,
   EVOImagePtrType image, unsigned int imageNo, EVOStereoEyeType whichEye);




/* IMAGE FUNCTIONS */

/* allocate a new empty image */
EVOImagePtrType evo_movie_alloc_image(unsigned int xdim, unsigned int ydim);

/* free an image */
void evo_movie_free_image(EVOImagePtrType image);

/* add an image to the new movie; images must be added sequentially */
EVOMovieReturnStatusType evo_movie_add_image(EVOMoviePtrType movie,
   EVOImagePtrType image);

/* return an image */
EVOMovieReturnStatusType evo_movie_get_image(EVOMoviePtrType movie,
   EVOImagePtrType image, unsigned int imageNo, EVOStereoEyeType whichEye);

#ifdef __cplusplus
}
#endif

#endif /* EVOMOVIE_COMMON_H */


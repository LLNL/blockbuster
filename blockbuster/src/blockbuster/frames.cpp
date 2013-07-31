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

#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include "util.h"
#include <libgen.h>
#include "frames.h"
#include "pnmFrame.h"
#include "pngFrame.h"
#include "tiffFrame.h"
#include "sgi-rgbFrame.h"
#include "smFrame.h"
#include "errmsg.h"
#include "settings.h"
#include "convert.h"

static QString matchName; 

#ifdef _LARGEFILE64_SOURCE
#define MYSTAT stat64
#else
#define MYSTAT stat
#endif

/* Note:  __APPLE__ assumes gcc on OS X*/
#if defined(irix) || defined(aix) || defined(__APPLE__)
static int matchStartOfName(struct dirent *entry)
#else
  static int matchStartOfName(const struct dirent *entry)
#endif
{
  /* RDC -- this is all that is needed.  The old version of this function called stat on every matching file, for no good reason I could tell.  Unfortunately, on a 32 bit OS with large files, this caused weird behavior, so I just pulled it out.  We're left with a very easy check  - check the name to see if it matches */
  return (QString(entry->d_name) == matchName); 
}


//===============================================================
/* This utility function handles loading and converting images appropriately,
 * for both the single-threaded and multi-threaded cases.
 */
#define MAX_CONVERSION_WARNINGS 10
Image *FrameInfo::LoadAndConvertImage(unsigned int frameNumber,
                                      ImageFormat *canvasFormat, 
                                      const Rectangle *region, 
                                      int levelOfDetail)
{
  DEBUGMSG(QString("LoadAndConvertImage frame %1, region %2, frameInfo %3").arg( frameNumber).arg(region->toString()).arg((uint64_t)this)); 

  Image *image, *convertedImage;
  int rv;
  static int conversionCount = 0;

  /*  if (!enable) {
    cerr << "Interesting: disabled FrameInfo" << endl; 
    return NULL;
    }*/

  image = new Image(); 
  if (!image) {
	ERROR("Out of memory in LoadAndConvertImage");
	return NULL;
  }

  DEBUGMSG("LoadImage being called"); 
  /* Call the file format module to load the image */
  rv = (*LoadImageFunPtr)(image, this, 
                    canvasFormat,
                    region, levelOfDetail);
  image->frameNumber = frameNumber; 
  DEBUGMSG("LoadImage done"); 

  if (!rv) {
	ERROR("could not load frame %d (frame %d of file name %s) for the cache",
          frameNumber,
          mFrameNumberInFile,
          filename.c_str()
          );
	//enable = 0;
	delete image; 
	return NULL;
  }

  /* The file format module, which loaded the image for us, tried to
   * do as good a job as it could to match the format our canvas
   * requires, but it might not have succeeded.  Try to convert
   * this image to the correct format.  If the same image comes
   * back, no conversion was necessary; if a NULL image comes back,
   * conversion failed.  If any other image comes back, we can
   * discard the original, and use the conversion instead.
   */
  convertedImage = ConvertImageToFormat(image, canvasFormat);
  if (convertedImage != image) {
    /* We either converted, or failed to convert; in either
     * case, the old image is useless.
     */
    delete image; 
  
    image = convertedImage;
    if (image == NULL) {
      ERROR("failed to convert frame %d", frameNumber);
    }
    else {
      conversionCount++;
      if (conversionCount < MAX_CONVERSION_WARNINGS) {
        /* We'll issue a warning anyway, as conversion is an expensive
         * process.
         */
        WARNING("had to convert frame %d", frameNumber);
          
        if (conversionCount +1 == MAX_CONVERSION_WARNINGS) {
          WARNING("(suppressing further conversion warnings)");
        }
      }
    }
  }

  /* If we have a NULL image, the converter already reported it. */
  DEBUGMSG("Done with LoadAndConvertImage, frame %d", frameNumber); 
  return image;
}



//===============================================================
void FrameList::GetInfo(int &maxWidth, int &maxHeight, int &maxDepth,
                        int &maxLOD, float &fps){
  maxWidth = maxHeight = maxDepth = maxLOD = 0; 
  uint32_t i; 
  for (i = 0; i < frames.size(); i++) {
    FrameInfoPtr frameInfoPtr = frames[i]; 
    maxWidth = MAX2(maxWidth, frameInfoPtr->width);
    maxHeight = MAX2(maxHeight, frameInfoPtr->height);
    maxDepth = MAX2(maxDepth, frameInfoPtr->depth);
    maxLOD = MAX2(maxLOD, frameInfoPtr->maxLOD);
  }
  
  fps = targetFPS;
  return; 
}
//===============================================================

 
/*
 * Given a list of filenames, return a FrameList describing the movie
 * frames.
 * Input: fileCount - length of files array
 * Input: files - array of filenames
 * Input: settings - a place to store recent files
 * Global variable used: fileFormats - list of acceptable file formats
 * Return: true if files were loaded, else false
 */
bool FrameList::LoadFrames(QStringList &files) {
  DEBUGMSG("LoadFrameList()");
  int32_t index;
  
  char  *filename, directory[BLOCKBUSTER_PATH_MAX], base[BLOCKBUSTER_PATH_MAX]; 
  for (index = 0; index < files.size(); index++) {
    filename = strdup(MakeAbsolutePath(files[index]).toStdString().c_str()); 
    DEBUGMSG(QString("Checking for file %1").arg( filename)); 
    strncpy(base, basename(filename), strlen(filename)); 
    filename = strdup(MakeAbsolutePath(files[index]).toStdString().c_str()); 
    strncpy(directory, dirname(filename), strlen(filename));; 
    filename = strdup(MakeAbsolutePath(files[index]).toStdString().c_str()); 
    
    QString recentFileName;
    int numFiles;
    struct dirent **fileList;
    int i;
       
    /* Get a list of files that match the given filename; these represent
     * our movie files.  Each can be a movie of itself, or a still image.
     */
    matchName = base; // matchName is a static variable
    /* Special case for the sm network based files.  No directory,
     * just a name prefixed by "dfc:".
     */
    if (QString(base).startsWith("dfc:")) {
      numFiles = 1;
      fileList = new struct dirent *;
      fileList[0] = new struct dirent;
      strcpy(fileList[0]->d_name,base);
      directory[0] = '\0';
    } 
    else if ((numFiles = scandir(directory,
                                 &fileList, 
                                 matchStartOfName, alphasort)) < 0){
      ERROR(QString("Error scanning directory '%1': %2").arg(directory ).arg(strerror(errno)));
      return false;
    }
    else if (numFiles == 0) {
      ERROR(QString("no files match '%1'.  Skipping...").arg(filename));
      return false;
    }
    
    /* One or more files matched - we're happy as clams.  Now the
     * fun and exciting part: each file may include one or more
     * frames (movie files in particular can include multiple frames).
     */
    /*   recentFileName = QString("%1/%2").arg(directory).arg(base);
         SetRecentFileSetting((void*)settings, recentFileName);
    */ 
    for (i = 0; i < numFiles; i++) {
      QString matchedFileName = 
        QString("%1/%2").arg(directory).arg(fileList[i]->d_name);;
      DEBUGMSG(QString("Matched file name '%1'").arg( matchedFileName));
      
      FrameList *frameListFromFile = NULL;
           
      /* Search for a format driver that can read the matched file. */
      frameListFromFile = smGetFrameList(matchedFileName.toStdString().c_str()); 
      if (!frameListFromFile) {
        frameListFromFile = tiffGetFrameList(matchedFileName.toStdString().c_str()); 
      }
      if (!frameListFromFile) {
        frameListFromFile = pnmGetFrameList(matchedFileName.toStdString().c_str()); 
      }
      if (!frameListFromFile) {
        frameListFromFile = pngGetFrameList(matchedFileName.toStdString().c_str()); 
      }
      if (!frameListFromFile) {
        frameListFromFile = sgirgbGetFrameList(matchedFileName.toStdString().c_str()); 
      }
      /* See if any of the formats matched */
      if (frameListFromFile) {
        /* Spit out a little information about the matched
         * frames.  
         */
        DEBUGMSG(QString("Matched format '%1' with file '%2'").
                 arg(frameListFromFile->formatName).arg(matchedFileName));
        /* This operation will dump more information about the
         * matched frames, and will destroy frameListFromFile
         */
        //allFrames = AppendFrameList(allFrames, frameListFromFile);
        append(frameListFromFile); 
        if (i==0) stereo = frameListFromFile->stereo; 
        delete frameListFromFile; 
      }
      else {
        /* No format was able to open the file.  Give an error. */
        ERROR(QString("No known format could open the file '%1' - skipping")
              .arg(matchedFileName));
      }
            
    } /* loop scanning all file names that match a single argument */
    
    /* Done with this argument; free up the matching file list and
     * other associated variables.
     */
    for (i = 0; i < numFiles; i++) {
      delete fileList[i];
    }
    delete fileList; 
  }
  
  return frames.size()>0;
}



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
#include "util.h"
#include <libgen.h>
#include "frames.h"
#include "pnm.h"
#include "bbpng.h"
#include "tiff.h"
#include "sgi-rgb.h"
#include "sm.h"
#include "errmsg.h"
#include "settings.h"

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

/* Image data allocation and deallocation routines, and image
 * deallocation routines, that are used by multiple modules. 
 * In general, it's the responsibility of the image allocator
 * to install the correct deallocation function in the image.
 * (Some modules, like the splash screen or modules that 
 * allocate image data from graphics chip memory, may install
 * their own image deallocation routines.)
 */

void *DefaultImageDataAllocator(Canvas *, unsigned int size)
{
    return malloc(size);
}
void DefaultImageDataDeallocator(Canvas *, void *imageData)
{
    free(imageData);
}

void DefaultImageDeallocator(Canvas *canvas, Image *image)
{
    if (image->imageData != NULL && image->ImageDataDeallocator != NULL) {
      (*image->ImageDataDeallocator)(canvas, image->imageData);
    }
    free(image);
}
/* This routine is called to release all the memory associated with a frame. */
void DefaultDestroyFrameInfo(FrameInfo *frameInfo)
{
    if (frameInfo) {
	/* Release the filename allocated with strdup. */
	if (frameInfo->filename) {
	    free(frameInfo->filename);
	}
	free(frameInfo);
    }
}

//===============================================================

void FrameList::DeleteFrames(void) {
  register uint32_t i;
  
  for (i = 0; i < frames.size(); i++) {
    //DEBUGMSG("Deleting frame %d", i); 
    (frames[i]->DestroyFrameInfo)(frames[i]);
  }
  //  DEBUGMSG("Done with DeleteFrames"); 
}

//===============================================================
void FrameList::append(FrameList *other) {
  uint32_t i=0; 
  while (i < other->frames.size()) {
    frames.push_back(other->frames[i]); 
    ++i; 
  }
  return; 
}

//===============================================================
void FrameList::GetInfo(int &maxWidth, int &maxHeight, int &maxDepth,
			     int &maxLOD, float &fps){
  maxWidth = maxHeight = maxDepth = maxLOD = 0; 
  uint32_t i; 
  for (i = 0; i < frames.size(); i++) {
    FrameInfo *frameInfoPtr = frames[i]; 
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
    strncpy(directory, dirname(filename), strlen(filename));; 
    
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
      fileList = (struct dirent **)malloc(sizeof(struct direct *));
      fileList[0] = (struct dirent *)malloc(sizeof(struct dirent));
      strcpy(fileList[0]->d_name,base);
      directory[0] = '\0';
    } 
    else if ((numFiles = scandir(directory,
				 &fileList, 
				 matchStartOfName, alphasort)) < 0){
      SYSERROR(QString("cannot scan for '%1' - skipping").arg( filename));
      continue;
    }
    else if (numFiles == 0) {
      WARNING(QString("no files match '%1'.  Skipping...").arg(filename));
      continue;
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
    //free(directory);
    //free(base);
    for (i = 0; i < numFiles; i++) {
      free(fileList[i]);
    }
    free(fileList);
  }
  
  return frames.size()>0;
}



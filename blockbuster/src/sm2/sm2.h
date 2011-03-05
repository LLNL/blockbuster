/*
** $RCSfile: smBase.h,v $
** $Name:  $
**
** ASCI Visualization Project 
**
** Lawrence Livermore National Laboratory
** Information Management and Graphics Group
** P.O. Box 808, Mail Stop L-561
** Livermore, CA 94551-0808
**
** For information about this project see:
** 	http://www.llnl.gov/sccd/lc/img/
**
**      or contact: asciviz@llnl.gov
**
** For copyright and disclaimer information see:
**      $(ASCIVIS_ROOT)/copyright_notice_1.txt
**
** 	or man llnl_copyright
**
** $Id: smBase.h,v 1.13 2009/05/19 02:52:19 wealthychef Exp $
**
*/
/*
**
**  Abstract:
//
//! sm2.C - base class for buffered, threadsafe "streamed movies"
//
**
**  Author:  Richard Cook
**
*/


#ifndef _SM_2_H
#define _SM_2_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h> 
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>

#include <string>
#include <vector>

void sm2_setVerbose(int level);  // 0-5, 0 is quiet, 5 is verbose

// On top of the movie "type".  Room for 256 "types".
#define SM2_FLAGS_MASK	0xffffff00
#define SM2_TYPE_MASK	0x000000ff
#define SM2_FLAGS_STEREO	0x00000100

#define SM2_FLAGS_FPS_MASK	0xffc00000
#define SM2_FLAGS_FPS_SHIFT	22
#define SM2_FLAGS_FPS_BASE	20.0

#define SM2_MAGIC_VERSION1	0x0f1e2d3c
#define SM2_MAGIC_VERSION2	0x0f1e2d3d


class sm2Base {
 public:
  sm2Base(int numthreads=1):mNumThreads(numthreads) {return;}
  ~sm2Base() {return; }
  
  void SetMovie(vector<string> filenames); 
  // only the master thread calls this.  It spawns a thread to begin buffering
  void StartBuffering(uint64_t buffersize); 
  void StopBuffering(void);  // does not invalidate current buffer necessarily

  /* fetch a frame from the output buffer and decompress it.  Thread safe. 
     returns frame bytes or -1 if error occurs.  
  */
  uint32_t GetFrame(uint32_t frame, char *oBuffer); 
  
 private:
  void ReadFramesFromFile(uint32_t start, uint32_t end, string filename); 
  void DecompressFramesFromBuffer(uint32_t start, uint32_t end); 

  int numThreads; 
  DiskBuffer mDiskBuffers[2], *mCurrentDiskBuffer;
  IOBuffer mFrameBuffers[2], *mCurrentFrameBuffer; 
}; 



#endif

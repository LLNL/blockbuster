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
**
**  Author:
**
*/


#ifndef _SM_BASE_H
#define _SM_BASE_H

//
// smBase.h - base class for "streamed movies"
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h> 
#include <sys/types.h>
#include "../common/stringutil.h"
#include <sys/mman.h>
#include <unistd.h>

#include <string>
#include <vector>
#include <deque>

#ifndef off64_t
#define off64_t int64_t
#endif

// On top of the movie "type".  Room for 256 "types".
#define SM_FLAGS_MASK	0xffffff00 // everything but the type
#define SM_COMPRESSION_TYPE_MASK	0x000000ff
#define SM_FLAGS_STEREO	0x00000100

#define SM_FLAGS_FPS_MASK	0xffc00000 // 10 bits at the top
#define SM_FLAGS_FPS_SHIFT	22 // 32 bits minus 10 bit fps
#define SM_FLAGS_FPS_BASE	20.0

#define SM_MAGIC_VERSION1	0x0f1e2d3c
#define SM_MAGIC_VERSION2	0x0f1e2d3d
#define SM_HDR_SIZE 64

#define SM_VERBOSE 1
#ifdef SM_VERBOSE 
#include <stdarg.h>
#include "../common/timer.h"
#include "../common/stringutil.h"
void smSetVerbose(int level);

struct smMsgStruct {
  smMsgStruct(int l, string fi, string func) :
    line(l), file(fi), function(func) {
    return; 
  }
  int line; 
  string file, function; 
}; 
//static smMsgStruct gMsgStruct; 

#define SMPREAMBLE 
#define smdbprintf(args...)                                  \
  sm_real_dbprintf( smMsgStruct(__LINE__,__FILE__,__FUNCTION__), args)   
//  gMsgStruct.line = __LINE__, gMsgStruct.file=__FILE__, gMsgStruct.function=__FUNCTION__, sm_real_dbprintf


extern int smVerbose;
inline void sm_real_dbprintf(const smMsgStruct msg, int level, const char *fmt, ...) {  
  if (smVerbose < level) return; 
  cerr << " SMDEBUG [" << msg.file << ":"<< msg.function << "(), line "<< msg.line << ", time=" << GetExactSeconds() << "]: " ;
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr,fmt,ap);
  va_end(ap);
  //cerr << endl; 
  return; 
}

#else
inline void sm_real_dbprintf(int , const char * ...) {  
  return; 
}
#define smdbprintf if(0) sm_real_dbprintf
#endif
//===============================================

class StreamingMovie {
 public:
  StreamingMovie(string filename):mFileName(filename) {    
    return; 
  }
  ~StreamingMovie() {
    return; 
  }
  
  void SetFilename(string filename) {
    mFileName = filename; 
  }

  float GetFrameRate(void) {
    return 0; 
  }
  
  int GetVersion (void) {
    if (mRawMagic == SM_MAGIC_VERSION1) return 1; 
    if (mRawMagic == SM_MAGIC_VERSION2) return 2;
  } 

  bool ReadHeader(void);

  private:

  uint32_t mRawMagic, mRawFlags; 
   // version
  int mVersion;
 // number of frames in the movie
  uint32_t mNumFrames;
  // number of Levels of Detail
  uint32_t mNumResolutions;
  // compression type: 
  uint32_t mCompressionType; 
  
  // image size at each LOD, x then y
  uint32_t mFrameSizes[8][2]; // 8 is the limit on LOD apparently. 
  // dimensions of the tiles at each LOD, x then y
  uint32_t mTileSizes[8][2];
  uint32_t mTileNxNy[8][2];
  uint32_t mMaxNumTiles; // the maximum number of tiles at any resolution
  uint32_t mMaxTileSize; 

  // 64-bit offset of each compressed frame
  vector<off64_t>mFrameOffsets;
  // 32-bit length of each compressed frame
  vector<unsigned int>mFrameLengths;
    
  
  // global file descriptor
  //int *fd;
  off64_t filesize;
  
  //path to movie file
  string mFileName;
    
}; 

#endif

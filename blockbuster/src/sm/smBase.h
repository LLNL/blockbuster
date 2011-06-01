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
#ifndef WIN32
#include <sys/mman.h>
#include <unistd.h>
#else
#define off64_t __int64
#endif

#include <string>
#include <vector>
#include <deque>
//#define DISABLE_PTHREADS 1
#ifndef DISABLE_PTHREADS
#include <pthread.h>
#else
#error PTHREADS DISABLED
#define pthread_cond_t char
#define pthread_mutex_t char
#endif 

#ifndef off64_t
#define off64_t int64_t
#endif

//#define SM_VERBOSE
void sm_setVerbose(int level);  // 0-5, 0 is quiet, 5 is verbose

// On top of the movie "type".  Room for 256 "types".
#define SM_FLAGS_MASK	0xffffff00
#define SM_TYPE_MASK	0x000000ff
#define SM_FLAGS_STEREO	0x00000100

#define SM_FLAGS_FPS_MASK	0xffc00000
#define SM_FLAGS_FPS_SHIFT	22
#define SM_FLAGS_FPS_BASE	20.0

#define SM_MAGIC_VERSION1	0x0f1e2d3c
#define SM_MAGIC_VERSION2	0x0f1e2d3d

/*!
  ===============================================
  debug print statements, using smdbprintf, which 
  output the file, line, time of execution for each statement. 
  Incredibly useful
*/ 

#ifdef SM_VERBOSE 
#include <stdarg.h>
#include "../common/timer.h"
#include "../common/stringutil.h"
void sm_setVerbose(int level);

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


struct TileInfo {  
  std::string toString(void); 
  u_int tileNum;
  u_int overlaps;  /* newly overlaping data */
  u_int cached; /* overlap data from a prior read -- cached */
  u_int blockOffsetX; /* where to start writing row data in uber block */
  u_int blockOffsetY; /* likewise for column data */
  u_int tileOffsetX; /* offset into current tile to grab data */
  u_int tileOffsetY;
  u_int tileLengthX;  /* length of mem copy */
  u_int tileLengthY;
  uint32_t fileOffset; /* offset from start of frame, including header */ 
  u_int readBufferOffset; /* where to find this tile in buffer */
  u_int compressedSize; 
  u_int skipCorruptFlag;
} ;

struct smThreadData {
  smThreadData(): fd(0),currentFrame(-1) {}
  ~smThreadData() {}
  int fd; 
  uint32_t currentFrame ; // prevent duplicate reads of same frame
  std::vector<u_char> io_buf;  // for reading chunks from files
  std::vector<u_char> tile_buf; // for reading tiles; 
  std::vector<TileInfo> tile_infos; // used for computing overlap info
};
  
/*!
  A struct to hold info for compressing a frame to avoid recomputing stuff
*/ 
struct FrameCompressionWork {
  FrameCompressionWork(int inFrame, u_char *data)  {
    mFrame = inFrame; 
    mUncompressed = data; 
  }
  ~FrameCompressionWork() {
    clear(); 
  }
  
  void clear(void) {
    int  pos = mCompressed.size(); 
    smdbprintf(5, "deleting %d compressed image buffers for frame %d\n", 
               pos, mFrame);
    
    while (pos--) {
      if (mCompressed[pos]) {
        smdbprintf(5, "deleting compressed image %d buffer %p for frame %d\n", 
                   pos, mCompressed[pos], mFrame);
        delete mCompressed[pos]; 
        mCompressed[pos]= NULL;
      }
    }
    smdbprintf(5, "deleting uncompressed image buffer %p for frame %d\n", 
               mUncompressed, mFrame);
    if (mUncompressed) {
      delete mUncompressed; 
      mUncompressed = NULL; 
    }
    mCompressed.clear(); 
    mCompTileSizes.clear(); 
    mCompFrameSizes.clear(); 
  }
  string toString(void ) {
    string mystring = string("<< FrameCompressionWork: mFrame = ") + intToString(mFrame); 
    int numsizes=mCompFrameSizes.size(), sizenum = 0; 
    if (numsizes) {
      mystring += ", mCompFrameSizes = ("; 
      while (sizenum < numsizes-1) {
        mystring += intToString(mCompFrameSizes[sizenum]) + ","; 
        ++ sizenum;
      }
      mystring += intToString(mCompFrameSizes[sizenum]) + ")"; 
    } else {
      mystring += ", no compressed frames found";
    }
    if (mCompTileSizes.size()) {
      mystring += ", tiles per resolution: "; 
      uint32_t resnum = 0; 
      while (resnum < mCompTileSizes.size()) {
        int numtiles = mCompTileSizes[resnum].size();
        uint32_t totalTilesize = 0; 
        mystring += "<<resolution " + intToString(resnum) + ", " + intToString(numtiles) + "tiles"; 
        if (numtiles) { 
          mystring += ", tilesizes: ("; 
          int tilenum = 0; 
          while (tilenum < numtiles-1) {
            mystring += intToString(mCompTileSizes[resnum][tilenum]) + ",";
            totalTilesize += mCompTileSizes[resnum][tilenum];
           ++tilenum; 
          }
          totalTilesize += mCompTileSizes[resnum][tilenum];
          mystring += intToString(mCompTileSizes[resnum][tilenum]) + "), total =  " + intToString(totalTilesize) + ">>";
          if (resnum < mCompTileSizes.size()-1) 
            mystring += ", ";
        }      
        ++ resnum;          
      }
    } else {
      mystring += ", no tiles detected"; 
    }
    return mystring;
  }      
  int mFrame;
  vector< vector<int> > mCompTileSizes; //sizes of the compressed tiles, for each resolution
  vector<int> mCompFrameSizes; // total size of compressed frame at each resolution
  u_char *mUncompressed;
  vector<u_char *>mCompressed; // Buffers of compressed frames, one for each resolution level
}; 

/*!
  Helps organize buffering for I/O. 
*/ 
struct OutputBuffer {
  OutputBuffer(uint32_t buffSize): mRequiredWriteBufferSize(0),
                  mFirstFrameNum(0), mNumFrames(0) {
    mFrameBuffer.resize(buffSize); 
    return; 
  }
  bool full(void) {
    return (mNumFrames == mFrameBuffer.size()); 
  } 

  void resize(uint32_t frames) {
    smdbprintf(3, "OutputBuffer::resize() Resizing frame buffer to %ld frames\n", frames); 
    mFrameBuffer.resize(frames); 
  }

  bool empty(void) { 
    return (mNumFrames == 0); 
  }
  
  bool addFrame(FrameCompressionWork* frame){
    int32_t slotnum = frame->mFrame - mFirstFrameNum;
    if (slotnum + 1 > (int32_t)mFrameBuffer.size() ||  slotnum < 0) {      
      return false; 
    }
    if (mFrameBuffer[slotnum]) {
      smdbprintf(0, "Bad thing:  placing frame %d in an occupied slot!\n", frame->mFrame);
    }
    mRequiredWriteBufferSize += frame->mCompFrameSizes[0] + frame->mCompTileSizes[0].size()*sizeof(uint32_t); 
    mFrameBuffer[slotnum] = frame; 
    mNumFrames++; 
    return true; 
  }
  
  
  vector<FrameCompressionWork*> mFrameBuffer; // stored as work quanta until actually rewritten; then marshalled into mWriteBuffer for efficient I/O 
  int64_t mRequiredWriteBufferSize; // computed as frames are buffered
  int32_t mFirstFrameNum;  
  uint32_t mNumFrames;  // number of entries in mDataBuffer -- can't use size() because elements are placed out of order, and resize() is called only once.  
}; 

/*!
  SMbASE base class for streaming movies
*/ 
class smBase {
 public:
  smBase(const char *fname, int numthreads=1, uint32_t bufferSize=50);
  virtual ~smBase();
  
  /// set size of output buffers. 
  void setBufferSize(uint32_t frames);

  // get the decompressed image 
  uint32_t getFrame (int frame, void *out, int threadnum, int res=0); // for threads
  
  // get part of an image and decompress it appropriately
  uint32_t getFrameBlock(int frame, void *out, int threadnum, 
                     int destRowStride=0,
                     int *dim = NULL, 
                     int *pos = NULL, int *step = NULL,int res = 0);
  
  void getCompFrame(int frame, int threadnum, void *out, int &outsize,int res = 0);
  int getCompFrameSize(int frame,int res = 0);
  
  // set the frame image, either uncompressed or compressed data
  void compressAndWriteFrame(int frame, u_char *data);
  //void setCompFrame(int frame, void *data, int size,int res = 0);
  void writeCompFrame(int frame, void *data, int *sizes,int res = 0);

  // for multithreaded case, this allows buffering into a queue.  This will hang if a frame is ever skipped!  
  // void bufferFrame(int frame,unsigned char *data, bool oktowrite);
  void writeThread(void); 
  void startWriteThread(void); 
  void stopWriteThread(void); 

  void combineResolutionFiles(void);
  bool flushFrames(bool force); 
  void compressAndBufferFrame(int frame,u_char *data);
  //void flushFrames(void); 

  // Tile based version follows
  int computeTileSizes(FrameCompressionWork *wrk, int resolution);
  void compressFrame(FrameCompressionWork *wrk);

  // convert an image into its compressed form, tiled 
  int compFrame(void *in, void *out, int *outsizes,int res = 0);
  
#ifdef WIN32
  static void __cdecl init(void);
#else
  static void init(void);
#endif
  
  u_int getWidth(int res=0)  { return(framesizes[res][0]); }
  u_int getHeight(int res=0) { return(framesizes[res][1]); }
  u_int getNumFrames() { return(mNumFrames); }
  u_int getNumResolutions() { return(mNumResolutions); }
  u_int getTileWidth(int res=0) { return(tilesizes[res][0]); }
  u_int getTileHeight(int res=0) { return(tilesizes[res][1]); }
  u_int getTileNx(int res=0) { return(tileNxNy[res][0]); }
  u_int getTileNy(int res=0) { return(tileNxNy[res][1]); }
  uint32_t getNumTiles(int res) { return getTileNx(res)*getTileNx(res); }
  u_int getMaxNumTiles() { return(maxNumTiles);}
  
  int Min(int a,int b) { return((a > b) ? b : a); }
  
  void printFrameDetails(FILE *fp, int f, int res);
  
  // open a movie
#ifdef WIN32
  static smBase * __cdecl openFile(const char *fname, int numthreads=1);
#else
  static smBase *openFile(const char *fname, int numthreads=1);
#endif
  // close a movie
  void closeFile(void);
  
  // various flag/type info
  virtual int getType(void) {return(-1);};
  u_int getFlags(void) { return(flags); };
  void setFlags(u_int f) { flags = f; };
  float getFPS(void) { 
    int i = (flags & SM_FLAGS_FPS_MASK) >> SM_FLAGS_FPS_SHIFT;
    if (i == 0) return(30.0);
    return ((float)i)/SM_FLAGS_FPS_BASE;
  };
  void setFPS(float fps) {
    u_int i = (u_int)(fps * SM_FLAGS_FPS_BASE);
    i = (i << SM_FLAGS_FPS_SHIFT) & SM_FLAGS_FPS_MASK;
    flags = (flags & ~SM_FLAGS_FPS_MASK) | i;
  };
  int getVersion(void) { return(mVersion); };
  
  void computeTileOverlap(int *blockDim, int* blockPos, int res, int thread);
  
 protected:
  
  // internal functions to compress or decompress a rectangle of pixels
  // most subclasses need only replace these...
  virtual void compBlock(void *in, void *out, int &outsize, int *dim) = 0;
  virtual bool decompBlock(u_char *in,u_char *out,int insize, int *dim) = 0;
  
  // create a new movie for writing
  int newFile(const char *fname, u_int w, u_int h,
		      u_int nframes, u_int *tilesizes = NULL, u_int nres=1, int numthreads=1);
  
  static void registerType(u_int t, smBase *(*)(const char *, int));
  
  
 private:
  void readHeader(void);
  void initWin(void);
  uint32_t readData(int fd, u_char *buf, int bytes);
  uint32_t readFrame(u_int frame, int threadnum);
  uint32_t readTiledFrame(u_int frame, int*dimensions, int*position, int resolution, int threadnum);
  
  vector<u_char>mWriteBuffer; // for use in marshalling data and writing
  int mNumThreads; 
  // Flags on top of the filetype...
  u_int flags;
  
  // number of frames in the movie
  u_int mNumFrames;
  u_int mNumResolutions;
  
  // image size
  u_int framesizes[8][2]; // 8 is the limit on LOD apparently. 
  // dimensions of the tiles...
  u_int tilesizes[8][2];
  u_int tileNxNy[8][2];
  u_int maxNumTiles;
  
  // 64-bit offset of each compressed frame
  vector<off64_t>mFrameOffsets;
  // 32-bit length of each compressed frame
  vector<unsigned int>mFrameLengths;
  
  // "mod" flag
  int bModFile;
  
  // version
  int mVersion;
  
  // global file descriptor
  //int *fd;
  off64_t filesize;
  
  //path to movie file
  char *mMovieName;
  
  
  vector<int> mResFDs; // for mipmap files written in "parallel" 
  vector<string> mResFileNames; //  they have to be written to, then read
  vector<uint64_t> mResFileBytes; // size of the tmp files
 /*!
    Per-thread data structures, to ensure thread safety.  Every function must now specify a thread number. 
  */ 
  std::vector<smThreadData> mThreadData; 
  // staging buffer is for worker threads to place work to
  OutputBuffer *mOutputBuffers[2]; 
  OutputBuffer *mStagingBuffer, 
  // output buffer is for I/O thread to write to disk
    *mOutputBuffer; 
  pthread_mutex_t mBufferMutex; 

  pthread_t mWriteThread; 
  bool mWriteThreadRunning, mWriteThreadStopSignal; 


  // directory of movie types
  static u_int ntypes;
  static u_int *typeID;
  static smBase *(**smcreate)(const char *, int);
  
};

#endif

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
#ifndef WIN32
#include <sys/mman.h>
#include <unistd.h>
#else
#define off64_t __int64
#endif

#include <vector>

//#define DISABLE_PTHREADS 1
#ifndef DISABLE_PTHREADS
#include <pthread.h>
#else
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

typedef struct {
  u_int frame;
  u_int overlaps;  /* newly overlaping data */
  u_int prev_overlaps; /* overlap data from a prior read -- cached */
  u_int blockOffsetX; /* where to start writing row data in uber block */
  u_int blockOffsetY; /* likewise for column data */
  u_int tileOffsetX; /* offset into current tile to grab data */
  u_int tileOffsetY;
  u_int tileLengthX;  /* length of mem copy */
  u_int tileLengthY;
  u_int readBufferOffset; /* where to find this tile in buffer */
  u_int compressedSize; 
  u_int skipCorruptFlag;
} tileOverlapInfo_t;

// rethinking the mutex scheme -- we do not need mutexes
struct smThreadData {
  std::vector<u_char> tile_buf; // for reading tiles; 
  std::vector<u_char> io_buf;  // for reading chunks from files
  std::vector <uint32_t> tile_offsets; 
}; 
  
class smBase {
   public:
      smBase(const char *fname);
      virtual ~smBase();

      // get the decompressed image or return the raw compressed data
      virtual void getFrame(int frame, void *out);

      virtual void getFrameBlock(int frame, void *out, int destRowStride=0,
                                 int *dim = NULL, 
		      int *pos = NULL, int *step = NULL,int res = 0);

      virtual void getCompFrame(int frame, void *out, int &outsize,int res = 0);
      virtual int getCompFrameSize(int frame,int res = 0);

      // set the frame image, either uncompressed or compressed data
      virtual void setFrame(int frame,void *data);
      virtual void setCompFrame(int frame,void *data,int size,int res = 0);
      virtual void setCompFrame(int frame,void *data,int *sizes,int res = 0);

      // convert an image into its compressed form
      virtual void compFrame(void *in, void *out, int &outsize,int res = 0);
      // Tile based version follows
      virtual void compFrame(void *in, void *out, int *outsizes,int res = 0);

#ifdef WIN32
      static void __cdecl init(int nwin = 1);
#else
      static void init(int nwin = 1);
#endif

      u_int getWidth(int res=0)  { return(framesizes[res][0]); }
      u_int getHeight(int res=0) { return(framesizes[res][1]); }
      u_int getNumFrames() { return(nframes); }
      u_int getNumResolutions() { return(nresolutions); }
      u_int getTileWidth(int res=0) { return(tilesizes[res][0]); }
      u_int getTileHeight(int res=0) { return(tilesizes[res][1]); }
      u_int getTileNx(int res=0) { return(tileNxNy[res][0]); }
      u_int getTileNy(int res=0) { return(tileNxNy[res][1]); }
      u_int getMaxNumTiles() { return(maxNumTiles);}

      int Min(int a,int b) { return((a > b) ? b : a); }
     
      void printFrameDetails(FILE *fp, int f);

      // open a movie
#ifdef WIN32
      static smBase * __cdecl openFile(const char *fname, int _nwin=1);
#else
      static smBase *openFile(const char *fname, int _nwin=1);
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
      int getVersion(void) { return(version); };

      void computeTileOverlap(int *blockDim, int* blockPos, int res, tileOverlapInfo_t *info);

   protected:

      // internal functions to compress or decompress a rectangle of pixels
      // most subclasses need only replace these...
      virtual void compBlock(void *in, void *out, int &outsize, int *dim) = 0;
      virtual void decompBlock(u_char *in,u_char *out,int insize, int *dim) = 0;

      // create a new movie
      int newFile(const char *fname, u_int nframes, u_int w, u_int h,
		      u_int *tilesizes = NULL, u_int nres=1);

      void seekToFrame(int, u_int);

      static void registerType(u_int t, smBase *(*)(const char *, int));

      /*! 
        locking a frame means reading it into memory and keeping it there
      */
      void *lockFrame(u_int frame, u_int &size);
      void *lockFrame(u_int frame, u_int &size,int*dim, int*pos, int res);
      void unlockFrame(u_int frame);

     

   private:
      void readHeader(void);
      void initWin(void);
      void readWin(u_int frame);
      void readWin(u_int frame, int*dimensions, int*position, int resolution);

      // Flags on top of the filetype...
      u_int flags;

      // number of frames in the movie
      u_int nframes;
      u_int nresolutions;

      // image size
      u_int framesizes[8][2]; // 8 is the limit on LOD apparently. 
      // size of the tiles...
      u_int tilesizes[8][2];
      u_int tileNxNy[8][2];
      u_int maxNumTiles;
      
      // 64-bit offset of each compressed frame
      off64_t *foffset;
      // 32-bit length of each compressed frame
      unsigned int *flength;

      // "mod" flag
      int bModFile;

      // version
      int version;

      // global file descriptor
      int *fd;
      off64_t filesize;

      //path to movie file
      char *fname;

      // number of frames the buffer of compressed frames can store
      static u_int nwin;

      /*!
        Per-thread data structures, to ensure thread safety.  Every function must now specify a thread number. 
      */ 
      std::vector<smThreadData> mThreadData; 

       
      // "windowing" mutex set
      pthread_mutex_t *winmut;
      pthread_cond_t *wincond;
      int *currentFrame; // WTF?  
      u_int *winlock;
      void **win;
      
      // writelock mutex
      pthread_mutex_t writelock;
      
      // directory of movie types
      static u_int ntypes;
      static u_int *typeID;
      static smBase *(**smcreate)(const char *, int);

      // direct-io -- oh, the irony-- not used at all.  ever. 
        void **dio_buf;
        void **dio_free;
      int dio_mem;
      int dio_min;
      int dio_max;

      // tile bufs --- point into dio_buf -- i.e. dio_buf has extra allocation for worst case tile size
      u_char **tile_buf;
      u_int **tile_sizes;
      u_int **tile_offsets;
      tileOverlapInfo_t **tile_info; // used for computing overlap info
};

#endif

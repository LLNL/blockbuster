/*
** $RCSfile: smBase.C,v $
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
** $Id: smBase.C,v 1.19 2009/05/19 02:52:19 wealthychef Exp $
**
*/
/*
**
**  Abstract:
**
**  Author:
**
*/


//
//! smBase.C - base class for "streamed movies"
//

#define DMALLOC 1
#undef DMALLOC

#include <assert.h>
#include <stdio.h>
#include <iostream>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef WIN32
#else
#include <netinet/in.h>
#include <unistd.h>
#endif

#include <errno.h>
#include <math.h>

#include "smBaseP.h"
#include "smRaw.h"
#include "smRLE.h"
#include "smGZ.h"
#include "smLZO.h"
#include "smJPG.h"

#ifdef DMALLOC
#include <dmalloc.h>
#else
#include <stdlib.h>
#endif
#include "stringutil.h"

using namespace std; 

 
const int SM_MAGIC_1=SM_MAGIC_VERSION1;
const int SM_MAGIC_2=SM_MAGIC_VERSION2;
const int DIO_DEFAULT_SIZE = 1024L*1024L*4;

#define SM_HDR_SIZE 64

string TileInfo::toString(void) {
  // return string("{{ TileInfo: frame ") + intToString(frame)
  //  + ", tile = "+intToString(tileNum)
  return string("{{ TileInfo:  tile = ")+intToString(tileNum)
    + ", overlaps="+intToString(overlaps)
    + ", cached="+intToString(cached)
    + ", blockOffsetX="+intToString(blockOffsetX)
    + ", blockOffsetY="+intToString(blockOffsetY)
    + ", tileOffsetX = "+intToString(tileOffsetX)
    + ", tileOffsetY = "+intToString(tileOffsetY)
    + ", tileLengthX = "+intToString(tileLengthX)
    + ", tileLengthY = "+ intToString(tileLengthY)
    + ", readBufferOffset = "+ intToString(readBufferOffset)
    + ", compressedSize = "+intToString(compressedSize) 
    + ", skipCorruptFlag = "+intToString(skipCorruptFlag)
    + "}}"; 
} 
/*!
  ===============================================
  debug print statements, using smdbprintf, which 
  output the file, line, time of execution for each statement. 
  Incredibly useful
*/ 
#define SM_VERBOSE 1
//#undef  SM_VERBOSE
static int smVerbose = 0; 
void sm_setVerbose(int level) {
  std::cerr << "sm_setVerbose level " << level << std::endl; 
  smVerbose = level; 
}

#ifdef SM_VERBOSE 
#include <stdarg.h>
#include "../common/timer.h"
#include "../common/stringutil.h"

struct smMsgStruct {
  int line; 
  string file, function; 
}; 
static smMsgStruct gMsgStruct; 

#define SMPREAMBLE 
#define smdbprintf  \
  gMsgStruct.line = __LINE__, gMsgStruct.file=__FILE__, gMsgStruct.function=__FUNCTION__, sm_real_dbprintf  
    

inline void sm_real_dbprintf(int level, const char *fmt, ...) {  
  if (smVerbose < level) return; 
  cerr << GetHostname() << " SMDEBUG: ";
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr,fmt,ap);
  va_end(ap);
  cerr << " [" << gMsgStruct.file << ":"<< gMsgStruct.function << "(), line "<< gMsgStruct.line << ", time=" << GetExactSeconds() << "]" << endl; 
  return; 
}
#else
inline void sm_real_dbprintf(int level, const char *fmt, ...) {  
  return; 
}
#define smdbprintf if(0) sm_real_dbprintf
#endif
//===============================================

#define CHECK(v) \
if(v == NULL) \
sm_fail_check(__FILE__,__LINE__)

void sm_fail_check(const char *file,int line)
{
  perror("fail_check");
  smdbprintf(0,"Failed at line %d in file %s\n",line,file);
  exit(1); 
}



u_int smBase::ntypes = 0;
u_int *smBase::typeID = NULL;
smBase *(**smBase::smcreate)(const char *, int) = NULL;

static void  byteswap(void *buffer,off64_t len,int swapsize);
static void Sample2d(unsigned char *in,int idx,int idy,
		     unsigned char *out,int odx,int ody,
		     int s_left,int s_top,int s_dx,int s_dy,int filter);
//!  The init() routine must be called before using the SM library
/*!
  init() just gets the subclasses ready, which registers them with the 
  smcreate() factory function used in openFile()
*/ 
void smBase::init(void)
{
  static int initialized = 0; 
  if (initialized) return; 
  initialized = 1;

  smJPG::init();
  smLZO::init();
  smGZ::init();
  smRLE::init();
  smRaw::init();

 
  return; 
}

//----------------------------------------------------------------------------
//! smBase - constructor initializes the synchronization primitives
/*!
  \param _fname filename of the sm file to read/write
  \param numthreads number of independent buffers to create for thread safety
*/
//
//----------------------------------------------------------------------------
smBase::smBase(const char *_fname, int numthreads)
{
  smdbprintf(5, "smBase::smBase(%s, %d)", _fname, numthreads);
  int i;
   setFlags(0);
   setFPS(getFPS());
   nframes = 0;
   foffset = NULL;
   flength = NULL;
   version = 0;
   nresolutions = 1;
   
   mThreadData.clear(); 
   mThreadData.resize(numthreads); 
   
   if (_fname != NULL) {
      fname = strdup(_fname);
      int threadnum = mThreadData.size();
      while (threadnum--) {
        mThreadData[threadnum].fd = OPEN(fname, O_RDONLY);
      }
      readHeader();
      initWin(); // only does something for version 1.0
   }
   else
      fname = NULL;

   bModFile = FALSE;

   return;
}

//----------------------------------------------------------------------------
//! ~smBase - destructor
/*!
  closes the file and frees the name...
*/ 
//
//----------------------------------------------------------------------------
smBase::~smBase()
{
   int i;

   smdbprintf(5,"smBase destructor : %s",fname);
   if ((bModFile == TRUE) && (mThreadData[0].fd)) CLOSE(mThreadData[0].fd);
   if (fname) free(fname);
}

//----------------------------------------------------------------------------
//! openFile - open an SM file of unknown compression type
/*!
  Looks at the magic cookie in the file, then calls the smcreate() 
  factory routine to generate the correct class.  
  \param _fname filename of the sm file to read/write
  \param numthreads number of independent buffers to create for thread safety
*/
//
//----------------------------------------------------------------------------
smBase *smBase::openFile(const char *_fname, int numthreads)
{
   smBase *base;
   u_int magic;
   u_int type;
   int fd;
   int i;

   smdbprintf(5, "smBase::openFile(%s, %d)", _fname, numthreads);

   if ((fd = OPEN(_fname, O_RDONLY)) == -1)
      return(NULL);

 
   READ(fd, &magic, sizeof(u_int));
   magic = ntohl(magic);
   READ(fd, &type, sizeof(u_int));
   type = ntohl(type) & SM_TYPE_MASK;

   if ((SM_MAGIC_2 != magic)  && (SM_MAGIC_1 != magic)) {
      CLOSE(fd);
      return(NULL);
   }

   for (i=0; i<ntypes; i++) {
      if (typeID[i] == type) break;
   }

   if (i == ntypes) {
      CLOSE(fd);
      return(NULL);
   }

   base = (*(smcreate[i]))(_fname, numthreads);

   CLOSE(fd);

   return(base);
}

//! convenience function
/*!
  \param fp FILE * to the file containing the frame
  \param f frame number
*/ 
void smBase::printFrameDetails(FILE *fp,int f)
{
   if ((f < 0) || (f >= nframes*nresolutions)) {
      fprintf(fp,"%d\tInvalid frame\n",f);
      return;
   }
#ifdef WIN32
   fprintf(fp,"%d\t%I64d\t%d\n",f,foffset[f],flength[f]);
#else
   fprintf(fp,"%d\t%lld\t%d\n",f,foffset[f],flength[f]);
#endif
   return;
}

//! Create a new SM File for writing
/*!
  \param _fname name of the file
  \param _width Width of the movie frames
  \param _height Height of the movie frames
  \param _nframes number of movie frames
  \param _tsizes size of tiles for each resolution, laid out as xyxyxy
  \param _nres  number of resolutions (levels of detail)
*/
int smBase::newFile(const char *_fname, u_int _width, u_int _height, 
                    u_int _nframes, u_int *_tsizes, u_int _nres)
{
   int i;
   nframes = _nframes;
   framesizes[0][0] = _width;
   framesizes[0][1] = _height;
   for(i=1;i<8;i++) {
       framesizes[i][0] = framesizes[i-1][0]/2;
       framesizes[i][1] = framesizes[i-1][1]/2;
   }
   memcpy(tilesizes,framesizes,sizeof(framesizes));
   nresolutions = _nres;
   if (nresolutions < 1) nresolutions = 1;
   if (nresolutions > 8) nresolutions = 8;

   version = 2;
   
  
   for(i=0;i<nresolutions;i++) {
     if (_tsizes) {
       tilesizes[i][0] = _tsizes[(i*2)+0];
       tilesizes[i][1] = _tsizes[(i*2)+1];
       tileNxNy[i][0] = (u_int)ceil((double)framesizes[i][0]/(double)tilesizes[i][0]);
       tileNxNy[i][1] = (u_int)ceil((double)framesizes[i][1]/(double)tilesizes[i][1]);
       assert(tileNxNy[i][0] < 1000);
       assert(tileNxNy[i][1] < 1000);
     }
     else {
       tileNxNy[i][0] = (u_int) 1;
       tileNxNy[i][1] = (u_int) 1;
     }
     smdbprintf(5,"newFile tile %dX%d of [%d,%d]",
                tileNxNy[i][0],tileNxNy[i][1], tilesizes[i][0], 
                tilesizes[i][1]);
   }
   
  
   smdbprintf(5,"init: w %d h %d frames %d", framesizes[0][0], framesizes[0][1], 
		nframes);

   fname = strdup(_fname);

   mThreadData[0].fd = OPENC(_fname, O_WRONLY|O_CREAT|O_TRUNC, 0666);
   foffset = (off64_t *)calloc(sizeof(off64_t),nframes*nresolutions);
   CHECK(foffset);
   foffset[0] = SM_HDR_SIZE*sizeof(u_int) +
	   nframes*nresolutions*(sizeof(off64_t)+sizeof(u_int));
   flength = (u_int *)calloc(sizeof(u_int),nframes*nresolutions);
   CHECK(flength);
   if (LSEEK64(mThreadData[0].fd, foffset[0], SEEK_SET) < 0)
     smdbprintf(0, "Error seeking to frame 0\n");

   bModFile = TRUE;

   return(0);
}
//! register a subclass as a possible compression method for SM
/*!
  \param id a unique ID for the type which can be used to identify the file type. 
  \param create  A factory function that returns the subclass 
  \bug The create function smells funny to me, but it works. 
*/ 
void smBase::registerType(u_int id, smBase *(*create)(const char *, int))
{
   if (ntypes == 0) {
      typeID = (u_int *)malloc(sizeof(u_int) * 32);
      CHECK(typeID);
      smcreate = (smBase *(**)(const char *, int))malloc(
                    sizeof(smBase *(*)()) * 32);
      CHECK(smcreate);
   }
   else if ((ntypes % 32) == 0) {
      typeID = (u_int *)realloc(typeID, sizeof(u_int) * (ntypes+32));
      CHECK(typeID);
      smcreate = (smBase *(**)(const char *, int))realloc(
                   smcreate, sizeof(smBase *(*)()) * (ntypes+32));
      CHECK(smcreate);
   }

   typeID[ntypes] = id;
   smcreate[ntypes] = create;

   ntypes++;
}

//! Get the juicy goodness of the header and store the info internally
/*!
  Information collected includes frame sizes, tile sizes, file size, 
  used extensively in reading and decompressing frames later. 
*/   
void smBase::readHeader(void)
{
   int lfd;
   u_int magic, type;
   u_int maxtilesize;
   u_int maxFrameSize;

   int i, w;

   // the file size is needed for the size of the last frame
   filesize = LSEEK64(mThreadData[0].fd,0,SEEK_END);

   lfd = OPEN(fname, O_RDONLY);
   READ(lfd, &magic, sizeof(u_int));
   magic = ntohl(magic);
   if (magic == SM_MAGIC_1) version = 1;
   if (magic == SM_MAGIC_2) version = 2;
   
   READ(lfd, &type, sizeof(u_int));
   setFlags(ntohl(type) & SM_FLAGS_MASK);
   type = ntohl(type) & SM_TYPE_MASK;

   READ(lfd, &nframes, sizeof(u_int));
   nframes = ntohl(nframes);
   smdbprintf(5,"open file, nframes = %d", nframes);

   READ(lfd, &i, sizeof(u_int));
   framesizes[0][0] = ntohl(i);
   READ(lfd, &i, sizeof(u_int));
   framesizes[0][1] = ntohl(i);
   for(i=1;i<8;i++) {
     framesizes[i][0] = framesizes[i-1][0]/2;
     framesizes[i][1] = framesizes[i-1][1]/2;
   }
   memcpy(tilesizes,framesizes,sizeof(framesizes));
   nresolutions = 1;

   smdbprintf(5,"image size: %d %d", framesizes[0][0], framesizes[0][1]);
   smdbprintf(5,"nframes=%d",nframes);

   // Version 2 header is bigger...
   if (version == 2) {
     maxtilesize = 0;
     maxNumTiles = 0;
     u_int arr[SM_HDR_SIZE];
     LSEEK64(lfd,0,SEEK_SET);
     READ(lfd, arr, sizeof(u_int)*SM_HDR_SIZE);
     byteswap(arr,sizeof(u_int)*SM_HDR_SIZE,sizeof(u_int));
     nresolutions = arr[5];
     //smdbprintf(5,"nresolutions : %d",nresolutions);
     for(i=0;i<nresolutions;i++) {
       tilesizes[i][1] = (arr[6+i] & 0xffff0000) >> 16;
       tilesizes[i][0] = (arr[6+i] & 0x0000ffff);
       tileNxNy[i][0] = (u_int)ceil((double)framesizes[i][0]/(double)tilesizes[i][0]);
       tileNxNy[i][1] = (u_int)ceil((double)framesizes[i][1]/(double)tilesizes[i][1]);
       
       if (maxtilesize < (tilesizes[i][1] * tilesizes[i][0])) {
	 maxtilesize = tilesizes[i][1] * tilesizes[i][0];
       }
       if(maxNumTiles < (tileNxNy[i][0] * tileNxNy[i][1])) {
	 maxNumTiles = tileNxNy[i][0] * tileNxNy[i][1];
       }
       //smdbprintf(5,"tileNxNy[%ld,%ld] : maxnumtiles %ld", tileNxNy[i][0], tileNxNy[i][1],maxNumTiles);
     }
     //smdbprintf(5,"maxtilesize = %ld, maxnumtiles = %ld",maxtilesize,maxNumTiles);
     
   }
   else {
     // initialize tile info to reasonable defaults
     for (i = 0; i < nresolutions; i++) {
       tilesizes[i][0] = framesizes[i][0];
       tilesizes[i][1] = framesizes[i][1];
       tileNxNy[i][0] = 1;
       tileNxNy[i][1] = 1;
     }
   }
   
   // Get the framestart offsets
   foffset = (off64_t *)malloc(sizeof(off64_t)*(nframes+1)*nresolutions);
   CHECK(foffset);
   READ(lfd, foffset, sizeof(off64_t)*nframes*nresolutions);
   byteswap(foffset,sizeof(off64_t)*nframes*nresolutions,sizeof(off64_t));
   foffset[nframes*nresolutions] = filesize;

   // Get the compressed frame lengths...
   flength = (u_int *)malloc(sizeof(u_int)*(nframes)*nresolutions);
   CHECK(flength);
   maxFrameSize = 0; //maximum frame size
   if (version == 2) {
     READ(lfd, flength, sizeof(u_int)*nframes*nresolutions);
     byteswap(flength,sizeof(u_int)*nframes*nresolutions,sizeof(u_int));
     for (w=0; w<nframes*nresolutions; w++) {
       if (flength[w] > maxFrameSize) maxFrameSize = flength[w];
     }
   } else {
     // Compute this for older format files...
     for (w=0; w<nframes*nresolutions; w++) {
	   flength[w] = (foffset[w+1] - foffset[w]);
       if (flength[w] > maxFrameSize) maxFrameSize = flength[w];
     }
   }
#if 0 && SM_VERBOSE
   for (w=0; w<nframes; w++) {
     smdbprintf(5,"window %d: %d size %d", w, (int)foffset[w],flength[w]);
   }
#endif
   smdbprintf(5,"maximum frame size is %d", maxFrameSize);
   
   // bump up the size to the next multiple of the DIO requirements
   maxFrameSize += 2;
   CLOSE(lfd);
   for (i=0; i<mThreadData.size(); i++) {
     unsigned long w;
     if(version == 2) {
       // put preallocated tilebufs and tile info support here as well
       mThreadData[i].io_buf.clear(); 
       mThreadData[i].io_buf.resize(maxFrameSize);
       mThreadData[i].tile_buf.clear();
       mThreadData[i].tile_buf.resize(maxtilesize*3);
       mThreadData[i].tile_offsets.clear(); 
       mThreadData[i].tile_offsets.resize(maxNumTiles);
       mThreadData[i].tile_infos.clear(); 
       mThreadData[i].tile_infos.resize(maxNumTiles);
     }
   }
   return; 
}

/*!
  Frankly, this function is a mystery to me.  
  It's used for SM version 1 files.
*/
void smBase::initWin(void)
{
  u_int i;
  
  if(getVersion() == 1) {
     readWin(0,0);
  }
}



/*!
  Loop until all read
*/ 
uint32_t smBase::readData(int fd, u_char *buf, int bytes) {
  ///smdbprintf(5, "readData: %d bytes", bytes); 
  int remaining = bytes; 
  while  (remaining >0) {
    int r=READ(fd, buf, remaining);
    if (r < 0) {
      if ((errno != EINTR) && (errno != EAGAIN)) {
        smdbprintf(0, "smBase::readData() error: read=%d remaining=%d: %s",r, remaining, strerror(errno)); 
        /* char	s[80];
        sprintf(s,"xmovie I/O error : r=%d k=%d: ",r, remaining);
        perror(s);*/
        return -1;
      }
    } else {
      buf+=r;
      remaining-=r;
    }
    //smdbprintf(5,"read %d, %d remaining",r, remaining);
  }
  return bytes-remaining; 
}

/*!
  Reads a frame.  Does not decompress the data.  Poorly named.
  \param f The frame to read
  \param threadnum A zero-based thread id.  Threadnum must be less than numthreads.  No other thread should be using this threadnum, as there is one buffer per thread and collisions can happen. 
*/
uint32_t smBase::readWin(u_int f, int threadnum)
{
  off64_t size;
  
  size = flength[f];
  int fd = mThreadData[threadnum].fd;
  
  smdbprintf(5,"READWIN frame %d, thread %d, %ld bytes at offset %ld", f, threadnum, size, foffset[f]);
  
  if (LSEEK64(fd, foffset[f], SEEK_SET) < 0){
    smdbprintf(0, "Error seeking to frame %d", f);
  }

  
  u_char *buf = &mThreadData[threadnum].io_buf[0]; 
  mThreadData[threadnum].currentFrame = f; 
  
  int r = readData(fd, buf, size); 
  if (r == -1) {
    cerr << "Error in readWin for frame " << f << endl; 
  }
  smdbprintf(5, "Done with readWin"); 
  return r; 
}

//!  Reads and decompresses a frame.  Poorly named.
/*!
  Tiles are read in order but can be stored out of order in the read buffer. 
  \param f The frame to read
  \param dim XY dimensions of the region of interest
  \param pos XY position of the region of interest
  \param res resolution desired (level of detail)
  \param threadnum A zero-based thread id.  Threadnum must be less than numthreads.  No other thread should be using this threadnum, as there is one buffer per thread and collisions can happen. 
*/
uint32_t smBase::readWin(u_int f, int *dim, int* pos, int res, int threadnum)
{
  // version 2 implied -- reads in only overlapping tiles 
  
  uint32_t bytesRead = 0; 
  u_int readBufferOffset;
  smdbprintf(5,"readWin version 2, frame %d, thread %d", f, threadnum); 
  
  off64_t offset = foffset[f] ;
  int fd = mThreadData[threadnum].fd; 

  if (LSEEK64(fd, offset, SEEK_SET) < 0) {
    smdbprintf(0, "Error seeking to frame %d, offset %d: %s", f, offset, strerror(errno));
    exit(1);
  }
  assert(res < nresolutions);
  int nx = getTileNx(res);
  int ny = getTileNy(res);
  int numTiles =  nx * ny;
  
  // If numTiles > 1 then read in frame header of tile sizes
  uint previousFrame = mThreadData[threadnum].currentFrame; 
  
  if(numTiles > 1 ) {
    u_char *ioBuf = &(mThreadData[threadnum].io_buf[0]);
    int k =  numTiles * sizeof(uint32_t);
    int r=readData(fd, ioBuf, k);
    
    if (r !=  k) { // hmm -- assume we read it all, I guess... iffy?  
      char	s[40];
      sprintf(s,"smBase::readWin I/O error : r=%d k=%d ",r,k);
      perror(s);
      exit(1); // !!! shit!  whatevs. 
    }

    //aliasing to reduce pointer dereferences 
    uint32_t *header = (uint32_t*)&(mThreadData[threadnum].io_buf[0]);
    uint32_t *toffsets = &(mThreadData[threadnum].tile_offsets[0]); 
    TileInfo *firstTile = &(mThreadData[threadnum].tile_infos[0]), *tileInfo;
    readBufferOffset = k;
    // get tile sizes and offsets
    u_int sum = 0;
    int i=0;

    // skip over previous overlaps to find where the next tile should be read into if any new are found
    for(i = 0, tileInfo = firstTile; i < numTiles;  i++, tileInfo++) { 
      tileInfo->compressedSize = (u_int)ntohl(header[i]);
      toffsets[i] = k + sum;
      sum += tileInfo->compressedSize;
      //smdbprintf(5,"tile[%d].frame = %d",i,tileInfo->frame);
      if (previousFrame == f) {
        if ( tileInfo->cached) {
          // This tile has been read, so we can add its size to the number of bytes we know is in the read buffer and mark it as cached
          readBufferOffset += tileInfo->compressedSize;
          //smdbprintf(5,"tile[%d] setting cached : readBufferOffset = %d",i,readBufferOffset);
        }
      }
      else {
        tileInfo->cached = 0;
        tileInfo->overlaps = 0;
      }      
    }
    //smdbprintf(5, "thread %d calling computeTileOverlap"); 
    computeTileOverlap(dim, pos, res, threadnum);
    
    // Grab data for overlapping tiles
    int tile; 
    for(tile = 0, tileInfo = &(mThreadData[threadnum].tile_infos[0]); 
        tile < numTiles; 
        tile++, tileInfo++) {
      if(tileInfo->overlaps && ! tileInfo->cached) {
        //	smdbprintf(5,"tile %d newly overlaps",tile);
        // store the tile offset in the read buffer for later decompression
        // tiles are read out of order -- this allows incremental caching
        tileInfo->readBufferOffset = readBufferOffset;
        tileInfo->skipCorruptFlag = 0;
        
        if (LSEEK64(fd, offset + toffsets[tile] , SEEK_SET) < 0) {
          smdbprintf(0, "Error seeking to frame %d at offset %Ld\n", f, offset + toffsets[tile]);
	      exit(1);
	    }
        //smdbprintf(5,"Reading %d bytes from file for winnum %d for tile %d", tileInfo->compressedSize, winnum, tile); 
        r = readData(fd, ioBuf+readBufferOffset, tileInfo->compressedSize);
        
        if (r != tileInfo->compressedSize ) {
	      smdbprintf(0,"smBase::readWin I/O error thread %d, frame %d: r=%d k=%d, offset=%Ld : skipping",threadnum,f, r,tileInfo->compressedSize, offset+ toffsets[tile]);
	      tileInfo->skipCorruptFlag = 1;
	    }
        else {
          tileInfo->cached = 1;
        }
        bytesRead += r; 
        readBufferOffset += tileInfo->compressedSize;
      }
    } // end loop over tiles
  }
  smdbprintf(5,"Done with readWin v2 for frame %d, thread %d, read %d bytes", f, threadnum, bytesRead); 
  mThreadData[threadnum].currentFrame = f; 
  return bytesRead; 
}
//!  Tile overlap tells us which tiles in the frame overlap our region of interest (ROI).
/*!
  \param blockDim dimensions of the frame data
  \param blockPos where in the block data is our ROI
  \param res the resolution (level of Detail)
  \param info the output information
*/ 
void smBase::computeTileOverlap(int *blockDim, int* blockPos, int res, int threadnum)
{
  TileInfo *info = &(mThreadData[threadnum].tile_infos[0]);

  int nx = getTileNx(res);
  int ny = getTileNy(res);
  int tileWidth = getTileWidth(res);
  int tileHeight = getTileHeight(res);
  int ipIndex;
  int t1,t2;
  int tileStartX, tileEndX, tileStartY, tileEndY;
  int blockStartX, blockEndX, blockStartY, blockEndY;

  blockStartX = blockPos[0];
  blockEndX = blockStartX + blockDim[0] - 1; 
  blockStartY = blockPos[1];
  blockEndY = blockStartY + blockDim[1] - 1;

  /*
    smdbprintf(5, "computeTileOverlap from <%d, %d> to <%d, %d> at res %d", 
    blockStartX, blockStartY, blockEndX, blockEndY, res); 
  */

  ipIndex = 0; 
  for(int j = 0 ; j < ny ; j++) {
    for(int i = 0; i < nx; i++) {
      int overlaps_x = 0;
      int overlaps_y = 0;
     
      ipIndex = (j * nx) + i;
      tileStartX = i * tileWidth;
      tileEndX = tileStartX + tileWidth - 1;
      tileStartY = j * tileHeight;
      tileEndY = tileStartY + tileHeight - 1;
      info[ipIndex].blockOffsetX = 0;
      info[ipIndex].blockOffsetY = 0;
      info[ipIndex].tileOffsetX = 0;
      info[ipIndex].tileOffsetY = 0;
      info[ipIndex].tileLengthX = 0;
      info[ipIndex].tileLengthY = 0;
      info[ipIndex].tileNum = ipIndex; 
      
      t1 = blockStartX - tileStartX;
      if( t1 >= 0) {
        if (t1 < tileWidth) {
          //smdbprintf(5,"have overlap, t1=%d ",t1);
          overlaps_x = 1;
          info[ipIndex].blockOffsetX = 0;
          info[ipIndex].tileOffsetX = t1;
          if(blockEndX < tileEndX) {
            info[ipIndex].tileLengthX = blockDim[0];
          }
          else {
            info[ipIndex].tileLengthX = tileWidth - t1;
          }
        }
      }
      else { /* tileStartX could be inside block */
        if((blockEndX - tileStartX) >= 0) {
          //smdbprintf(5," inside block X ");
          overlaps_x = 1;
          info[ipIndex].blockOffsetX = tileStartX - blockStartX;
          info[ipIndex].tileOffsetX = 0;
          info[ipIndex].tileLengthX = Min(tileWidth,((blockEndX-tileStartX)+1));
        }
      }
      
      
      
      if(overlaps_x) {
        t2 = blockStartY - tileStartY;
        if( t2 >= 0) {
          if (t2 < tileHeight) {
            //smdbprintf(5," t2=%d ",t2);
            overlaps_y = 1;
            info[ipIndex].blockOffsetY = 0;
            info[ipIndex].tileOffsetY = t2;
            if(blockEndY < tileEndY) {
              info[ipIndex].tileLengthY = blockDim[1];
            }
            else {
              info[ipIndex].tileLengthY = tileHeight - t2;
            }
          }
        }
        else { /* tileStartY could be inside block */
          if((blockEndY - tileStartY) >= 0) {
            //smdbprintf(5," inside block Y ");
            overlaps_y = 1;
            info[ipIndex].blockOffsetY = tileStartY - blockStartY;
            info[ipIndex].tileOffsetY = 0;
            info[ipIndex].tileLengthY = Min(tileHeight,((blockEndY-tileStartY)+1));
          }
        }
      }
      
      if(overlaps_x && overlaps_y) {
        info[ipIndex].overlaps = 1;
      }
      else {
        info[ipIndex].overlaps = 0;
      }
      //smdbprintf(5, "thread %d, info[%d]: %s", threadnum,  ipIndex, info[ipIndex].toString().c_str()); 
      
    } // for i
  } // for j
  
  return;  
  
}

//-----------------------------------------------------------
// Core frame functions
//-----------------------------------------------------------

//! Convenient wrapper for getFrameBlock(), when you want the whole Frame.
/*!
  \param f frame number
  \param data an appropriately allocated pointer to receive the data, based on the size and resolution of the region of interest.
  \param threadnum A zero-based thread id.  Threadnum must be less than numthreads.  No other thread should be using this threadnum, as there is one buffer per thread and collisions can happen. 
*/ 
uint32_t smBase::getFrame(int f, void *data, int threadnum, int res)
{
  return  getFrameBlock(f,data, threadnum, 0, NULL, NULL, NULL, res);
}

//! Read a region of interest from the given frame of the movie. 
/*!
  This is where many CPU cycles will be spent when you are CPU bound.  Lots of pointer copies.  
  This function will "autores" the returned block based on "step" 
   \param f frame number
  \param data an appropriately allocated pointer to receive the data, based on the size and resolution of the region of interest.
  \param threadnum A zero-based thread id.  Threadnum must be less than numthreads.  No other thread should be using this threadnum, as there is one buffer per thread and collisions can happen. 
  \param destRowStride The X dimension of the output buffer ("data"), to properly locate output stripes 
  \param dim XY dimensions of the region of interest
  \param pos XY position of the region of interest
  \param res resolution desired (level of detail)
*/
 
uint32_t smBase::getFrameBlock(int f, void *data, int threadnum,  int destRowStride, int *dim, int *pos, int *step, int res)
{
  smdbprintf(5,"smBase::getFrameBlock, frame %d, thread %d, data %p", f, threadnum, data); 
   u_char *ioBuf;
   uint32_t size;
   u_char *image;
   u_char *out = (u_char *)data;
   u_char *rowPtr = (u_char *)data;
   uint32_t bytesRead=0; 
   int d[2],_dim[2],_step[2],_pos[2],tilesize[2];
   int _res,_f;
   
   if((res < 0) || (res > (nresolutions - 1))) {
     _res = 0;
   }
   else { 
     _res = res;
   }

   _f = f;

   if (step) {
       _step[0] = step[0]; _step[1] = step[1];
   } else {
       _step[0] = 1; _step[1] = 1;
   }
   if (dim) {
       _dim[0] = dim[0]; _dim[1] = dim[1];
   } else {
       _dim[0] = getWidth(_res); _dim[1] = getHeight(_res);
   }
   if (pos) {
       _pos[0] = pos[0]; _pos[1] = pos[1];
   } else {
       _pos[0] = 0; _pos[1] = 0;
   }
   
   if(_dim[0] <= 0 || _dim[1] <= 0)
     return 0;
   
   assert(_dim[0] + _pos[0] <= getWidth(0));
   assert(_dim[1] + _pos[1] <= getHeight(0));
   
   /* move _f into initial resolution */
   if(_res > 0)
     _f += getNumFrames() * _res;
   
   /* pick a resolution based on stepping */
   while((_res+1 < getNumResolutions()) && (_step[0] > 1) && (step[1] > 1)) {
     _res++;
     _step[0] >>= 1;
     _step[1] >>= 1;
     _pos[0] >>= 1;
     _pos[1] >>= 1;
     _dim[0] >>= 1;
     _dim[1] >>=1;
     _f += getNumFrames();
   }
   
   d[0] = getWidth(_res);
   d[1] = getHeight(_res);
   
   if (!destRowStride)
     destRowStride = _dim[0] * 3;
   
   tilesize[0] = getTileWidth(_res);
   tilesize[1] = getTileHeight(_res);
   
   if (d[0] < tilesize[0]) d[0] = tilesize[0];
   if (d[1] < tilesize[1]) d[1] = tilesize[1];
   
   
   // tile support 
   int nx = 0;
   int ny = 0;
   int numTiles = 0;
   u_char *tbuf;
    
   nx = getTileNx(_res);
   ny = getTileNy(_res);
   numTiles =  nx * ny;

   assert(numTiles < 1000);

   ioBuf = (u_char *)NULL;

   if((version == 2) && (numTiles > 1)) {
     bytesRead = readWin(_f,&_dim[0],&_pos[0],(int)_res, threadnum);
   }
   else {
     bytesRead = readWin(_f, threadnum);
   }
   ioBuf = &(mThreadData[threadnum].io_buf[0]); 
   size = flength[_f];
   tbuf = (u_char *)&(mThreadData[threadnum].tile_buf[0]);
   TileInfo *tileInfoList = (TileInfo *)&(mThreadData[threadnum].tile_infos[0]);

   
   if(numTiles < 2) {
     smdbprintf(5,"Calling decompBlock on frame %d since numTiles < 2", _f);
     if ((_pos[0] == 0) && (_pos[1] == 0) && 
         (_step[0] == 1) && (_step[1] == 1) &&
         (_dim[0] == d[0]) && (_dim[1] == d[1])) {
       smdbprintf(5,"getFrameBlock: decompBlock on entire frame"); 
       decompBlock(ioBuf,out,size,d); 
       
     } else {
       smdbprintf(5, "downsampled or partial frame %d", _f); 
       image = (u_char *)malloc(3*d[0]*d[1]);
       CHECK(image);
       decompBlock(ioBuf,image,size,d);
       for(int y=_pos[1];y<_pos[1]+_dim[1];y+=_step[1]) {
         u_char *dest = rowPtr;
         const u_char *p = image + 3*d[0]*y + _pos[0]*3;
         for(int x=0;x<_dim[0];x+=_step[0]) {
           *dest++ = p[0];
           *dest++ = p[1];
           *dest++ = p[2];
           p += 3*_step[0];
         }
         rowPtr += destRowStride;
       }
       free(image);
     }
     smdbprintf(5, "Done with single tiled image"); 
   } /* END single tiled simage */ 
   else { 
     smdbprintf(5,"smBase::getFrameBlock(frame %d, thread %d): process across %d overlapping tiles", f, threadnum, numTiles); 
     uint32_t copied=0;
     for(int tile=0; tile<numTiles; tile++){
       //smdbprintf(5,"tile %d", tile); 
       TileInfo tileInfo = tileInfoList[tile];
       //smdbprintf(5, "thread %d, Frame %d, tile %s", threadnum, f, tileInfo.toString().c_str());        
       if(tileInfo.overlaps && (tileInfo.skipCorruptFlag == 0)) {
         
         u_char *tdata = (u_char *)(ioBuf + tileInfo.readBufferOffset);
         smdbprintf(5,"decompBlock, tile %d", tile); 
         decompBlock(tdata,tbuf,tileInfo.compressedSize,tilesize);
         // 
         u_char *to = (u_char*)(out + (tileInfo.blockOffsetY * destRowStride) + (tileInfo.blockOffsetX * 3));
         u_char *from = (u_char*)(tbuf + (tileInfo.tileOffsetY * tilesize[0] * 3) + (tileInfo.tileOffsetX * 3));
         int maxX = tileInfo.tileLengthX, maxY = tileInfo.tileLengthY; 
         uint32_t newBytes = 3*maxX*maxY, maxAllowed = (_dim[0]*_dim[1]*3);
         uint32_t newTotal = newBytes + copied; 
         /*!
           smdbprintf(5, "thread %d, Frame %d, tile %d, copying %d rows %d pixels per row, %d new bytes, %d copied so far, new total will be %d, max allowed is %d x %d x 3 = %d", 
           threadnum, f, tile, maxY, maxX, newBytes, 
           copied, newTotal,  
           _dim[0], _dim[1], maxAllowed ); 
         */ 
         if (newTotal > maxAllowed) {
           smdbprintf(5, "Houston, we have a problem. Dump core here. new total > maxAllowed"); 
           abort(); 
         }
         for(int rows = 0; rows < maxY; rows += _step[1]) {
           if(_step[0] == 1) {
             copied += maxX * 3;
#if 1
             assert(copied <= maxAllowed);
#endif
             //smdbprintf(5,"frame %d tile %d",f,tile);
             memcpy(to,from,maxX * 3);
             
           }
           else {
             u_char *tx = to;
             u_char *p = from;
             for(int x=0; x<maxX; x+=_step[0]) {
               *tx++ = p[0];
               *tx++ = p[1];
               *tx++ = p[2];
               p += 3*_step[0];
             }
           }
           to += destRowStride;
           from += tilesize[0] * _step[1] * 3;
         }
       } 
       
     }
   } /* end process across overlapping tiles */
   
   smdbprintf(5,"END smBase::getFrameBlock, frame %d, thread %d, bytes read = %d", f, threadnum, bytesRead); 
   return bytesRead; 
 }

//! Another poorly named function . Should be called "writeFrame", I think.
/*!
  Compress the data and calls helpers to write it out to disk. 
  \param f the frame number
  \param data frame data to write.
*/ 
void smBase::setFrame(int f, void *data)
{
   int i,tile,size;
   int *sizes,numTiles,version;
   u_char *scaled0,*scaled1,*tmpBuf;

   version = getVersion();
   numTiles = getTileNx(0)*getTileNy(0);


   // Set the zeroth resolution
   if((version == 1) || (numTiles == 1)) {
     compFrame(data,NULL,size,0);
     //smdbprintf(5,"Frame %d is size %d",f,size);
     tmpBuf=(u_char *)malloc(size);
     CHECK(tmpBuf);
     compFrame(data,tmpBuf,size,0);
     setCompFrame(f,tmpBuf,size,0);
     free(tmpBuf);
   }
   else {
     // we handle tiles
     
     sizes = (int*)malloc(numTiles*sizeof(int)); 
     CHECK(sizes);
     compFrame(data,NULL,sizes,0);
     // compute sum 
     size = 0;
     for(tile=0; tile < numTiles;tile++) {
       size += sizes[tile];
     }
     tmpBuf=(u_char *)malloc(size); 
     CHECK(tmpBuf);
     compFrame(data,tmpBuf,sizes,0);
     setCompFrame(f,tmpBuf,sizes,0);
     free(sizes);
     free(tmpBuf);  
   }

   // quick out
   if (getNumResolutions() == 1) return;

   // Now the mipmaps...
   scaled0 = (u_char *)malloc(getWidth(0)*getHeight(0)*3);
   CHECK(scaled0);
   memcpy(scaled0,data,getWidth(0)*getHeight(0)*3);
   for(i=1;i<getNumResolutions();i++) {
       scaled1 = (u_char *)malloc(getWidth(i)*getHeight(i)*3);
       CHECK(scaled1);

       Sample2d(scaled0,getWidth(i-1),getHeight(i-1),
		scaled1,getWidth(i),getHeight(i),
		0,0,getWidth(i-1),getHeight(i-1),1);

       // write the frame level
       if(version == 1) {
	 compFrame(scaled1,NULL,size,i);
	 tmpBuf=(u_char *)malloc(size);
	 CHECK(tmpBuf);
	 compFrame(scaled1,tmpBuf,size,i);
	 setCompFrame(f,tmpBuf,size,i);
	 free(tmpBuf);
       }
       else {
	 // we handle tiles
	 numTiles = getTileNx(i)*getTileNy(i);
	 sizes = (int*)malloc(numTiles*sizeof(int));
	 CHECK(sizes);
	 compFrame(scaled1,NULL,sizes,i);
	 // compute sum 
	 size = 0;
	 for(tile=0; tile< numTiles;tile++) {
	   size += sizes[tile];
	 }
	 tmpBuf=(u_char *)malloc(size); 
	 CHECK(tmpBuf);
	 compFrame(scaled1,tmpBuf,sizes,i);
	 setCompFrame(f,tmpBuf,sizes,i);
	 free(tmpBuf);
	 free(sizes);
       }

       free(scaled0);
       scaled0 = scaled1;
   }
   free(scaled0);
#ifdef DMALLOC
   dmalloc_log_stats();
#endif
   return;
}

//! Should probably be called "writeCompFrame"
/*!
  Writes an already-compressed set of data to the file. 
  \param f the frame number
  \param data the compressed data
  \param size size of the frame in the data buffer to write
  \param res the resolution of level of detail (determines location in file)
*/
void smBase::setCompFrame(int f, void *data, int size, int res)
{
   foffset[f+res*getNumFrames()] = LSEEK64(mThreadData[0].fd,0,SEEK_CUR);
   flength[f+res*getNumFrames()] = size;
   WRITE(mThreadData[0].fd, data, size);

   bModFile = TRUE;
   return;
}


//! Should probably be called "writeCompFrameTiles"
/*!
  Writes an already-compressed set of frame tiles to the file. The difference is that here the sizes are tile sizes, not frame size.  
  \param f the frame number
  \param data the compressed data
  \param sizes sizes of the tiles in the buffer to write
  \param res the resolution of level of detail (determines location in file)
*/
void smBase::setCompFrame(int f, void *data, int *sizes, int res)
{
  uint32_t tz;
  int size;
  int numTiles = getTileNx(res)*getTileNy(res);

  foffset[f+res*getNumFrames()] = LSEEK64(mThreadData[0].fd,0,SEEK_CUR);
 
  size = 0;
  // if frame is tiled then write tile offset (jump table) first
  if(numTiles > 1) {
    for(int i = 0; i < numTiles; i++) {
      tz =  htonl((uint32_t)sizes[i]);
      WRITE(mThreadData[0].fd,&tz,sizeof(uint32_t));
      //smdbprintf(5,"size tile[%d] = %d\n",i,sizes[i]);
      size += sizes[i];
    } 
    flength[f+res*getNumFrames()] = size + numTiles*sizeof(uint32_t);
   
  }
  else {
     size += sizes[0];
     flength[f+res*getNumFrames()] = size;
  }
  //smdbprintf(5,"write %d bytes\n",size);
  WRITE(mThreadData[0].fd, data, size);

  bModFile = TRUE;
  return;
}


//! return the compressed frame (if data==NULL return the size)
/*!
  \param f frame number
  \param threadnum A zero-based thread id.  Threadnum must be less than numthreads.  No other thread should be using this threadnum, as there is one buffer per thread and collisions can happen. 
  \param data an appropriately allocated pointer to receive the data, based on the size and resolution of the region of interest.
  \param rsize output parameter that tells you how much data was read
  \param res resolution desired (level of detail)
*/
void smBase::getCompFrame(int frame, int threadnum, void *data, int &rsize, int res) 
{
   u_int size;
   void *ioBuf;

   readWin(frame+res*getNumFrames(), threadnum);
   ioBuf = &(mThreadData[threadnum].io_buf[0]); 
   size = flength[frame];
   
   if (data) memcpy(data, ioBuf, size);
   rsize = (int)size;

   return;
}
//! Convenience function for external programs
/*!
  \param frame frame number
  \param res resolution (level of Detail)
*/
int smBase::getCompFrameSize(int frame, int res)
{
	return(flength[frame+res*getNumFrames()]);
}

//! Wrapper around compBlock, compresses a frame as a single block
/*!
  \param in uncompressed data
  \param out compressed result
  \param outsize size of compressed data
  \param res resolution (level of detail)
*/ 
void smBase::compFrame(void *in, void *out, int &outsize, int res)
{
   int dim[2];
   dim[0] = getWidth(res);
   dim[1] = getHeight(res);
   int size;

   // right now, frames are handled as a single block...
   if (!out) {
 	// How big is the frame
	compBlock(in,NULL,size,dim);
   } else {
        // build the compressed frame...
   	compBlock(in,out,size,dim);
   }

   outsize = size;
   return;
}


//! Wrapper around compBlock, compresses a frame as a set of tiles
/*!
  \param in uncompressed data
  \param out compressed result
  \param outsizes sizes of compressed tiles in outbuffer
  \param res resolution (level of detail)
*/ 
void smBase::compFrame(void *in, void *out, int *outsizes, int res)
{
   int dim[2];
   int tilesize[2];
   int size;
   int msize;
   char *base;
   
   dim[0] = getWidth(res);
   dim[1] = getHeight(res);

   tilesize[0] = getTileWidth(res);
   tilesize[1] = getTileHeight(res);

   
   //smdbprintf(5,"dim[%d,%d] , tilesize[%d,%d]\n",dim[0],dim[1],tilesize[0],tilesize[1]);
   
   char *tilebuf = (char *)malloc(tilesize[0] * tilesize[1] * 3);
   CHECK(tilebuf);

   int nx = getTileNx(res);
   int ny = getTileNy(res);

   char *outp = (char*)out;

    for(int j=0;j<ny;j++) {
     for(int i=0;i<nx;i++) {
       if(out == NULL) {
	 //smdbprintf(5,"compFrame tile index[%d,%d]\n",i,j);
       }
       base = (char*)in + (((j * tilesize[1] * dim[0]) + (i * tilesize[0]))*3) ;
       if(((i+1) * tilesize[0]) > dim[0]) {
	 msize = (dim[0] - (i*tilesize[0])) * 3;
	 memset(tilebuf,0,tilesize[0] * tilesize[1] * 3);
       }
       else {
	 msize = tilesize[0]*3;
	 if(((j+1) * tilesize[1]) > dim[1]) {
	   memset(tilebuf,0,tilesize[0] * tilesize[1] * 3);
	 }
       }
       for(int k=0;k<tilesize[1];k++) {
	 if(((j * tilesize[1])+k) == dim[1]) 
	   break;
	
	 memcpy(tilebuf+(k*tilesize[0]*3),base+(k*dim[0]*3),msize);
       }

      
       if (!out) {
	 // How big is the tile
	 compBlock(tilebuf,NULL,size,tilesize);
       } else {
	 // build the compressed frame...
	 compBlock(tilebuf,outp,size,tilesize);
	 outp += size;
       }
       
       *(outsizes + (j*nx) + i) = size;
     }
   }
#if SM_DUMP  
    if(out) {
      char *p = (char *)out;
      for(int dd = 0; dd < dim[0]*3*dim[1];dd++)
	 smdbprintf(5," %d ",p[dd]);
    }
#endif 
    free(tilebuf);
    return;
}


void smBase::closeFile(void)
{
   u_int arr[64] = {0};
   smdbprintf(3, "smBase::closeFile"); 

   if (bModFile == TRUE) {
	int i;

   	LSEEK64(mThreadData[0].fd, 0, SEEK_SET);

   	arr[0] = SM_MAGIC_2;
   	arr[1] = getType() | getFlags();
 	arr[2] = nframes;
  	arr[3] = framesizes[0][0];
   	arr[4] = framesizes[0][1];
	arr[5] = nresolutions;
	//smdbprintf(5,"nresolutions = %d\n",nresolutions);
	for(i=0;i<nresolutions;i++) {
		arr[i+6] = (tilesizes[i][1] << 16) | tilesizes[i][0];
	}
   	byteswap(arr,sizeof(u_int)*SM_HDR_SIZE,sizeof(u_int));
   	WRITE(mThreadData[0].fd, arr, sizeof(u_int)*SM_HDR_SIZE);

   	byteswap(foffset,sizeof(off64_t)*nframes*nresolutions,sizeof(off64_t));
   	WRITE(mThreadData[0].fd, foffset, sizeof(off64_t)*nframes*nresolutions);
   	byteswap(foffset,sizeof(off64_t)*nframes*nresolutions,sizeof(off64_t));

   	byteswap(flength,sizeof(u_int)*nframes*nresolutions,sizeof(u_int));
   	WRITE(mThreadData[0].fd, flength, sizeof(u_int)*nframes*nresolutions);
   	byteswap(flength,sizeof(u_int)*nframes*nresolutions,sizeof(u_int));

	//smdbprintf(5,"seek header end is %d\n",LSEEK64(mThreadData[0].fd, 0, SEEK_CUR));
	CLOSE(mThreadData[0].fd);
   }
	
}
//! convenience function
/*!
  \param buffer data to swap
  \param len length of buffer
  \param swapsize size of each word in the buffer
*/
static void  byteswap(void *buffer,off64_t len,int swapsize)
{
        off64_t num;
        char    *p = (char *)buffer;
        char    t;

// big endian check...
	short   sh[] = {1};
	char    *by;

	by = (char *)sh;
	if (by[0] == 0) return;

        switch(swapsize) {
                case 2:
                        num = len/swapsize;
                        while(num--) {
                                t = p[0]; p[0] = p[1]; p[1] = t;
                                p += swapsize;
                        }
                        break;
                case 4:
                        num = len/swapsize;
                        while(num--) {
                                t = p[0]; p[0] = p[3]; p[3] = t;
                                t = p[1]; p[1] = p[2]; p[2] = t;
                                p += swapsize;
                        }
                        break;
                case 8:
                        num = len/swapsize;
                        while(num--) {
                                t = p[0]; p[0] = p[7]; p[7] = t;
                                t = p[1]; p[1] = p[6]; p[6] = t;
                                t = p[2]; p[2] = p[5]; p[5] = t;
                                t = p[3]; p[3] = p[4]; p[4] = t;
                                p += swapsize;
                        }
                        break;
                default:
                        break;
        }
        return;
}

//! Convolvement (see Google if you don't know what that is)
/*!
  smooth along X scanlines using 242 kernel 
  \param image the data to smooth 
  \param dx  fineness in X
  \param dy finness in Y
*/
static void smoothx(unsigned char *image, int dx, int dy)
{
        register int x,y;
	int	p1[3],p2[3],p3[3];

        for(y=0;y<dy;y++) {
                p1[0] = image[(y*dx)*3+0];
                p1[1] = image[(y*dx)*3+1];
                p1[2] = image[(y*dx)*3+2];

                p2[0] = image[((y*dx)+1)*3+0];
                p2[1] = image[((y*dx)+1)*3+1];
                p2[2] = image[((y*dx)+1)*3+2];

                for(x=1;x<dx-1;x++) {
                        p3[0] = image[((y*dx)+x+1)*3+0];
                        p3[1] = image[((y*dx)+x+1)*3+1];
                        p3[2] = image[((y*dx)+x+1)*3+2];

                        image[((y*dx)+x)*3+0]=((p1[0]*2)+(p2[0]*4)+(p3[0]*2))/8;
                        image[((y*dx)+x)*3+1]=((p1[1]*2)+(p2[1]*4)+(p3[1]*2))/8;
                        image[((y*dx)+x)*3+2]=((p1[2]*2)+(p2[2]*4)+(p3[2]*2))/8;

                        p1[0] = p2[0];
                        p1[1] = p2[1];
                        p1[2] = p2[2];

                        p2[0] = p3[0];
                        p2[1] = p3[1];
                        p2[2] = p3[2];
                }
        }
        return;
}

//! Convolvement (see Google if you don't know what that is)
/*!
  smooth along Y scanlines using 242 kernel 
  \param image the data to smooth 
  \param dx  fineness in X
  \param dy finness in Y
*/
static void smoothy(unsigned char *image, int dx, int dy)
{
        register int x,y;
	int	p1[3],p2[3],p3[3];

/* smooth along Y scanlines using 242 kernel */
        for(x=0;x<dx;x++) {
                p1[0] = image[x*3+0];
                p1[1] = image[x*3+1];
                p1[2] = image[x*3+2];

                p2[0] = image[(x+dx)*3+0];
                p2[1] = image[(x+dx)*3+1];
                p2[2] = image[(x+dx)*3+2];

                for(y=1;y<dy-1;y++) {
                        p3[0] = image[((y*dx)+x+dx)*3+0];
                        p3[1] = image[((y*dx)+x+dx)*3+1];
                        p3[2] = image[((y*dx)+x+dx)*3+2];

                        image[((y*dx)+x)*3+0]=((p1[0]*2)+(p2[0]*4)+(p3[0]*2))/8;
                        image[((y*dx)+x)*3+1]=((p1[1]*2)+(p2[1]*4)+(p3[1]*2))/8;
                        image[((y*dx)+x)*3+2]=((p1[2]*2)+(p2[2]*4)+(p3[2]*2))/8;

                        p1[0] = p2[0];
                        p1[1] = p2[1];
                        p1[2] = p2[2];

                        p2[0] = p3[0];
                        p2[1] = p3[1];
                        p2[2] = p3[2];
                }
        }
        return;
}

//! Subsample the data
/*!
  \param in input buffer
  \param idx subsampling in X
  \param idy subsampling in Y
  \param out output 
  \param odx X subsampling to output ?
  \param ody Y subsampling to output ?
  \param s_left location to start
  \param s_top location to start
  \param s_dx ?
  \param s_dy ?
  \param filter ? 
*/
static void Sample2d(unsigned char *in,int idx,int idy,
        unsigned char *out,int odx,int ody,
        int s_left,int s_top,int s_dx,int s_dy,int filter)
{
        register double xinc,yinc,xp,yp;
        register int x,y;
        register int i,j;
        register int ptr;

        xinc = (double)s_dx / (double)odx;
        yinc = (double)s_dy / (double)ody;

/* prefilter if decimating */
        if (filter) {
                if (xinc > 1.0) smoothx(in,idx,idy);
                if (yinc > 1.0) smoothy(in,idx,idy);
        }
/* resample */
        ptr = 0;
        yp = s_top;
        for(y=0; y < ody; y++) {  /* over all scan lines in output image */
                j = (int)yp;
                xp = s_left;
                for(x=0; x < odx; x++) {  /* over all pixel in each scanline of
output */
                        i = (int)xp;
			i = (i+(j*idx))*3;
                        out[ptr++] = in[i++];
                        out[ptr++] = in[i++];
                        out[ptr++] = in[i++];
                        xp += xinc;
                }
                yp += yinc;
        }
/* postfilter if magnifing */
        if (filter) {
                if (xinc < 1.0) smoothx(out,odx,ody);
                if (yinc < 1.0) smoothy(out,odx,ody);
        }
        return;
}  

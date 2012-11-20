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


#ifdef DMALLOC
#include <dmalloc.h>
#else
#include <stdlib.h>
#endif
#include "stringutil.h"
#include <boost/make_shared.hpp>
#include <boost/shared_array.hpp>

using namespace std; 

 #define SM_VERBOSE 1
#include "smBaseP.h"
#include "smRaw.h"
#include "smRLE.h"
#include "smGZ.h"
#include "smXZ.h"
#include "smLZO.h"
#include "smJPG.h"
#include <stringutil.h>

//#undef  SM_VERBOSE
int smVerbose = 0; 
double gBaseTime = -1; 

void sm_setVerbose(int level) {
  if (level) std::cerr << "sm_setVerbose level " << level << std::endl; 
  smVerbose = level; 
}
#define smdebug cerr

const int SM_MAGIC_1=SM_MAGIC_VERSION1;
const int SM_MAGIC_2=SM_MAGIC_VERSION2;
const int SM_MAGIC_3=SM_MAGIC_VERSION3;
const int DIO_DEFAULT_SIZE = 1024L*1024L*4;

#define SM_HDR_SIZE 64

string SM_MetaData::toString(void) {
  string s = "SM_MetaData: { \n"; 
  s += string("mTag: ") + mTag + "\n"; 
  s += string("mType: ");
  if (mType == METADATA_TYPE_ASCII) {
    s += "ASCII: value: \""; 
    s += (mAscii + "\"\n"); 
  } 
  else if (mType == METADATA_TYPE_DOUBLE) {
    s += "DOUBLE: value = "; 
    s += doubleToString(mDouble) + "\n"; 
  } 
  else if (mType == METADATA_TYPE_INT64) {
    s += "INT64: value = "; 
    s += intToString(mInt64) + "\n"; 
  } 
  else {
    s += "UKNOWN ("+intToString(mType)+")\n"; 
  }
  s += "}"; 
  return s;
}

///==========================================================================
// switch byte ordering 
template <class T, class W> 
void SwapEndian(T *ptr, W numelems) {
  //debug5 << "swapping endianism..." << endl;
  W elemsize = sizeof(T); 
  char tmp[32];
  char *target = (char*)ptr;
  
  W i, j;
  for (i=0; i<numelems; i++, target += elemsize){
    memcpy(tmp, target, elemsize);
    for (j=0; j<elemsize; j++)
      target[j] = tmp[elemsize - 1 - j];      
  }
  //delete[] tmp; 
  return;
}

///==========================================================================
template <class T, class W> 
off64_t ReadAndSwap(int fd, T *ptr, W numelems, bool needswap) {
  uint64_t bytes = READ(fd,ptr,numelems*sizeof(T)); 
  if (needswap) SwapEndian(ptr, numelems); 
  return bytes; 
}

///==========================================================================
template <class T, class W> 
off64_t SwapAndWrite(int fd, T *ptr, W numelems, bool needswap) {
  if (needswap) SwapEndian(ptr, numelems); 
  uint64_t bytes = WRITE(fd,ptr,numelems*sizeof(T)); 
  if (needswap) SwapEndian(ptr, numelems); //unswap after write
  return bytes; 
}

///==========================================================================
bool SM_MetaData::Write(int lfd) {
  if (sizeof(double) != 8) {
    smdbprintf(0, "FIXME!  ERROR!  Cannot properly write metadata object since a double is not 8 bytes!\n"); 
    return false; 
  } 
  uint32_t endianTest = 500; 
  bool needswap = (ntohl(endianTest) != endianTest); 

  off64_t filepos = LSEEK64(lfd,0,SEEK_CUR); 
  smdbprintf(5, "SM_MetaData::Write() writing payload type %lu of %s at pos %d\n", mType, toString().c_str(), filepos); 
  SwapAndWrite(lfd, &mType, 1, needswap);
  filepos = LSEEK64(lfd,0,SEEK_CUR); 
    
  uint64_t payloadLength = 8; 
  if (mType == METADATA_TYPE_ASCII) {
    uint64_t stringlen = mAscii.size()+1; 
    SwapAndWrite(lfd, &stringlen, 1, needswap);
    smdbprintf(5, "SM_MetaData::Write() wrote stringlen %d  at pos %d\n",stringlen, filepos); 
    
    filepos = LSEEK64(lfd,0,SEEK_CUR); 
    WRITE(lfd, &mAscii[0], stringlen); 
    smdbprintf(5, "SM_MetaData::Write() wrote string \"%s\" at pos %d\n", mAscii.c_str(), filepos ); 
    payloadLength += stringlen;
  }
  else if (mType == METADATA_TYPE_DOUBLE) {
    SwapAndWrite(lfd, &mDouble,  1, needswap);    
    payloadLength += 8;
  }
  else if (mType == METADATA_TYPE_INT64) {
    SwapAndWrite(lfd, &mInt64,  1, needswap);    
    payloadLength += 8;
  }
  filepos = LSEEK64(lfd,0,SEEK_CUR); 
  SwapAndWrite(lfd, &payloadLength,  1, needswap);   
  smdbprintf(5, "SM_MetaData::Write() wrote payloadLength %lu  at pos %d\n",payloadLength, filepos); 
  
  mTag.resize(1015); // avoid segfaults I hope! 
  filepos = LSEEK64(lfd,0,SEEK_CUR); 
  WRITE(lfd, &mTag[0], 1014); 
  smdbprintf(5, "SM_MetaData::Write() wrote name \"%s\"  at pos %d\n",mTag.c_str(), filepos); 
  
  uint64_t MAGIC = METADATA_MAGIC; 
  SwapAndWrite(lfd, &MAGIC, 1, needswap);
  filepos = LSEEK64(lfd,0,SEEK_CUR); 
  smdbprintf(5, "SM_MetaData::Write() exiting at pos %d\n", filepos ); 
  return true;
}

///==========================================================================
off64_t SM_MetaData::Read(int lfd) {
  if (sizeof(double) != 8) {
    smdbprintf(0, "FIXME!  Error!  Cannot properly read metadata object since a double is not 8 bytes!\n"); 
    return 0; 
  } 
  off64_t filepos = LSEEK64(lfd,0,SEEK_CUR), startpos = 0; 
  smdbprintf(5, "SM_MetaData::Read() at pos %d\n", filepos); 
  uint32_t endianTest = 500; 
  bool needswap = (ntohl(endianTest) != endianTest); 

  uint64_t mdmagic; 
  uint64_t payloadLength, 
    headerLength=sizeof(payloadLength)+sizeof(mdmagic) + 1014; 
  char namebuf[1014]; 

  filepos = LSEEK64(lfd,-headerLength,SEEK_CUR); // to beginning of header, at payload length
  smdbprintf(5, "seeked to beginning of header at pos %d\n", filepos); 
  off64_t headerpos = filepos; 
    
  filepos += ReadAndSwap(lfd, &payloadLength, 1, needswap);  

  filepos += READ(lfd, namebuf, 1014);
  namebuf[1013] = 0; // be safe
  mTag = namebuf; 
  
  filepos += ReadAndSwap(lfd, &mdmagic, 1, needswap);
  if (mdmagic != METADATA_MAGIC) {
    smdbprintf(1, "Found bad magic %ld at pos %d (expected %ld)\n", mdmagic, filepos-8, METADATA_MAGIC); 
    return 0; 
  }
  
  // we need the payload now, so have to seek backwards -- the price to pay for a "linked list" approach for metadata
  smdbprintf(5, "payloadLength is %ld, headerLength is %ld, so have to seek backwards %ld \n", payloadLength, headerLength, payloadLength+headerLength); 
  filepos =  LSEEK64(lfd,-(payloadLength + headerLength + 8),SEEK_CUR);
  startpos = filepos; 

  ReadAndSwap(lfd, &mType, 1, needswap);
  smdbprintf(5, "read payload type %lu at pos %d\n", mType, filepos); 
  filepos = LSEEK64(lfd,0,SEEK_CUR);

  if (mType == METADATA_TYPE_ASCII) {
    uint64_t stringlen; 
    ReadAndSwap(lfd, &stringlen, 1, needswap);
    smdbprintf(5, "read stringlen %d at pos %d\n", stringlen, filepos); 
    filepos = LSEEK64(lfd,0,SEEK_CUR);
    char buf[stringlen+1];
    READ(lfd, buf, stringlen+1); 
    mAscii = buf; 
    smdbprintf(5, "read string %s at pos %d\n", &mAscii[0], filepos); 
  }
  else if (mType == METADATA_TYPE_DOUBLE) {
    ReadAndSwap(lfd, &mDouble,  1, needswap);    
  }
  else if (mType == METADATA_TYPE_INT64) {
    ReadAndSwap(lfd, &mInt64,  1, needswap);    
  }
  
  
  filepos = LSEEK64(lfd,startpos,SEEK_SET); 
  smdbprintf(5, "exiting at pos %d\n", filepos); 
  smdbprintf(5, "Metadata found: %s.\n", toString().c_str());  
  return startpos; 
}



///==========================================================================
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
  smXZ::init();
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
smBase::smBase(const char *_fname, int numthreads, uint32_t bufferSize):mNumThreads(numthreads), mWriteThreadRunning(false), mWriteThreadStopSignal(false) {
  smdbprintf(5, "smBase::smBase(%s, %d)", _fname, numthreads);
  int i;
  setFlags(0);
  setFPS(getFPS());
  mNumFrames = 0;
  mVersion = 0;
  mNumResolutions = 1;
  
  mThreadData.clear(); 
  mThreadData.resize(numthreads); 

  mStagingBuffer = mOutputBuffers[0] = new OutputBuffer(bufferSize); 
  mOutputBuffer = mOutputBuffers[1] = new OutputBuffer(bufferSize); 

  if (_fname != NULL) {
    mMovieName = strdup(_fname);
    int threadnum = mNumThreads;
    while (threadnum--) {
      mThreadData[threadnum].fd = OPEN(mMovieName, O_RDONLY);
    }
    readHeader();
    initWin(); // only does something for version 1.0
  }
  else
    mMovieName = NULL;
  
  if (mNumThreads > 0) {
    // this assumes we have already called pthreads_init(); 
    smdbprintf(5, "smBase::smBase: initializing buffer mutex\n"); 
    int status = pthread_mutex_init(&mBufferMutex, NULL); 
    if (status) {
      fprintf(stderr, "Error:  cannot initializes output buffer mutex\n"); 
      exit(2); 
    }
  }
  bModFile = FALSE;
  return;
}

// adjust the buffer size
void smBase::setBufferSize(uint32_t frames) {
  mStagingBuffer->resize(frames) ;
  mOutputBuffer->resize(frames); 
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

   smdbprintf(5,"smBase destructor : %s",mMovieName);
   if ((bModFile == TRUE) && (mThreadData[0].fd)) CLOSE(mThreadData[0].fd);
   if (mMovieName) free(mMovieName);
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

   if ((SM_MAGIC_3 != magic)  && (SM_MAGIC_2 != magic)  && (SM_MAGIC_1 != magic)) {
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
void smBase::printFrameDetails(FILE *fp,int f, int res)
{
  f = res*getNumFrames() + f; 
  if ((f < 0) || (f >= mNumFrames*mNumResolutions)) {
    fprintf(fp,"%d\tInvalid frame\n",f);
    return;
  }
#ifdef WIN32
  fprintf(fp,"%d\toffset=%I64d\tlength=%d\n",f,mFrameOffsets[f],mFrameLengths[f]);
#else
  fprintf(fp,"%d\t%lld\t%d\n",f,mFrameOffsets[f],mFrameLengths[f]);
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
                    u_int _nframes, u_int *_tsizes, u_int _nres, 
                    int numthreads)
{
   int i;
   mNumFrames = _nframes;
   mNumThreads = numthreads;
   framesizes[0][0] = _width;
   framesizes[0][1] = _height;
   for(i=1;i<8;i++) {
       framesizes[i][0] = framesizes[i-1][0]/2;
       framesizes[i][1] = framesizes[i-1][1]/2;
   }
   memcpy(tilesizes,framesizes,sizeof(framesizes));
   mNumResolutions = _nres;
   if (mNumResolutions < 1) mNumResolutions = 1;
   if (mNumResolutions > 8) mNumResolutions = 8;

   mVersion = 2;
   
  
   for(i=0;i<mNumResolutions;i++) {
     if (_tsizes) {
       if (_tsizes[(i*2)+0] < tilesizes[i][0]) {
         tilesizes[i][0] = _tsizes[(i*2)+0];
       }
       if (_tsizes[(i*2)+1] < tilesizes[i][1]) {
         tilesizes[i][1] = _tsizes[(i*2)+1];
       }
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
		mNumFrames);

   mMovieName = strdup(_fname);

   mThreadData[0].fd = OPENC(_fname, O_WRONLY|O_CREAT|O_TRUNC, 0666);   
   mResFileNames.push_back(_fname); 
   mResFDs.push_back(mThreadData[0].fd); 
   mResFileBytes.resize(mNumResolutions, 0); 
   if (mNumResolutions > 1) {
     int res = 1;
     while (res < mNumResolutions) {
       char filename[] = "/tmp/smBase-tmpfileXXXXXX";
       int fd = mkstemp(filename); 
       smdbprintf(2, "Res file %d is named %s", res, filename); 
       mResFileNames.push_back(filename); 
       mResFDs.push_back(fd); 
       ++res; 
     }
   }
   
   mFrameOffsets.resize(mNumFrames*mNumResolutions, 0); 
   mFrameOffsets[0] = SM_HDR_SIZE*sizeof(u_int) +
	   mNumFrames*mNumResolutions*(sizeof(off64_t)+sizeof(u_int));
   
   mFrameLengths.resize(mNumFrames*mNumResolutions, 0);
   if (LSEEK64(mThreadData[0].fd, mFrameOffsets[0], SEEK_SET) < 0)
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

  int lfd; /* local file descriptor for movie file */ 
   u_int magic, type;
   u_int maxtilesize;
   u_int maxFrameSize;

   int i, w;

   lfd = OPEN(mMovieName, O_RDONLY);
   READ(lfd, &magic, sizeof(u_int));
   magic = ntohl(magic);
   if (magic == SM_MAGIC_1) mVersion = 1;
   if (magic == SM_MAGIC_2) mVersion = 2;

   READ(lfd, &type, sizeof(u_int));
   setFlags(ntohl(type) & SM_FLAGS_MASK);
   type = ntohl(type) & SM_TYPE_MASK;

   READ(lfd, &mNumFrames, sizeof(u_int));
   mNumFrames = ntohl(mNumFrames);
   smdbprintf(4,"open file, mNumFrames = %d", mNumFrames);

   READ(lfd, &i, sizeof(u_int));
   framesizes[0][0] = ntohl(i);
   READ(lfd, &i, sizeof(u_int));
   framesizes[0][1] = ntohl(i);
   for(i=1;i<8;i++) {
     framesizes[i][0] = framesizes[i-1][0]/2;
     framesizes[i][1] = framesizes[i-1][1]/2;
   }
   memcpy(tilesizes,framesizes,sizeof(framesizes));
   mNumResolutions = 1;

   smdbprintf(4,"image size: %d %d", framesizes[0][0], framesizes[0][1]);
   smdbprintf(4,"mNumFrames=%d",mNumFrames);

   // Version 2 header is bigger...
   if (mVersion == 2) {
     maxtilesize = 0;
     maxNumTiles = 0;
     u_int arr[SM_HDR_SIZE];
     LSEEK64(lfd,0,SEEK_SET);
     READ(lfd, arr, sizeof(u_int)*SM_HDR_SIZE);
     byteswap(arr,sizeof(u_int)*SM_HDR_SIZE,sizeof(u_int));
     mNumResolutions = arr[5];
     //smdbprintf(5,"mNumResolutions : %d",mNumResolutions);
     for(i=0;i<mNumResolutions;i++) {
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
       smdbprintf(5,"tileNxNy[%ld,%ld] : maxnumtiles %ld", tileNxNy[i][0], tileNxNy[i][1],maxNumTiles);
     }
     smdbprintf(5,"maxtilesize = %ld, maxnumtiles = %ld",maxtilesize,maxNumTiles);
     
   }
   else {
     // initialize tile info to reasonable defaults
     for (i = 0; i < mNumResolutions; i++) {
       tilesizes[i][0] = framesizes[i][0];
       tilesizes[i][1] = framesizes[i][1];
       tileNxNy[i][0] = 1;
       tileNxNy[i][1] = 1;
     }
   }
   
   // Get the framestart offsets
   mFrameOffsets.resize((mNumFrames+1)*mNumResolutions, 0); 
   READ(lfd, &mFrameOffsets[0], sizeof(off64_t)*mNumFrames*mNumResolutions);
   byteswap(&mFrameOffsets[0],sizeof(off64_t)*mNumFrames*mNumResolutions,sizeof(off64_t));

   // Get the compressed frame lengths...
   mFrameLengths.resize(mNumFrames*mNumResolutions, 0); 
   maxFrameSize = 0; //maximum frame size
   if (mVersion == 2) {
     READ(lfd, &mFrameLengths[0], sizeof(u_int)*mNumFrames*mNumResolutions);
     byteswap(&mFrameLengths[0],sizeof(u_int)*mNumFrames*mNumResolutions,sizeof(u_int));
     for (w=0; w<mNumFrames*mNumResolutions; w++) {
       if (mFrameLengths[w] > maxFrameSize) maxFrameSize = mFrameLengths[w];
     }
   } else {
     // Compute this for older format files...
     for (w=0; w<mNumFrames*mNumResolutions; w++) {
	   mFrameLengths[w] = (mFrameOffsets[w+1] - mFrameOffsets[w]);
       if (mFrameLengths[w] > maxFrameSize) maxFrameSize = mFrameLengths[w];
     }
   }
   /* locate the end of the last frame */ 
   mFrameOffsets[mNumFrames*mNumResolutions] = mFrameOffsets[mNumFrames*mNumResolutions-1] + mFrameLengths[mNumFrames*mNumResolutions-1];

   // Read the metadata if present   
   off64_t filepos = LSEEK64(lfd,0,SEEK_END);
   SM_MetaData md; 
   while (md.Read(lfd)) {
     mMetaData.push_back(md); 
     smdbprintf(5, "Read metadata: %s\n", md.toString().c_str());
   }


#if SM_VERBOSE
   for (w=0; w<mNumFrames; w++) {
     smdbprintf(5,"window %d: %d size %d", w, (int)mFrameOffsets[w],mFrameLengths[w]);
   }
#endif
   smdbprintf(4,"maximum frame size is %d", maxFrameSize);
   
   // bump up the size to the next multiple of the DIO requirements
   maxFrameSize += 2;

   
   CLOSE(lfd);
   for (i=0; i<mThreadData.size(); i++) {
     unsigned long w;
     if(mVersion == 2) {
       // put preallocated tilebufs and tile info support here as well
       mThreadData[i].io_buf.clear(); 
       mThreadData[i].io_buf.resize(maxFrameSize, 0);
       mThreadData[i].tile_buf.clear();
       mThreadData[i].tile_buf.resize(maxtilesize*3, 0);
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
  
  if(mVersion == 1) {
     readFrame(0,0);
  }
}



/*!
  Loop until all read
*/ 
uint32_t smBase::readData(int fd, u_char *buf, int bytes) {
  ///smdbprintf(5, "readData: %d bytes", bytes); 
  uint32_t remaining = bytes; 
  while  (remaining >0) {
    int r=READ(fd, buf, remaining);
    if (!r) {
        smdbprintf(0, "smBase::readData() error: read=%d remaining=%d, unsuccessful read, giving up\n",r, remaining); 
        return bytes-remaining; 
    }
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
  Reads a frame.  Does not decompress the data. 
  \param f The frame to read
  \param threadnum A zero-based thread id.  Threadnum must be less than numthreads.  No other thread should be using this threadnum, as there is one buffer per thread and collisions can happen. 
*/
uint32_t smBase::readFrame(u_int f, int threadnum)
{
  off64_t size;
  
  size = mFrameLengths[f];
  int fd = mThreadData[threadnum].fd;
  
  smdbprintf(5,"readCompressedFrame frame %d, thread %d, %lu bytes at offset %lu\n", f, threadnum, size, mFrameOffsets[f]);
  
  if (LSEEK64(fd, mFrameOffsets[f], SEEK_SET) < 0){
    smdbprintf(0, "Error seeking to frame %d\n", f);
  }

  
  u_char *buf = &mThreadData[threadnum].io_buf[0]; 
  mThreadData[threadnum].currentFrame = f; 
  
  int r = readData(fd, buf, size); 
  if (r == -1) {
    char buf[2048]; 
    smdbprintf(0, "Error in readCompressedFrame  for frame %d: %s\n", f, strerror(errno)); 
    return 0; 
  }
  if (r != size) {
    smdbprintf(0, "Error in readCompressedFrame  for frame %d: read %d bytes but expected %lu\n", f, r, size);
  }
  smdbprintf(5, "Done with readCompressedFrame, read %lu bytes\n", r); 
  return r; 
}

//!  Reads a tiled frame.  Does not decompress the data. Stores in internal buffers for later decompression etc.  
/*!
  Tiles are read in order but can be stored out of order in the read buffer. 
  \param f The frame to read
  \param dim XY dimensions of the region of interest
  \param pos XY position of the region of interest
  \param res resolution desired (level of detail)
  \param threadnum A zero-based thread id.  Threadnum must be less than numthreads.  No other thread should be using this threadnum, as there is one buffer per thread and collisions can happen. 
*/
uint32_t smBase::readTiledFrame(u_int f, int *dim, int* pos, int res, int threadnum)
{
  // version 2 implied -- reads in only overlapping tiles 
  
  uint32_t bytesRead = 0; 
  u_int readBufferOffset=0;
  smdbprintf(5,"readTiledFrame version 2, frame %d, thread %d", f, threadnum); 
  
  off64_t frameOffset = mFrameOffsets[f] ;
  int fd = mThreadData[threadnum].fd; 
  
  if (LSEEK64(fd, frameOffset, SEEK_SET) < 0) {
    smdbprintf(0, "Error seeking to frame %d, frameOffset %d: %s", f, frameOffset, strerror(errno));
    exit(1);
  }
  if (res > mNumResolutions-1) {
    smdbprintf(0, "Error: requested resolution %d does not exist in movie", res);
    exit(1);
  }
    
  int nx = getTileNx(res);
  int ny = getTileNy(res);
  int numTiles =  nx * ny;
  
  // If numTiles > 1 then read in frame header of tile sizes
  uint previousFrame = mThreadData[threadnum].currentFrame; 
  
  if(numTiles > 1 ) {
    u_char *ioBuf = &(mThreadData[threadnum].io_buf[0]);
    int headerSize =  numTiles * sizeof(uint32_t);
    // read the tile sizes from frame header
    int r=readData(fd, ioBuf, headerSize);
    smdbprintf(5, "Read header, %lu bytes from offset %lu in file\n", 
               headerSize, frameOffset); 
    if (r !=  headerSize) { // hmm -- assume we read it all, I guess... iffy?  
      char	s[40];
      sprintf(s,"smBase::readTiledFrame I/O error : r=%d k=%d ",r,headerSize);
      perror(s);
      exit(1); // !!! shit!  whatevs. 
    }
    //byteswap(ioBuf, headerSize, sizeof(uint32_t)); 

    //aliasing to reduce pointer dereferences 
    uint32_t *header = (uint32_t*)&(mThreadData[threadnum].io_buf[0]);
    TileInfo *tileInfo=NULL;
    readBufferOffset = headerSize;
    // get tile sizes and offsets
    uint64_t tileFileOffset = headerSize ;
    //int i=0;
    int tileNum = 0; 
    
    // skip over previous overlaps to find where the next tile should be read into if any new are found
    for(tileNum = 0; tileNum < numTiles;  tileNum++) { 
      tileInfo = &(mThreadData[threadnum].tile_infos[tileNum]);
      tileInfo->compressedSize = (u_int)ntohl(header[tileNum]);
      tileInfo->fileOffset = tileFileOffset;
      tileFileOffset += tileInfo->compressedSize;
      //smdbprintf(5,"tile[%d].frame = %d",i,tileInfo->frame);
      if (previousFrame == f) {
        if ( tileInfo->cached) {
          // This tile has been read, so we can add its size to the number of bytes we know is in the read buffer; it's already marked as cached
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
    
    // Figure out the extents needed to read, based on overlapping tiles.  As we do so, we  mark each tile that needs to be read with its position in the read buffer, so we can copy into it after reading one big chunk.  A bit complicated!  But it's faster to do all this computing than read each tile one at a time. 
    uint64_t firstByte=mFrameLengths[f], // file offset of first byte of the first tile to read
      stopByte = 0;// 1 + the file offset of the last byte in the last tile to read
    uint32_t firstTile = mThreadData[threadnum].tile_infos.size(), 
      stopTile = 0; 
    for(tileNum = 0 ;  tileNum < numTiles;  tileNum++) {
      tileInfo = &(mThreadData[threadnum].tile_infos[tileNum]);
      if(tileInfo->overlaps && ! tileInfo->cached) {
        // We need this tile
        if (firstByte > tileInfo->fileOffset ) {
          firstByte =  tileInfo->fileOffset; 
          firstTile = tileNum; 
        }
        if (stopByte <  tileInfo->fileOffset + tileInfo->compressedSize) {
          stopByte = tileInfo->fileOffset + tileInfo->compressedSize;
          stopTile = tileNum +1; 
        }
        smdbprintf(5, "after tile %d, stopByte=%lu\n", stopByte); 
      }
    }
   
    if (!stopTile) {
      smdbprintf(1, "Skipping read because no tiles are unread.\n"); 
      return 0; 
    }
    if (firstByte > stopByte) {
      smdbprintf(0, "Error: stop byte is before first byte.  Absolutely inconceivable!\n"); 
      abort(); 
    }
    smdbprintf(5, "Reading tiles %d to %d, from byte %lu to %lu\n", 
               firstTile, stopTile-1, firstByte, stopByte-1); 

    uint64_t bytesNeeded = stopByte - firstByte, 
      firstOffset = frameOffset + firstByte; 
    if (LSEEK64(fd,  firstOffset, SEEK_SET) < 0) {
      smdbprintf(0, "Error seeking to first needed tile of frame %d at offset %Ld\n", f, firstOffset);
      exit(1);
    }
    if (mThreadData[threadnum].tile_buf.size() < bytesNeeded) {
      mThreadData[threadnum].tile_buf.resize(bytesNeeded); 
    }
    u_char *tile_buf_ptr =  &mThreadData[threadnum].tile_buf[0];
    r = readData(fd, tile_buf_ptr, bytesNeeded);
    if (r != bytesNeeded ) {
      smdbprintf(0,"smBase::readTiledFrame I/O error thread %d, frame %d, r=%d bytesNeeded=%d, frameOffset=%Ld, firstByte = %Ld : marking all current unread tiles as corrupt and moving returning.",threadnum,f, r, bytesNeeded, frameOffset, firstByte);
      for(tileNum = firstTile; tileNum < stopTile ;  tileNum++) {
        tileInfo = &(mThreadData[threadnum].tile_infos[tileNum]);
        if(tileInfo->overlaps && ! tileInfo->cached) {
          tileInfo->skipCorruptFlag = 1;
        }
      }    
      if (r>0) {
        bytesRead += r; 
      }
      return bytesRead; 
    }
    bytesRead += r; 
    
    // now walk through the tile buffer and  place the tiles in the buffer:
    for(tileNum = firstTile;  tileNum < stopTile;  tileNum++) {
      tileInfo = &(mThreadData[threadnum].tile_infos[tileNum]);
      if(tileInfo->overlaps && ! tileInfo->cached && !tileInfo->skipCorruptFlag) {
        memcpy(ioBuf+readBufferOffset, tile_buf_ptr, tileInfo->compressedSize);
        
        //	smdbprintf(5,"tile %d newly overlaps",tile);
        // store the tile offset in the read buffer for later decompression
        // tiles are read out of order -- this allows incremental caching
        tileInfo->readBufferOffset = readBufferOffset;
        readBufferOffset += tileInfo->compressedSize;
        tileInfo->skipCorruptFlag = 0;        
        tileInfo->cached = 1;    
      }    
      tile_buf_ptr += tileInfo->compressedSize;
      
    } // end memcpy of all new tiles
  }
  smdbprintf(5,"Done with readTiledFrame v2 for frame %d, thread %d, read %d bytes", f, threadnum, bytesRead); 
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
 
uint32_t smBase::getFrameBlock(int frame, void *data, int threadnum,  int destRowStride, int *inDim, int *inPos, int *inStep, int res)
{
  smdbprintf(5,"smBase::getFrameBlock, frame %d, thread %d, data %p\n", frame, threadnum, data); 
   u_char *ioBuf;
   uint32_t size;
   u_char *image;
   u_char *out = (u_char *)data;
   u_char *rowPtr = (u_char *)data;
   uint32_t bytesRead=0; 
   int full_frame_dims[2], _dim[2],_step[2],_pos[2],tilesize[2];
   
   if((res < 0) || (res > (mNumResolutions - 1))) {
     res = 0;
   }
 
   if (inStep) {
       _step[0] = inStep[0]; _step[1] = inStep[1];
   } else {
       _step[0] = 1; _step[1] = 1;
   }
   if (inDim) {
       _dim[0] = inDim[0]; _dim[1] = inDim[1];
   } else {
       _dim[0] = getWidth(res); _dim[1] = getHeight(res);
   }
   if (inPos) {
       _pos[0] = inPos[0]; _pos[1] = inPos[1];
   } else {
       _pos[0] = 0; _pos[1] = 0;
   }
   
   if(_dim[0] <= 0 || _dim[1] <= 0)
     return 0;
   if (_dim[0] + _pos[0] > getWidth(0) ||
       _dim[1] + _pos[1] > getHeight(0)) {
     fprintf(stderr, "dim(%d,%d) + _pos(%d,%d) > dims(%u, %u)\n", 
             _dim[0], _dim[1], _pos[0], _pos[1], getWidth(0), getHeight(0));
     abort(); 
   }
  
   /* move frame into initial resolution */
   if(res > 0)
     frame += mNumFrames * res;
   
   /* pick a resolution based on stepping */
   while((res+1 < getNumResolutions()) && (_step[0] > 1)) {
     res++;
     _step[0] >>= 1;
     _step[1] >>= 1;
     _pos[0] >>= 1;
     _pos[1] >>= 1;
     _dim[0] >>= 1;
     _dim[1] >>=1;
     frame += mNumFrames;
   }
   
   full_frame_dims[0] = getWidth(res);
   full_frame_dims[1] = getHeight(res);
   
   if (!destRowStride)
     destRowStride = _dim[0] * 3;
   
   tilesize[0] = getTileWidth(res);
   tilesize[1] = getTileHeight(res);
   
   // tile support 
   int nx = 0;
   int ny = 0;
   int numTiles = 0;
   u_char *tbuf;
    
   nx = getTileNx(res);
   ny = getTileNy(res);
   numTiles =  nx * ny;

   //assert(numTiles < 1000);

   ioBuf = (u_char *)NULL;

   if((mVersion == 2) && (numTiles > 1)) {
     bytesRead = readTiledFrame(frame, &_dim[0], &_pos[0], res, threadnum);
   }
   else {
     bytesRead = readFrame(frame, threadnum);
   }
   ioBuf = &(mThreadData[threadnum].io_buf[0]); 
   size = mFrameLengths[frame];
   tbuf = (u_char *)&(mThreadData[threadnum].tile_buf[0]);
   TileInfo *tileInfoList = (TileInfo *)&(mThreadData[threadnum].tile_infos[0]);

   
   if(numTiles < 2) {
     smdbprintf(5,"Calling decompBlock on frame %d since numTiles < 2\n", frame);
     if ((_pos[0] == 0) && (_pos[1] == 0) && 
         (_step[0] == 1) && (_step[1] == 1) &&
         (_dim[0] == full_frame_dims[0]) && (_dim[1] == full_frame_dims[1])) {
       smdbprintf(5,"getFrameBlock: decompBlock on entire frame\n"); 
       if (!decompBlock(ioBuf,out,size,full_frame_dims)) {
         smdbprintf(0, "Error decompressing block in frame %d, with dim=(%d,%d), pos=(%d,%d), res=%d\n", frame, _dim[0], _dim[1], _pos[0], _pos[1], res); 
         return 0; 
       }       
     } else {
       smdbprintf(5, "downsampled or partial frame %d\n", frame); 
       image = (u_char *)malloc(3*full_frame_dims[0]*full_frame_dims[1]);
       CHECK(image);
       if (!decompBlock(ioBuf,image,size,full_frame_dims)) {
         smdbprintf(0, "Error decompressing block in frame %d\n", frame); 
         return 0; 
       }       
       for(int y=_pos[1];y<_pos[1]+_dim[1];y+=_step[1]) {
         u_char *dest = rowPtr;
         const u_char *p = image + 3*full_frame_dims[0]*y + _pos[0]*3;
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
     smdbprintf(5, "Done with single tiled image\n"); 
   } /* END single tiled simage */ 
   else { 
     smdbprintf(5,"smBase::getFrameBlock(frame %d, thread %d): process across %d overlapping tiles\n", frame, threadnum, numTiles); 
     uint32_t copied=0;
     for(int tile=0; tile<numTiles; tile++){
       //smdbprintf(5,"tile %d", tile); 
       TileInfo tileInfo = tileInfoList[tile];
       //smdbprintf(5, "thread %d, Frame %d, tile %s", threadnum, f, tileInfo.toString().c_str());        
       if(tileInfo.overlaps && (tileInfo.skipCorruptFlag == 0)) {
         
         u_char *tdata = (u_char *)(ioBuf + tileInfo.readBufferOffset);
         smdbprintf(5,"decompBlock, tile %d\n", tile); 
         decompBlock(tdata,tbuf,tileInfo.compressedSize,tilesize);
         // 
         u_char *to = (u_char*)(out + (tileInfo.blockOffsetY * destRowStride) + (tileInfo.blockOffsetX * 3));
         u_char *from = (u_char*)(tbuf + (tileInfo.tileOffsetY * tilesize[0] * 3) + (tileInfo.tileOffsetX * 3));
         int maxX = tileInfo.tileLengthX, maxY = tileInfo.tileLengthY; 
         uint32_t newBytes = 3*maxX*maxY, maxAllowed = (_dim[0]*_dim[1]*3);
         uint32_t newTotal = newBytes + copied; 
         
         smdbprintf(5, "thread %d, Frame %d, tile %d, copying %d rows %d pixels per row, %d new bytes, %d copied so far, new total will be %d, max allowed is %d x %d x 3 = %d\n", 
           threadnum, frame, tile, maxY, maxX, newBytes, 
           copied, newTotal,  
           _dim[0], _dim[1], maxAllowed ); 
         
         if (newTotal > maxAllowed) {
           smdbprintf(0, "Houston, we have a problem. Dump core here. new total %lu > maxAllowed  %lu\n", newTotal, maxAllowed); 
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
   
   smdbprintf(5,"END smBase::getFrameBlock, frame %d, thread %d, bytes read = %d\n", frame, threadnum, bytesRead); 
   return bytesRead; 
 }

/*!
  Work proc for the writing thread -- just calls sm->writeThread()
*/
void *writeThreadFunction(void *arg) {
  smdbprintf(5, "writeThreadFunction\n"); 
  ((smBase*) arg)-> writeThread(); 
  return NULL; 
}

/*!
  Called by work proc for the writing thread -- keeps flushing the buffer. 
*/
void smBase::writeThread(void) {
  smdbprintf(2, "Starting writeThread\n"); 
  while (!mWriteThreadStopSignal) {
    if (!flushFrames(false)) {
      usleep(1000); 
    }
  }
  return; 
}

/*!
  Starts the writing thread 
*/
void smBase::startWriteThread(void) {
  mWriteThreadRunning = true; 
  mWriteThreadStopSignal = false; 
  pthread_create(&mWriteThread, NULL, writeThreadFunction, this); 
  return; 
}
/*!
  Stops the writing thread.  Does not return until thread is dead
*/
void smBase::stopWriteThread(void) {
  mWriteThreadStopSignal = true; 
  pthread_join(mWriteThread, NULL); 
  mWriteThreadRunning = false; 
  return; 
}  

/*!
  1) close all resolution tmp files written by flushFrames()
  2) reopen each resolution file  and cat onto the end of our movie.  
  3) Fix all file offsets at this point from relative to absolute
*/
//#define CHUNK_SIZE (50*1000*1000)
#define CHUNK_SIZE (50*1000*1000)

void smBase::combineResolutionFiles(void) {
  smdbprintf(1, "combining Resolution Files into final movie...\n"); 
  if (mResFDs.size() < 2) return; 
  boost::shared_array<char> bufptr(new char[CHUNK_SIZE+1]); 
  char *buf = bufptr.get(); 
  int res = 1; 
  while (res < mNumResolutions) {
    // we are going to place this resolution at the end of the movie file, 
    // so adjust the offsets by the current movie file length
    uint64_t offsetAdjust = LSEEK64(mResFDs[0], 0, SEEK_END); // get file length
    smdbprintf(4, "offsetAdjust is %lld\n", offsetAdjust); 
    smdbprintf(1, "combining Resolution level %d from file %s into final movie, which has size of %lld bytes...\n", res, mResFileNames[res].c_str(), offsetAdjust); 
    uint32_t frame = 0, resframe = res*mNumFrames; 
    while (frame++ < mNumFrames) { 
      mFrameOffsets[resframe++] += offsetAdjust; 
    }

    // close the res file and reopen as readonly:
    int fd = mResFDs[res]; 
    if (close(fd) == -1) {
      smdbprintf(0, "ERROR: failed to close res file %s.\n", mResFileNames[res].c_str());
      perror("combineResolutionFiles"); 
      abort(); 
    }
      
    fd = open(mResFileNames[res].c_str(), O_RDONLY); 
    if (fd == -1) {
      smdbprintf(0, "ERROR: failed to open res file %s for reading.\n", mResFileNames[res].c_str());
      perror("combineResolutionFiles"); 
      abort(); 
    }
    // spin through the file and grab bytes and copy to "real" movie
    uint64_t inputFileSize = mResFileBytes[res], actualFileBytes = LSEEK64(fd, 0, SEEK_END);   
    if (actualFileBytes != inputFileSize) {
      smdbprintf(0, "ERROR: Res file %s has %lld bytes but we expect %lld bytes.\n", mResFileNames[res].c_str(), actualFileBytes, inputFileSize);    
      perror("combineResolutionFiles"); 
      abort(); 
    }
    LSEEK64(fd, 0, SEEK_SET); 
    uint64_t  bytesRemaining = inputFileSize; 
    while (bytesRemaining) {
      uint64_t bytesToRead = bytesRemaining; 
      if (bytesToRead > CHUNK_SIZE) bytesToRead = CHUNK_SIZE; 
      smdbprintf(3, "bytesRemaining: %lld, bytesToRead = %lld\n", bytesRemaining, bytesToRead); 
      uint64_t bytesRead = read(fd, buf, bytesToRead); 
      if (bytesRead <= 0) {
        smdbprintf(0, "Error: failed read from res file %s\n", mResFileNames[res].c_str()); 
        perror("combineResolutionFiles"); 
        abort(); 
      }
      
      uint64_t bytesWritten = WRITE(mResFDs[0], buf, bytesRead); 
      if (bytesWritten != bytesRead) {
         smdbprintf(0, "Error: failed write from res file %s to movie file\n", mResFileNames[res].c_str()); 
        perror("combineResolutionFiles"); 
        abort(); 
      }       
      bytesRemaining -= bytesRead;
      smdbprintf(1, "Res %d: Wrote %lld MB, %0.1f%%, size of movie file is %lld MB\n", 
                 res, 
		 (inputFileSize - bytesRemaining)/(1000*1000), 
		 ((float)inputFileSize - bytesRemaining)/(float)inputFileSize, 
                 LSEEK64(mResFDs[0], 0, SEEK_CUR)/(1000*1000)); 
                 
    }
    smdbprintf(1, "Finished copying %lld bytes from res %d file %s to movie file, which is now %d bytes. Unlinking res file\n", inputFileSize, res, mResFileNames[res].c_str(), LSEEK64(mResFDs[0], 0, SEEK_END)); 
    close (fd); 
    unlink (mResFileNames[res].c_str() ); 
    ++ res; 
  }
  bModFile = TRUE;
  return; 
}

/*! 
  Called from the I/O thread to flush the staging buffer iff it is full.  
  If force == true, then does not check to see if the buffer is full.  
  To flush the buffer, first swap buffers, then write.  
*/
bool smBase::flushFrames(bool force) {
  //  smdbprintf(5, "flushFrames(%d)\n", (int)force); 
  if (!force && !mStagingBuffer->full()) {
    //smdbprintf(5, "flushFrames() returning false\n");     
    return false; 
  }
  if (!mOutputBuffer->empty()) {
    cerr << "Error:  mOutputBuffer has data in it already before swapping!" << endl; 
    abort(); 
  }
  // swap buffers: 
  smdbprintf(5, "flushFrames() locking buffer mutex for buffer swap\n"); 
  pthread_mutex_lock(&mBufferMutex); 
  OutputBuffer *tmpBuf = mOutputBuffer; 
  mOutputBuffer = mStagingBuffer; 
  mStagingBuffer = tmpBuf; 
  mStagingBuffer->mFirstFrameNum = mOutputBuffer->mFirstFrameNum + mOutputBuffer->mNumFrames; 
  smdbprintf(4, "flushFrames() swapped buffers\n");
  smdbprintf(5, "flushFrames() unlocking mutex\n");     
  pthread_mutex_unlock(&mBufferMutex); 
  smdbprintf(2, "flushing %d frames from output buffer\n", mOutputBuffer->mNumFrames); 
  
  // see how much of a write buffer we need and reallocate if needed: 
  if (mOutputBuffer->mRequiredWriteBufferSize > mWriteBuffer.size()) {
    smdbprintf(3, "Resizing write buffer to %d\n", mOutputBuffer->mRequiredWriteBufferSize); 
    mWriteBuffer.resize(mOutputBuffer->mRequiredWriteBufferSize, 42); 
  }

  // copy each level of the output buffer: 
  int res = 0; 
  int firstFrame = mOutputBuffer->mFirstFrameNum, 
    stopFrame =  mOutputBuffer->mFirstFrameNum + mOutputBuffer->mNumFrames;
  
  while (res < mNumResolutions) {

    string filename = mResFileNames[res]; 
    int fd =  mResFDs[res];   
    // figure out the offset to the first frame in the resolution:
    uint64_t firstFileOffset = 0; // true for res!=0 and firstFrame == 0, as they write to temp files
    uint32_t frame = firstFrame;   
    uint32_t resFrame = frame + res*mNumFrames; 
    
    if (firstFrame != 0) {
      firstFileOffset = mFrameOffsets[resFrame-1] + mFrameLengths[resFrame-1];
    }     
    else { //  firstFrame == 0) {
      if (res == 0 ) {
        firstFileOffset = mFrameOffsets[0]; // already pre-computed
      }
    }
    uint64_t fileOffset = firstFileOffset; 

    // setup some other variables for loop
    int numTiles = getTileNx(res)*getTileNy(res);
    uint32_t buffBytes = 0; // keep track of bytes to write
    u_char *bufptr = &mWriteBuffer[0];
    // copy all frames in this resolution to the output buffer:
    while (frame < stopFrame) {
      FrameCompressionWork *frameData = mOutputBuffer->mFrameBuffer[frame-firstFrame]; 
      smdbprintf(5, "Writing frameData:  %s\n", frameData->toString().c_str());
      mFrameOffsets[resFrame] = fileOffset; 
      mFrameLengths[resFrame] = 0;
      // copy the tile info if needed into the output buffer:
      int tileBytes = 0; 
      if (mVersion != 1 && numTiles > 1) {  
        tileBytes = numTiles*sizeof(uint32_t);
        memcpy(bufptr, &frameData->mCompTileSizes[res][0], tileBytes); 
        byteswap(bufptr, tileBytes, sizeof(uint32_t));
        mFrameLengths[resFrame] += tileBytes;   
        bufptr += tileBytes;  
        fileOffset += tileBytes; 
        buffBytes += tileBytes;
      }
      // copy the image into the output buffer:  
      int frameBytes = frameData->mCompFrameSizes[res]; 
      buffBytes += frameBytes;
      if (buffBytes >  mOutputBuffer->mRequiredWriteBufferSize) {
        smdbprintf(0, "Houston, we have a buffer overflow. buffBytes = %lu, mRequiredWriteBufferSize = %lu  :-(\n", buffBytes, mOutputBuffer->mRequiredWriteBufferSize); 
        abort(); 
      }
      memcpy(bufptr, &frameData->mCompressed[res][0], frameBytes); 
      mFrameLengths[resFrame] += frameBytes;;
      bufptr += frameBytes;;
      fileOffset += frameBytes;
      smdbprintf(5, "Copied %lu bytes to buffer, %lu for tile header, %lu for compressed frame\n", tileBytes+frameBytes, tileBytes, frameBytes);  
      ++frame;
      ++resFrame; 
    }
    // now write the entire buffer out into the temp file: 
    LSEEK64(fd, firstFileOffset, SEEK_SET); 
    WRITE(fd, &mWriteBuffer[0], buffBytes); 
    mResFileBytes[res] += buffBytes; 
    smdbprintf(3, "flushFrames( is writing resolution frames to buffer:  Res=%d, startFrame=%d, numFrames=%d, bufbytes = %d , requiredBytes=%d, firstFileOffset = %lld, mResFileBytes = %lld, actual res file length = %lld, filename=%s.\n", res, firstFrame, stopFrame-firstFrame, buffBytes, mOutputBuffer->mRequiredWriteBufferSize, firstFileOffset, mResFileBytes[res], LSEEK64(fd, 0, SEEK_END), filename.c_str()); 
    ++res; 
  }
  uint32_t f = mOutputBuffer->mFrameBuffer.size(); 
  while (f--) {
    if (mOutputBuffer->mFrameBuffer[f]) {
      delete mOutputBuffer->mFrameBuffer[f]; 
      mOutputBuffer->mFrameBuffer[f] = NULL;
    }
  }
  mOutputBuffer->mNumFrames = 0; 
  mOutputBuffer->mRequiredWriteBufferSize = 0; 
  bModFile = TRUE;
  return true; 
}
/*! 
  Compress the frame, then place it into the output buffer for writing.  
  In order for buffered frames to actually get written,  call flushFrames() 
  occasionally, whether from the same thread or another.  Make sure enough 
  frames are buffered so that I/O proceeds in large chunks.  
 */
void smBase::compressAndBufferFrame(int f,  u_char *data) {  
  smdbprintf(4, "compressAndBufferFrame(frame=%d)\n", f); 
  FrameCompressionWork *wrk = new FrameCompressionWork(f, data); 
  compressFrame(wrk); 

  bool canBuffer = false; 
  smdbprintf(5, "compressAndBufferFrame: locking buffer mutex\n"); 
  pthread_mutex_lock(&mBufferMutex); 
  while (!mStagingBuffer->addFrame(wrk)) {
    smdbprintf(5, "compressAndBufferFrame: unlocking buffer mutex\n"); 
    pthread_mutex_unlock(&mBufferMutex); 
    usleep(1500); 
    smdbprintf(5, "compressAndBufferFrame: locking buffer mutex\n"); 
    pthread_mutex_lock(&mBufferMutex); 
  } 
  smdbprintf(5, "compressAndBufferFrame: unlocking buffer mutex\n"); 
  pthread_mutex_unlock(&mBufferMutex); 
  
  return; 
}

/*!
  Thread-safe compression routine to help get speedup on multicore machines.  
  This allows writing to be deferred until later, so that more than one thread
  can compress and I/O can be done from a single thread without any other work
  to do.   
  Memory is freed when the wrk pointer is deleted.  
*/ 
void smBase::compressFrame(FrameCompressionWork *wrk) {
  smdbprintf(4, "START compressFrame, frame=%d\n", wrk->mFrame); 
  if (wrk->mCompressed.size()) {
    smdbprintf(0, "Warning: Unexpected memory allocation present in work quantum.  Probably that should not ever happen.\n");      
  } 
  if (wrk->mCompressed.size() < mNumResolutions) {
    wrk->mCompressed.resize(mNumResolutions, NULL);  
    wrk->mCompFrameSizes.resize(mNumResolutions, 0); 
    wrk->mCompTileSizes.resize(mNumResolutions);
  }
  int res = 0; 
  for (res=0; res<mNumResolutions; res++) {
    int numTiles = getTileNx(res)*getTileNy(res);
    if (wrk->mCompTileSizes[res].size() < numTiles) {
      wrk->mCompTileSizes[res].resize(numTiles, 0);      
    } 
  }
  wrk->mCompFrameSizes[0] = compFrame(wrk->mUncompressed, NULL, &wrk->mCompTileSizes[0][0], 0); 
  // Set the zeroth resolution
  wrk->mCompressed[0] = new u_char [wrk->mCompFrameSizes[0]];
  wrk->mCompFrameSizes[0] = compFrame(wrk->mUncompressed,wrk->mCompressed[0],&wrk->mCompTileSizes[0][0],0);
  
  // quick out
  if (getNumResolutions() == 1) return;
  
  // Now the mipmaps...
  u_char *scaled0 = new u_char[getWidth(0)*getHeight(0)*3];
  u_char *scaled1 = new u_char[getWidth(0)*getHeight(0)*3];
  CHECK(scaled0);
  CHECK(scaled1);
  memcpy(scaled0,wrk->mUncompressed,getWidth(0)*getHeight(0)*3);
  for(res=1;res<getNumResolutions();res++) {
    
    Sample2d(scaled0,getWidth(res-1),getHeight(res-1),
             scaled1,getWidth(res),getHeight(res),
             0,0,getWidth(res-1),getHeight(res-1),1);
    
    // write the frame level
    wrk->mCompFrameSizes[res] = compFrame(scaled1,NULL,&wrk->mCompTileSizes[res][0],res);
    wrk->mCompressed[res]= new u_char[wrk->mCompFrameSizes[res]];
    wrk->mCompFrameSizes[res] = compFrame(scaled1,wrk->mCompressed[res],&wrk->mCompTileSizes[res][0],res);
    smdbprintf(4, "res %d, frame %d compressed size: %d\n", res, wrk->mFrame, wrk->mCompFrameSizes[res] ); 
    u_char *tmp = scaled0; 
    scaled0 = scaled1;
    scaled1 = tmp; 
  }
  delete scaled0;
  delete scaled1;
  smdbprintf(4, "END compressFrame: wrk=%s\n", wrk->toString().c_str()); 
  return; 
}

/*!
  Compress the data and calls helpers to write it out to disk. Writing one 
  frame at a time is bad for performance;  I just rewrote the old code with my new compressFrame() code as a test case.  
  For better performance, call bufferFrame() and then occasionally flushFrames(). 
  \param f the frame number
  \param data frame data to write.
*/ 
void smBase::compressAndWriteFrame(int f, u_char *data)
{
  smdbprintf(4, "compressAndWriteFrame(%d)\n", f); 
  FrameCompressionWork wrk(f, data); 

  compressFrame(&wrk); 
   
  int res=0; 
  for(res=0;res<getNumResolutions();res++) {
    writeCompFrame(f,wrk.mCompressed[res],&wrk.mCompTileSizes[res][0],res);
  }
     
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
void smBase::writeCompFrame(int f, void *data, int *sizes, int res)
{
  smdbprintf(4, "writeCompFrame(f=%d, res=%d)\n", f, res); 
  uint32_t tz;
  int numTiles = getTileNx(res)*getTileNy(res);
  int fd = mThreadData[0].fd;
  mFrameOffsets[f+res*mNumFrames] = LSEEK64(fd,0,SEEK_CUR);
 
  if (mVersion == 1 || numTiles == 1) {
    mFrameOffsets[f+res*mNumFrames] = LSEEK64(mThreadData[0].fd,0,SEEK_CUR);
    mFrameLengths[f+res*mNumFrames] = sizes[0];
    WRITE(mThreadData[0].fd, data, sizes[0]);
    
    bModFile = TRUE;
    return;
  }

  int size = 0;
  // if frame is tiled then write tile offset (jump table) first
  if(numTiles > 1) {
    for(int i = 0; i < numTiles; i++) {
      tz =  htonl((uint32_t)sizes[i]);
      WRITE(fd,&tz,sizeof(uint32_t));
      //smdbprintf(5,"size tile[%d] = %d\n",i,sizes[i]);
      size += sizes[i];
    } 
    mFrameLengths[f+res*mNumFrames] = size + numTiles*sizeof(uint32_t);
   
  }
  else {
     size += sizes[0];
     mFrameLengths[f+res*mNumFrames] = size;
  }
  //smdbprintf(5,"write %d bytes\n",size);
  WRITE(fd, data, size);

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
   readFrame(frame+res*mNumFrames, threadnum);
   ioBuf = &(mThreadData[threadnum].io_buf[0]); 
   size = mFrameLengths[frame];
   
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
	return(mFrameLengths[frame+res*mNumFrames]);
}



//! Wrapper around compBlock, compresses a frame as a set of tiles
/*!
  \param in uncompressed data
  \param out compressed result
  \param outsizes sizes of compressed tiles in outbuffer
  \param res resolution (level of detail)
*/ 
int smBase::compFrame(void *in, void *out, int *outsizes, int res)
{
   int compressedSize = 0;
   int nx = getTileNx(res);
   int ny = getTileNy(res);
   int numTiles = nx*ny; 
   
   int frameDims[2];
   frameDims[0] = getWidth(res);
   frameDims[1] = getHeight(res);

   
   if (mVersion == 1 || numTiles == 1) {
     if (!out) {
       // How big is the frame
       compBlock(in,NULL,compressedSize,frameDims);
     } else {
       // build the compressed frame...
       compBlock(in,out,compressedSize,frameDims);
     }
     outsizes[0] = compressedSize; 
     return compressedSize;
   }

   int tileDims[2];
   tileDims[0] = getTileWidth(res);
   tileDims[1] = getTileHeight(res);

   smdbprintf(5,"smBase::compFrame: frameDims[%d,%d] , tileDims[%d,%d]\n",frameDims[0],frameDims[1],tileDims[0],tileDims[1]);
   
   uint32_t tilepixelbytes = tileDims[0] * tileDims[1] * 3;
   char *tilebuf = new char[tilepixelbytes];
   CHECK(tilebuf);
   char *outp = (char*)out;
   char *base  = (char*)in;
   
   for(uint32_t j=0;j<ny;j++) {
     for(uint32_t i=0;i<nx;i++) {
       int size =0, tileLineBytes=tileDims[0]*3; 
       long offset =  (((j * tileDims[1] * frameDims[0]) + (i * tileDims[0]))*3); 
       smdbprintf(5,"compFrame tile index[%d,%d], offset = %d\n",i,j,offset);
       base = (char*)in + offset;
       if(((i+1) * tileDims[0]) > frameDims[0]) {
         tileLineBytes = (frameDims[0] - (i*tileDims[0])) * 3;
         memset(tilebuf,0,tilepixelbytes);
       }
       else {
         //msize = tileDims[0]*3;
         if(((j+1) * tileDims[1]) > frameDims[1]) {
           memset(tilebuf,0,tilepixelbytes);
         }
       }
       uint32_t maxk = tileDims[1];
       if (maxk > frameDims[1] - (j * tileDims[1])) {
         maxk = frameDims[1] - (j * tileDims[1]);
       }
       for(int k=0; k<maxk; k++) {
         /*       for(int k=0;k<tileDims[1];k++) {
                  if(((j * tileDims[1])+k) == frameDims[1]) 
                  break;
         */
         memcpy(tilebuf+(k*tileDims[0]*3),
                base+(k*frameDims[0]*3),
                tileLineBytes);
       }
       
       
       if (!out) {
         // How big is the tile
         compBlock(tilebuf,NULL,size,tileDims);
       } else {
         // build the compressed frame...
         compBlock(tilebuf,outp,size,tileDims);
         outp += size;
       }
       
       *(outsizes + (j*nx) + i) = size;
       compressedSize += size; 
     }
   }
#if SM_DUMP  
   if(out) {
     char *p = (char *)out;
     for(int dd = 0; dd < frameDims[0]*3*frameDims[1];dd++)
       smdbprintf(5," %d ",p[dd]);
   }
#endif 
   delete [] tilebuf;
   return compressedSize;
}

/* close the file and write out the header
 */
void smBase::closeFile(void)
{
   u_int arr[64] = {0};
   smdbprintf(3, "smBase::closeFile"); 

   if (bModFile == TRUE) {
     this->combineResolutionFiles(); 
	int i;

   	LSEEK64(mThreadData[0].fd, 0, SEEK_SET);

   	arr[0] = SM_MAGIC_2;
   	arr[1] = getType() | getFlags();
 	arr[2] = mNumFrames;
  	arr[3] = framesizes[0][0];
   	arr[4] = framesizes[0][1];
	arr[5] = mNumResolutions;
	//smdbprintf(5,"mNumResolutions = %d\n",mNumResolutions);
	for(i=0;i<mNumResolutions;i++) {
		arr[i+6] = (tilesizes[i][1] << 16) | tilesizes[i][0];
	}
   	byteswap(arr,sizeof(u_int)*SM_HDR_SIZE,sizeof(u_int));
   	WRITE(mThreadData[0].fd, arr, sizeof(u_int)*SM_HDR_SIZE);

   	byteswap(&mFrameOffsets[0],sizeof(off64_t)*mNumFrames*mNumResolutions,sizeof(off64_t));
   	WRITE(mThreadData[0].fd, &mFrameOffsets[0], sizeof(off64_t)*mNumFrames*mNumResolutions);
   	byteswap(&mFrameOffsets[0],sizeof(off64_t)*mNumFrames*mNumResolutions,sizeof(off64_t));

   	byteswap(&mFrameLengths[0],sizeof(u_int)*mNumFrames*mNumResolutions,sizeof(u_int));
   	WRITE(mThreadData[0].fd, &mFrameLengths[0], sizeof(u_int)*mNumFrames*mNumResolutions);
   	byteswap(&mFrameLengths[0],sizeof(u_int)*mNumFrames*mNumResolutions,sizeof(u_int));

	//smdbprintf(5,"seek header end is %d\n",LSEEK64(mThreadData[0].fd, 0, SEEK_CUR));
   }
   int i=mNumThreads; 
   while (i--) {
     CLOSE(mThreadData[i].fd);
   }
   smdbprintf(1, "Finished with movie %s\n", mMovieName);
   return;
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

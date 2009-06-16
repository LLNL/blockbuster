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
// smBase.C - base class for "streamed movies"
//

#define DMALLOC 1
#undef DMALLOC

#include <assert.h>
#include <stdio.h>

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

#ifdef DISABLE_PTHREADS
#warning pthreads disabled
void fake_mutexer(char *c, int i=0, int j=0) {
  i=j=*c;
  return; 
}
void fake2(char *, char *) {
  return; 
}
#define pthread_cond_init fake_mutexer
#define pthread_cond_wait fake2
#define pthread_cond_destroy fake_mutexer
#define pthread_cond_broadcast fake_mutexer
#define pthread_cond_signal fake_mutexer
#define pthread_mutex_init fake_mutexer
#define pthread_mutex_lock fake_mutexer
#define pthread_mutex_unlock fake_mutexer
#define pthread_mutex_destroy fake_mutexer

#endif // end test for DISABLE_PTHREADS
 
const int SM_MAGIC_1=SM_MAGIC_VERSION1;
const int SM_MAGIC_2=SM_MAGIC_VERSION2;
const int DIO_DEFAULT_SIZE = 1024L*1024L*4;

#define SM_HDR_SIZE 64


//===============================================
// debug print statements
#define SM_VERBOSE 1
//#undef  SM_VERBOSE
static int smVerbose = 0; 
void sm_setVerbose(int level) {
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
#define smdbprintf if(0) printf
#endif
//===============================================

#define CHECK(v) \
if(v == NULL) \
sm_fail_check(__FILE__,__LINE__)

void sm_fail_check(const char *file,int line)
{
  perror("fail_check");
  fprintf(stderr,"Failed at line %d in file %s\n",line,file);
  exit(1); 
}



u_int smBase::nwin = 1; 
u_int smBase::ntypes = 0;
u_int *smBase::typeID = NULL;
smBase *(**smBase::smcreate)(const char *, int) = NULL;

static void  byteswap(void *buffer,off64_t len,int swapsize);
static void Sample2d(unsigned char *in,int idx,int idy,
		     unsigned char *out,int odx,int ody,
		     int s_left,int s_top,int s_dx,int s_dy,int filter);

void smBase::init(int _nwin)
{
  static int initialized = 0; 
  if (initialized) return; 
  initialized = 1;
  smdbprintf(5,"smBase::init(%d", _nwin); 

  smJPG::init();
  smLZO::init();
  smGZ::init();
  smRLE::init();
  smRaw::init();

  nwin = _nwin?_nwin:1; 

  return; 
}

//----------------------------------------------------------------------------
// smBase - constructor initializes the synchronization primitives
//
//----------------------------------------------------------------------------
smBase::smBase(const char *_fname)
{
   int i;
   setFlags(0);
   setFPS(getFPS());
   nframes = 0;
   foffset = NULL;
   flength = NULL;
   version = 0;
   nresolutions = 1;

   //nwin = _nwin;
   smdbprintf(5,"smBase constructor : nwin %d",nwin);
   winlock = (u_int *)calloc(sizeof(u_int) , nwin);
   CHECK(winlock);
   currentFrame = (int *)calloc(sizeof(int) , nwin);
   CHECK(currentFrame);
   win = (void **)calloc(sizeof(void *) , nwin);
   CHECK(win);
   dio_buf = (void **)calloc(sizeof(void *) , nwin);
   CHECK(dio_buf);
   dio_free = (void **)calloc(sizeof(void *) , nwin);
   CHECK(dio_free);
   tile_buf = (u_char **)calloc(sizeof(u_char *),nwin);
   CHECK(tile_buf);
   tile_offsets = (u_int **)calloc(sizeof(u_int *),nwin);
   CHECK(tile_offsets);
   tile_info = (tileOverlapInfo_t **)calloc(sizeof(tileOverlapInfo_t*),nwin);
   CHECK(tile_info);
   winmut = (pthread_mutex_t *)calloc(sizeof(pthread_mutex_t) , nwin);
   CHECK(winmut);
   wincond = (pthread_cond_t *)calloc(sizeof(pthread_cond_t) , nwin);
   CHECK(wincond);
   fd = (int *)calloc(sizeof(int) , nwin);
   CHECK(fd);
   for (i=0; i<nwin; i++) {
      currentFrame[i] = -1;
      win[i] = NULL;
      dio_buf[i] = NULL;
      dio_free[i] = NULL;
      winlock[i] = 0;
      pthread_mutex_init(&winmut[i], NULL);
      pthread_cond_init(&wincond[i], NULL);
   }

   pthread_mutex_init(&writelock, NULL);

   if (_fname != NULL) {
      fname = strdup(_fname);
      for (i=0; i<nwin; i++) fd[i] = OPEN(fname, O_RDONLY);
      dio_mem = 0;
      dio_min = 1;
      dio_max = DIO_DEFAULT_SIZE;
      readHeader();
      initWin(); // only does something for version 1.0
   }
   else
      fname = NULL;

   bModFile = FALSE;

   return;
}

//----------------------------------------------------------------------------
// ~smBase - destructor
//
//----------------------------------------------------------------------------
smBase::~smBase()
{
   int i;

   smdbprintf(5,"smBase destructor : %s",fname);
   if ((bModFile == TRUE) && (fd[0])) CLOSE(fd[0]);
   for(i=0;i<nwin;i++) {
   	if ((bModFile == FALSE) && (fd[i])) CLOSE(fd[i]);
	if (dio_free[i]) free(dio_free[i]);
	if (tile_buf[i]) free(tile_buf[i]);
	if (tile_offsets[i]) free(tile_offsets[i]);
	if (tile_info[i]) free(tile_info[i]);
	pthread_mutex_destroy(&winmut[i]);
	pthread_cond_destroy(&wincond[i]);
   }
   pthread_mutex_destroy(&writelock);
   free(wincond);
   free(winmut);
   free(dio_buf);
   free(dio_free);
   free(tile_buf);
   free(tile_offsets);
   free(tile_info);
   free(win);
   free(currentFrame);
   free(winlock);
   free(foffset);
   free(flength);
   free(fd);
   if (fname) free(fname);
}

//----------------------------------------------------------------------------
// openFile - open a file of unknown compression type
//
//----------------------------------------------------------------------------
smBase *smBase::openFile(const char *_fname, int _nwin)
{
   smBase *base;
   u_int magic;
   u_int type;
   int fd;
   int i;

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

   base = (*(smcreate[i]))(_fname, _nwin);

   CLOSE(fd);

   return(base);
}

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
   fd = (int *)malloc(sizeof(int));
   CHECK(fd);

   fd[0] = OPENC(_fname, O_WRONLY|O_CREAT|O_TRUNC, 0666);
   assert(fd[0] != 0);

   foffset = (off64_t *)calloc(sizeof(off64_t),nframes*nresolutions);
   CHECK(foffset);
   foffset[0] = SM_HDR_SIZE*sizeof(u_int) +
	   nframes*nresolutions*(sizeof(off64_t)+sizeof(u_int));
   flength = (u_int *)calloc(sizeof(u_int),nframes*nresolutions);
   CHECK(flength);
   seekToFrame(fd[0], 0);

   bModFile = TRUE;

   return(0);
}

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

void smBase::seekToFrame(int fd, u_int f)
{
  
  off64_t offset = foffset[f];

  if (LSEEK64(fd, offset, SEEK_SET) < 0)
    fprintf(stderr, "Error seeking to frame %d\n", f);
}


void smBase::readHeader(void)
{
   int lfd;
   u_int magic, type;
   u_int maxtilesize;
   u_int maxwin;

   int i, w;

   // the file size is needed for the size of the last frame
   filesize = LSEEK64(fd[0],0,SEEK_END);

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
   maxwin = 0;
   if (version == 2) {
       READ(lfd, flength, sizeof(u_int)*nframes*nresolutions);
       byteswap(flength,sizeof(u_int)*nframes*nresolutions,sizeof(u_int));
       for (w=0; w<nframes*nresolutions; w++) {
           if (flength[w] > maxwin) maxwin = flength[w];
       }
   } else {
       // Compute this for older format files...
       for (w=0; w<nframes*nresolutions; w++) {
	   flength[w] = (foffset[w+1] - foffset[w]);
           if (flength[w] > maxwin) maxwin = flength[w];
       }
   }
#if 0 && SM_VERBOSE
   for (w=0; w<nframes; w++) {
       smdbprintf(5,"window %d: %d size %d", w, (int)foffset[w],flength[w]);
   }
#endif
   smdbprintf(5,"maximum window size is %d", maxwin);

   // bump up the size to the next multiple of the DIO requirements
   maxwin += dio_mem + 2*dio_min;
   CLOSE(lfd);
   for (i=0; i<nwin; i++) {
      unsigned long w;
      if(version == 2) {
	// put preallocated tilebufs and tile info support here as well
	dio_buf[i] = (void *)malloc(maxwin + dio_mem);
	CHECK(dio_buf[i]);
	tile_buf[i] = (u_char *)malloc(maxtilesize*3);
	CHECK(tile_buf[i]);
	tile_offsets[i] = (u_int *)calloc(maxNumTiles, sizeof(u_int));
	CHECK(tile_offsets[i]);
	tile_info[i] = (tileOverlapInfo_t *)calloc(maxNumTiles , sizeof(tileOverlapInfo_t));
	CHECK(tile_info[i]);
      }
      else {
	dio_buf[i] = (void *)malloc(maxwin + dio_mem);
	CHECK(dio_buf[i]);
      }
      dio_free[i] = dio_buf[i];
      w = (unsigned long)dio_buf[i];
      if ((dio_mem) && (w & (dio_mem-1))) {
         w += dio_mem - (w & (dio_mem-1));
      }
      dio_buf[i] = (void *)w;
   }
}

void smBase::initWin(void)
{
   u_int i;

   if(getVersion() == 1) {
     for (i=0; i<nwin; i++) {
       readWin(i);
       currentFrame[i] = i;
     }
   }
}




void smBase::readWin(u_int f)
{
   u_int winnum;
   off64_t size;

   smdbprintf(5,"READWIN %d", f);

   winnum = f % nwin;

   size = flength[f];
   off64_t offset = (foffset[f] | (dio_min-1)) ^ (dio_min-1);

   smdbprintf(5,"actual offset is %lld", foffset[f]);
   smdbprintf(5,"dio offset is %lld", offset);
   smdbprintf(5,"memory offset is %lld", foffset[f] & (dio_min-1));
   if (LSEEK64(fd[winnum], offset, SEEK_SET) < 0)
     fprintf(stderr, "Error seeking to frame %d", f);
   win[winnum] = (u_char *)dio_buf[winnum] + (foffset[f] & (dio_min-1));

   // increase the size to account for the overhead at beginning of block
   size += (foffset[f] & (dio_min-1));

   if (size & (dio_min-1))
      size += dio_min - (size & (dio_min-1));


   int n;
   u_char *buf;

   for (n=size, buf=(u_char *)dio_buf[winnum]; n>0;) {
      int k=(n>dio_max?dio_max:n);
      int r=READ(fd[winnum], buf, k);
      smdbprintf(5,"reading %d bytes", k);
      if (r < 0) {
         if ((errno == EINTR) && (errno == EAGAIN)) {
	    char	s[40];
	    sprintf(s,"xmovie I/O error : r=%d k=%d ",r, k);
	    perror(s);
            exit(1);
         }
      }
      buf+=k;
      n-=k;
   }
}



void smBase::readWin(u_int f, int *dim, int* pos, int res )
{
  // version 2 implied -- reads in only overlapping tiles and doesn't use direct I/O
 
  u_int winnum;
  u_int readBufferOffset;
  u_char *cdata;
 
  smdbprintf(5,"readWin version 2, frame %d", f); 

  winnum = f % nwin;
  off64_t offset = foffset[f] ;

  if (LSEEK64(fd[winnum], offset, SEEK_SET) < 0) {
    fprintf(stderr, "Error seeking to frame %d\n", f);
    exit(1);
  }
  win[winnum] = (u_char *)dio_buf[winnum] ;

 
  assert(res < nresolutions);
  int nx = getTileNx(res);
  int ny = getTileNy(res);
  int numTiles =  nx * ny;

  // If numTiles > 1 then read in frame header of tile sizes

  if(numTiles > 1 ) {
    cdata = (u_char *)win[winnum];
    int k =  numTiles * sizeof(uint32_t);
    int r=READ(fd[winnum],cdata, k);
    
    if (r !=  k) {
      char	s[40];
      sprintf(s,"smBase::readWin I/O error : r=%d k=%d ",r,k);
      perror(s);
      exit(1);
    }
    uint32_t *header = (uint32_t*)cdata;
    uint32_t *toffsets = (uint32_t *)tile_offsets[winnum];
    tileOverlapInfo_t *tinfo = (tileOverlapInfo_t *)tile_info[winnum];
    readBufferOffset = k;
    // get tile sizes and offsets
    u_int sum = 0;
    int i=0;

    // reset frame in tileinfo insensitive to current MIPMAP -- i.e. maxNumTiles
    
    for(i=0 ; i < maxNumTiles; i++) {
      if(tinfo[i].frame != tinfo[0].frame)
	tinfo[i].frame = tinfo[0].frame;
    }

    // determine previous overlaps
    for(i = 0; i < numTiles; i++) { 
      tinfo[i].compressedSize = (u_int)ntohl(header[i]);
      toffsets[i] = k + sum;
      sum += tinfo[i].compressedSize;
      //smdbprintf(5,"tile[%d].frame = %d",i,tinfo[i].frame);
      if(tinfo[i].frame == f) {
	if(tinfo[i].overlaps) {
	  tinfo[i].prev_overlaps = 1;
	  readBufferOffset += tinfo[i].compressedSize;
	  //smdbprintf(5,"tile[%d] setting prev_overlaps : readBufferOffset = %d",i,readBufferOffset);
	}
	else {
	  if(tinfo[i].prev_overlaps) {
	    readBufferOffset += tinfo[i].compressedSize;
	    //smdbprintf(5,"tile[%d] prev_overlaps : readBufferOffset = %d",i,readBufferOffset);
	  }
	}
      }
      else {
	tinfo[i].frame = f;
	tinfo[i].prev_overlaps = 0;
	tinfo[i].overlaps = 0;
      }
    }
    computeTileOverlap(dim, pos, res, tinfo);
   
    // Grab data for overlapping tiles
    for(int tile = 0; tile < numTiles; tile++) {
      if(tinfo[tile].overlaps && (!tinfo[tile].prev_overlaps)) {
        //	smdbprintf(5,"tile %d newly overlaps",tile);
	tinfo[tile].readBufferOffset = readBufferOffset;
	tinfo[tile].skipCorruptFlag = 0;

	if (LSEEK64(fd[winnum], offset + toffsets[tile] , SEEK_SET) < 0) {
      fprintf(stderr, "Error seeking to frame %d at offset %Ld\n", f, offset + toffsets[tile]);
	      exit(1);
	    }
    //smdbprintf(5,"Reading %d bytes from file for winnum %d for tile %d", tinfo[tile].compressedSize, winnum, tile); 
	r=READ(fd[winnum],cdata+readBufferOffset,tinfo[tile].compressedSize);
    //smdbprintf(5,"Done reading"); 

	if (r != tinfo[tile].compressedSize ) {
	      fprintf(stderr,"smBase::readWin I/O error : r=%d k=%d : skipping\n",r,tinfo[tile].compressedSize);
	      tinfo[tile].skipCorruptFlag = 1;
	    }
	
	readBufferOffset += tinfo[tile].compressedSize;
      }
    }
  }
  smdbprintf(5,"Done with readWin for frame %d", f); 
  return; 
}

void smBase::computeTileOverlap(int *blockDim, int* blockPos, int res, tileOverlapInfo_t *info)
{
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
     smdbprintf(5,"Tileinfo pointer %p",info);
     smdbprintf(5,"Tiles %dx%d of %dx%d",nx,ny,tileWidth,tileHeight);
     smdbprintf(5,"Block Dim %dx%d at Position %d,%d",blockDim[0],blockDim[1],blockPos[0],blockPos[1]);
  */ 

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


      t1 = blockStartX - tileStartX;
      if( t1 >= 0) {
	if (t1 < tileWidth) {
	  //smdbprintf(5," t1=%d ",t1);
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
	//smdbprintf(5,"boffsetX %d boffsetY %d toffsetX %d toffsetY %d",info[ipIndex].blockOffsetX,info[ipIndex].blockOffsetY,info[ipIndex].tileOffsetX,info[ipIndex].tileOffsetY);
      }
      else {
	info[ipIndex].overlaps = 0;
      }

      
    } // for i
  } // for j
 

  
}

//-----------------------------------------------------------
// Core frame functions
//-----------------------------------------------------------

/* this is now just a wrapper for getFrameBlock() */
void smBase::getFrame(int f, void *data)
{
   getFrameBlock(f,data);
}

/* this function will "autores" the returned block based on "step" */
void smBase::getFrameBlock(int f, void *data,  int destRowStride, int *dim, int *pos, int *step, int res)
{
  smdbprintf(5,"smBase::getFrameBlock, frame %d", f); 
   u_char *cdata;
   u_int size;
   u_char *image;
   u_char *out = (u_char *)data;
   u_char *rowPtr = (u_char *)data;

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
     return;

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

   /*smdbprintf(5,"res %d tilesize[%d,%d] step[%d,%d] pos[%d,%d] dim[%d,%d]",_res,tilesize[0],tilesize[1],_step[0],_step[1],_pos[0],_pos[1],_dim[0],_dim[1]); */

   // tile support 
   int nx = 0;
   int ny = 0;
   int numTiles = 0;
   u_char *tbuf;
   tileOverlapInfo_t *tinfo;
    
   nx = getTileNx(_res);
   ny = getTileNy(_res);
   numTiles =  nx * ny;

   assert(numTiles < 1000);

   cdata = (u_char *)NULL;

   if((version == 2) && (numTiles > 1)) {
     cdata = (u_char *)lockFrame((int)_f, size, &_dim[0],&_pos[0],(int)_res);
     int winnum = _f % nwin;

     //smdbprintf(5,"winnum %d",winnum);

     tbuf = (u_char *)tile_buf[winnum];
     tinfo = (tileOverlapInfo_t *)tile_info[winnum];
   }
   else {
      cdata = (u_char *)lockFrame(_f, size);
   }

   //smdbprintf(5,"cdata %p",cdata);
   
   if(numTiles < 2) {
     smdbprintf(5,"Special case for *entire frame*"); 
     if ((_pos[0] == 0) && (_pos[1] == 0) && 
	 (_step[0] == 1) && (_step[1] == 1) &&
	 (_dim[0] == d[0]) && (_dim[1] == d[1])) {
       
       /*
         smdbprintf(5,"Calling decomp block for cdata %p out %p size %d dim[%d,%d]",cdata,out,size,d[0],d[1]);
       */
       
       decompBlock(cdata,out,size,d); 


     } else {
       image = (u_char *)malloc(3*d[0]*d[1]);
       CHECK(image);
       smdbprintf(5,"Calling decompBlock since numTiles < 2");
       decompBlock(cdata,image,size,d);
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
   }
   else { 
     smdbprintf(5,"smBase::getFrameBlock(%d): process across %d overlapping tiles", f, numTiles); 
     u_int copied=0;
     for(int tile=0; tile<numTiles; tile++){
       //smdbprintf(5,"tile %d", tile); 
      tileOverlapInfo_t tileinfo = tinfo[tile];
       if(tileinfo.overlaps && (tileinfo.skipCorruptFlag == 0)) {

	 u_char *tdata = (u_char *)(cdata + tileinfo.readBufferOffset);
	 decompBlock(tdata,tbuf,tileinfo.compressedSize,tilesize);
     //smdbprintf(5,"done with decompBlock, tile %d", tile); 

	 u_char *to = (u_char*)(out + (tileinfo.blockOffsetY * destRowStride) + (tileinfo.blockOffsetX * 3));
	 u_char *from = (u_char*)(tbuf + (tileinfo.tileOffsetY * tilesize[0] * 3) + (tileinfo.tileOffsetX * 3));
     int maxX = tileinfo.tileLengthX, maxY = tileinfo.tileLengthY; 
	 for(int rows = 0; rows < maxY; rows += _step[1]) {
	   if(_step[0] == 1) {
	     copied += maxX * 3;
#if 1
	     assert(copied <= (_dim[0]*_dim[1]*3));
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

   unlockFrame(_f);
   smdbprintf(5,"END smBase::getFrameBlock, frame %d", f); 
   return; 
 }


void smBase::setFrame(int f, void *data)
{
   int i,tile,size;
   int *sizes,numTiles,version;
   u_char *scaled0,*scaled1,*cdata;

   version = getVersion();
   numTiles = getTileNx(0)*getTileNy(0);


   // Set the zeroth resolution
   if((version == 1) || (numTiles == 1)) {
     compFrame(data,NULL,size,0);
     //smdbprintf(5,"Frame %d is size %d",f,size);
     cdata=(u_char *)malloc(size);
     CHECK(cdata);
     compFrame(data,cdata,size,0);
     setCompFrame(f,cdata,size,0);
     free(cdata);
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
     cdata=(u_char *)malloc(size); 
     CHECK(cdata);
     compFrame(data,cdata,sizes,0);
     setCompFrame(f,cdata,sizes,0);
     free(sizes);
     free(cdata);  
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
	 cdata=(u_char *)malloc(size);
	 CHECK(cdata);
	 compFrame(scaled1,cdata,size,i);
	 setCompFrame(f,cdata,size,i);
	 free(cdata);
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
	 cdata=(u_char *)malloc(size); 
	 CHECK(cdata);
	 compFrame(scaled1,cdata,sizes,i);
	 setCompFrame(f,cdata,sizes,i);
	 free(cdata);
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

void smBase::setCompFrame(int f, void *data, int size, int res)
{
   pthread_mutex_lock(&writelock);
   foffset[f+res*getNumFrames()] = LSEEK64(fd[0],0,SEEK_CUR);
   flength[f+res*getNumFrames()] = size;
   WRITE(fd[0], data, size);
   pthread_mutex_unlock(&writelock);

   bModFile = TRUE;
   return;
}


void smBase::setCompFrame(int f, void *data, int *sizes, int res)
{
  uint32_t tz;
  int size;
  int numTiles = getTileNx(res)*getTileNy(res);

  pthread_mutex_lock(&writelock);
  foffset[f+res*getNumFrames()] = LSEEK64(fd[0],0,SEEK_CUR);
 
  size = 0;
  // if frame is tiled then write tile offset (jump table) first
  if(numTiles > 1) {
    for(int i = 0; i < numTiles; i++) {
      tz =  htonl((uint32_t)sizes[i]);
      WRITE(fd[0],&tz,sizeof(uint32_t));
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
  WRITE(fd[0], data, size);
  pthread_mutex_unlock(&writelock);

  bModFile = TRUE;
  return;
}


// return the compressed frame (if data==NULL return the size)
void smBase::getCompFrame(int frame, void *data, int &rsize, int res)
{
   u_int size;
   void *cdata;

   cdata = lockFrame(frame+res*getNumFrames(), size);
   if (data) memcpy(data, cdata, size);
   rsize = (int)size;
   unlockFrame(frame+res*getNumFrames());

   return;
}
int smBase::getCompFrameSize(int frame, int res)
{
	return(flength[frame+res*getNumFrames()]);
}

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



void *smBase::lockFrame(u_int f, u_int &size)
{
   void *frame;
   int winnum = f % nwin;

   pthread_mutex_lock(&winmut[winnum]);

   smdbprintf(5,"locking frame %d for currentFrame[winnum] = %d\n", f,currentFrame[winnum]);

   if (currentFrame[winnum] != f) {
     smdbprintf(5,"blocking for window %d (%d)\n", winnum, f);
     while (winlock[winnum] > 0) {
#ifdef DISABLE_PTHREADS
       usleep(5000);
#else
       pthread_cond_wait(&wincond[winnum], &winmut[winnum]);
#endif
       // check to see if window index has changed
       if (currentFrame[winnum] == f)
         break;
     }
     //smdbprintf(5,"done waiting for winnum %d f %d\n",winnum,f);
      // maybe another thread already paged in the window
      if (currentFrame[winnum] != f) {
         readWin(f);
         currentFrame[winnum] = f;
      }
   }
   smdbprintf(5,"have lock w=%d\n", f);
   
   winlock[winnum]++;

   frame = (u_char *)win[winnum];
   size = flength[f];

   pthread_mutex_unlock(&winmut[winnum]);

   return(frame);
}



void *smBase::lockFrame(u_int f, u_int &size, int *dim, int* pos, int res)
{
   void *frame;
   int winnum = f % nwin;

   pthread_mutex_lock(&winmut[winnum]);

   if (winlock[winnum] > 0) smdbprintf(5,"have to wait for lock on window %d, frame %d", winnum, f);

   while (winlock[winnum] > 0) {
#ifdef DISABLE_PTHREADS
     usleep(5000);
#else
     pthread_cond_wait(&wincond[winnum], &winmut[winnum]);
#endif
     smdbprintf(5,"done waiting for lock on window %d, frame %d", winnum, f);
   }
   
   winlock[winnum]++;
   if (currentFrame[winnum] != f) {
     currentFrame[winnum] = f;
   }
   readWin(f,dim,pos,res);
   
   frame = (u_char *)win[winnum];
   size = flength[f];
   
   pthread_mutex_unlock(&winmut[winnum]);
   smdbprintf(5,"Done with lockFrame for frame %d", f);
   return(frame);
}


void smBase::unlockFrame(u_int f)
{
   int winnum = f % nwin;

   pthread_mutex_lock(&winmut[winnum]);

   winlock[winnum]--;

 
   if(version < 2) {
     if (winlock[winnum] == 0)
       pthread_cond_broadcast(&wincond[winnum]);
   }
   else {
     if (winlock[winnum] == 0)
       pthread_cond_signal(&wincond[winnum]);
   }
  pthread_mutex_unlock(&winmut[winnum]);

}

void smBase::closeFile(void)
{
   u_int arr[64] = {0};

   if (bModFile == TRUE) {
	int i;

   	LSEEK64(fd[0], 0, SEEK_SET);

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
   	WRITE(fd[0], arr, sizeof(u_int)*SM_HDR_SIZE);

   	byteswap(foffset,sizeof(off64_t)*nframes*nresolutions,sizeof(off64_t));
   	WRITE(fd[0], foffset, sizeof(off64_t)*nframes*nresolutions);
   	byteswap(foffset,sizeof(off64_t)*nframes*nresolutions,sizeof(off64_t));

   	byteswap(flength,sizeof(u_int)*nframes*nresolutions,sizeof(u_int));
   	WRITE(fd[0], flength, sizeof(u_int)*nframes*nresolutions);
   	byteswap(flength,sizeof(u_int)*nframes*nresolutions,sizeof(u_int));

	//smdbprintf(5,"seek header end is %d\n",LSEEK64(fd[0], 0, SEEK_CUR));
	CLOSE(fd[0]);
   }
	
}

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

static void smoothx(unsigned char *image, int dx, int dy)
{
        register int x,y;
	int	p1[3],p2[3],p3[3];

/* smooth along X scanlines using 242 kernel */
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

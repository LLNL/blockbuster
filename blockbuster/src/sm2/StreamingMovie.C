#include "StreamingMovie.h"
#include "iodefines.h"
#include "SMJpegCodec.h" 
#include "SMGZCodec.h" 

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include <vector>
using namespace std; 

int smVerbose = 0; 
void smSetVerbose(int level) {
  smVerbose = level;
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


/*
  SwizzleTileIntoFrame()
  Copy data from the internal mRawTileBuf to the given cimg object, converting from RGB interleaved to block data to make CImg library happy. 
  param tilenum -- which tile to insert 
  param cimg -- the output vehicle to modify and return, which also defines the size of the frame region of interest implicitly with its size
  param frameOffset -- start X and Y of CImg into the frame

  return false if fails
*/ 
bool StreamingMovie::SwizzleTileIntoCImg(uint32_t tilenum, CImg<unsigned char> &cimg, uint32_t cimgFrameOffset[2]){
  
  int32_t tileIJ[2] = { tilenum % mTileNxNy[mLOD][0], 
			tilenum / mTileNxNy[mLOD][0] }; 
  int32_t tilePosInFrame[2] = { tileIJ[0]*mTileSizes[mLOD][0], 
				tileIJ[1]*mTileSizes[mLOD][1] };
  // may be negative: 
  int32_t tileOffsetInCImg[2] = { tilePosInFrame[0] - cimgFrameOffset[0], 
			       tilePosInFrame[1] - cimgFrameOffset[1] }; 
  int32_t starti=0, startj=0,
    stopi = mTileSizes[mLOD][0], stopj = mTileSizes[mLOD][1];
  if (tileOffsetInCImg[0] < 0) starti = -tileOffsetInCImg[0];
  if (tileOffsetInCImg[1] < 0) startj = -tileOffsetInCImg[1];
  if (stopi + tileOffsetInCImg[0] > cimg.width()) {
    stopi =  cimg.width() - tileOffsetInCImg[0]; 
  }
  if (stopj + tileOffsetInCImg[1] > cimg.height()) {
    stopj =  cimg.height() - tileOffsetInCImg[1]; 
  }
  int32_t cimgstarti = tileOffsetInCImg[0] + starti, 
    cimgstartj = tileOffsetInCImg[1] + startj; 
  unsigned char *cimgbuf = cimg.data(); 
  uint32_t isize = cimg.size(); 
  unsigned char *cimgbufrp, *cimgbufgp, *cimgbufbp; 
  unsigned char *tilebufp; 
  int32_t tilei=starti, tilej = startj, 
    cimagei= cimgstarti, cimagej = cimgstartj;
  
  while (tilej < stopj) {
    tilei = starti; 
    cimagei = cimgstarti; 
    // cimgbufrp = cimgbuf + tilej * cimg.width(); 
    // cimgbufgp = cimgbufrp + isize; 
    // cimgbufbp = cimgbufgp + isize; 
    tilebufp = &mRawTileBuf[0] + 3*(mTileSizes[mLOD][0]*tilej + tilei); 
    while (tilei++ < stopi) { // deinterleave: 
      cimg(cimagei, cimagej, 0, 0) = *tilebufp; tilebufp++; 
      cimg(cimagei, cimagej, 0, 1) = *tilebufp; tilebufp++; 
      cimg(cimagei, cimagej, 0, 2) = *tilebufp; tilebufp++; 
      cimagei++; 
      /*
       *cimgbufrp =  *tilebufp;  cimgbufrp++; tilebufp++; 
       *cimgbufgp =  *tilebufp;  cimgbufgp++; tilebufp++; 
       *cimgbufbp =  *tilebufp;  cimgbufbp++; tilebufp++;    
       */ 
    }    
    tilej++; 
    cimagej++; 
  }
  return true; 
}

/*
  FetchFrame()
  param framenum -- which frame to fetch
  param cimg -- the output vehicle to modify and return

  return value false if it failes, true on success
*/ 
// TO DO:  deal with concurrency and shared_ptrs when threading
// TO DO:  how do I choose the right buffer size? 
bool StreamingMovie::FetchFrame(uint32_t framenum, CImg<unsigned char> &cimg) {
  int lfd = open(mFileName.c_str(), O_RDONLY);
  
  // seek to frame beginning
  uint32_t virtualFrame = mLOD * mNumFrames + framenum;
  uint64_t pos = LSEEK64(lfd, mFrameOffsets[virtualFrame], SEEK_SET);
  //cerr << "Current pos is "<< pos << endl; 
  // read compressed tile sizes
  int numTiles = mTileNxNy[mLOD][0] * mTileNxNy[mLOD][1];
  vector<uint32_t> tilesizes(numTiles); 
  if (numTiles > 1) {
    read(lfd, &tilesizes[0], numTiles*sizeof(uint32_t));     
    byteswap(&tilesizes[0], numTiles*sizeof(uint32_t), sizeof(uint32_t)); 
    int i=0; 
    pos = 0; 
    while (i<numTiles) {
      pos += tilesizes[i++]; 
    }
    // cerr << "Total tile sizes is "<< pos << endl; 
  } else {
    tilesizes[0] = mFrameLengths[virtualFrame];
  }
  //read the whole frame off disk: 
  pos =  LSEEK64(lfd, 0, SEEK_CUR);
  //ssmdbprintf("After tilesizes, pos is %d\n", pos); 
  pos = read(lfd, &mFrameReadBuffer[0], mFrameLengths[virtualFrame]);   
  //cerr << "Read " << pos << " bytes from frame and mFrameLengths[virtualFrame] is " << mFrameLengths[virtualFrame] << endl; 
  // decompress each tile into the image:
  if (mRawTileBuf.size() < mMaxTileSize) {
    mRawTileBuf.resize(mMaxTileSize*3); 
  }
  uint32_t tilenum = 0, tileOffset = 0; 
  pos=  LSEEK64(lfd, 0, SEEK_CUR);
  //cerr << "After frame, pos is "<< pos << endl; 


  uint32_t cimgFrameOffset[2]={0};
  while (tilenum < numTiles) {  
    mCodec->Decompress(&mFrameReadBuffer[tileOffset], tilesizes[tilenum], mRawTileBuf, mTileSizes[mLOD]); 
    SwizzleTileIntoCImg(tilenum, cimg, cimgFrameOffset); 
    tileOffset += tilesizes[tilenum++];
  }

  cimg.mirror('y'); 
  
  return false; 
}

/*
  ReadHeader()
  Read the Streaming Movie file header and save the information
*/ 
bool StreamingMovie::ReadHeader(void) {
   uint32_t magic, flags;

   //uint32_t maxtilesize;

   int i, w;
   int lfd = open(mFileName.c_str(), O_RDONLY);

   // the file size is needed for the size of the last frame
   mFileSize = LSEEK64(lfd,0,SEEK_END);

   // read a version 2 header, which has version 1 as a subset
   uint32_t header[SM_HDR_SIZE];
   LSEEK64(lfd,0,SEEK_SET);
   read(lfd, &magic, sizeof(uint32_t));   
   mRawMagic = magic = ntohl(magic);
  
   LSEEK64(lfd,0,SEEK_SET);
   if (magic == SM_MAGIC_VERSION1) {
     mVersion = 1;
     read(lfd, header, sizeof(uint32_t)*5);
     byteswap(header, sizeof(uint32_t)*5, sizeof(uint32_t));
   }     
   if (magic == SM_MAGIC_VERSION2) {
     mVersion = 2;
     read(lfd, header, sizeof(uint32_t)*SM_HDR_SIZE);
     byteswap(header, sizeof(uint32_t)*SM_HDR_SIZE, sizeof(uint32_t));
   }   
   // read(lfd, &flags, sizeof(uint32_t));
   flags = header[1];
   mRawFlags = flags & SM_FLAGS_MASK;
   mCompressionType = flags & SM_COMPRESSION_TYPE_MASK;
   switch (mCompressionType) {
   case 2:
     mCodec = new SMGZCodec(); 
     break; 
   case 4:
     mCodec = new SMJpegCodec(); 
     break; 
   case 0: 
   case 1:
   case 3:
   default:
     cerr << "Unimplemented compression type: "<< mCompressionType << endl;
     exit(1); 
   }
   // read(lfd, &mNumFrames, sizeof(uint32_t));
   mNumFrames = header[2]; 
   mNumFrames = mNumFrames;
   smdbprintf(4,"open file, mNumFrames = %d\n", mNumFrames);
   
   // read(lfd, &i, sizeof(uint32_t));   
   mFrameSizes[0][0] = header[3];
   // read(lfd, &i, sizeof(uint32_t));
   mFrameSizes[0][1] = header[4];
   for(i=1;i<8;i++) {
     mFrameSizes[i][0] = mFrameSizes[i-1][0]/2;
     mFrameSizes[i][1] = mFrameSizes[i-1][1]/2;
   }
   memcpy(mTileSizes,mFrameSizes,sizeof(mFrameSizes));
   mNumResolutions = 1;
   
   smdbprintf(4,"image size: %d %d\n", mFrameSizes[0][0], mFrameSizes[0][1]);
   smdbprintf(4,"mNumFrames=%d\n",mNumFrames);
   
   // Version 2 header is bigger...
   if (mVersion == 2) {
     mMaxNumTiles = 0;
     mNumResolutions = header[5];
     //smdbprintf(5,"mNumResolutions : %d",mNumResolutions);
     for(i=0;i<mNumResolutions;i++) {
       mTileSizes[i][1] = (header[6+i] & 0xffff0000) >> 16;
       mTileSizes[i][0] = (header[6+i] & 0x0000ffff);
       mTileNxNy[i][0] = (uint32_t)ceil((double)mFrameSizes[i][0]/(double)mTileSizes[i][0]);
       mTileNxNy[i][1] = (uint32_t)ceil((double)mFrameSizes[i][1]/(double)mTileSizes[i][1]);
       
       if(mMaxNumTiles < (mTileNxNy[i][0] * mTileNxNy[i][1])) {
	 mMaxNumTiles = mTileNxNy[i][0] * mTileNxNy[i][1];
       }
       smdbprintf(5,"mTileNxNy[%ld,%ld] : maxnumtiles %ld\n", mTileNxNy[i][0], mTileNxNy[i][1],mMaxNumTiles);
     }
     mMaxTileSize = mTileSizes[0][1] * mTileSizes[0][0];;
     smdbprintf(5,"mMaxTileSize = %ld, maxnumtiles = %ld\n",mMaxTileSize,mMaxNumTiles);
     
   }
   else {
     // SM Version 1 -- initialize tile info to reasonable defaults
     for (i = 0; i < mNumResolutions; i++) {
       mTileSizes[i][0] = mFrameSizes[i][0];
       mTileSizes[i][1] = mFrameSizes[i][1];
       mTileNxNy[i][0] = 1;
       mTileNxNy[i][1] = 1;
     }
   }
   
   // Get the framestart offsets
   mFrameOffsets.resize(mNumResolutions*mNumFrames+1); 
   // it's ok to read into a std::vector exactly as an array per the standard:
   read(lfd, &mFrameOffsets[0], sizeof(off64_t)*mNumFrames*mNumResolutions);
   byteswap(&mFrameOffsets[0],sizeof(off64_t)*mNumFrames*mNumResolutions,sizeof(off64_t));
   mFrameOffsets[mNumFrames*mNumResolutions] = mFileSize;

   // Get the compressed frame lengths...
   mFrameLengths.resize(mNumFrames*mNumResolutions, 0); 
   mMaxCompressedFrameSize = 0; //maximum frame size
   if (mVersion == 2) {
     read(lfd, &mFrameLengths[0], sizeof(uint32_t)*mNumFrames*mNumResolutions);
     byteswap(&mFrameLengths[0],sizeof(uint32_t)*mNumFrames*mNumResolutions,sizeof(uint32_t));
     for (w=0; w<mNumFrames*mNumResolutions; w++) {
       if (mFrameLengths[w] > mMaxCompressedFrameSize) mMaxCompressedFrameSize = mFrameLengths[w];
     }
   } else {
     // Compute this for older format files...
     for (w=0; w<mNumFrames*mNumResolutions; w++) {
	   mFrameLengths[w] = (mFrameOffsets[w+1] - mFrameOffsets[w]);
       if (mFrameLengths[w] > mMaxCompressedFrameSize)  mMaxCompressedFrameSize = mFrameLengths[w];
     }
   }
   if ( mFrameReadBuffer.size() < mMaxCompressedFrameSize + 2) {
     mFrameReadBuffer.resize(mMaxCompressedFrameSize + 2);
   }

#if SM_VERBOSE
   for (w=0; w<mNumFrames; w++) {
     smdbprintf(5,"window %d: %d size %d\n", w, (int)mFrameOffsets[w],mFrameLengths[w]);
   }
#endif
   smdbprintf(4,"maximum compressed frame size is %d\n", mMaxCompressedFrameSize);
   
   CLOSE(lfd);

   
   // bump up the size to the next multiple of the DIO requirements
   /* maxFrameSize += 2;
      for (i=0; i<mThreadData.size(); i++) {
      unsigned long w;
      if(mVersion == 2) {
      // put preallocated tilebufs and tile info support here as well
      mThreadData[i].io_buf.clear(); 
      mThreadData[i].io_buf.resize(maxFrameSize, 0);
      mThreadData[i].tile_buf.clear();
      mThreadData[i].tile_buf.resize(mMaxTileSize*3, 0);
      mThreadData[i].tile_infos.clear(); 
      mThreadData[i].tile_infos.resize(mMaxNumTiles);
      }
      }
   */
   return true; 
}

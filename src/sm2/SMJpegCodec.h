#ifndef SM_Jpeg_DECOMPRESSOR_H
#define SM_Jpeg_DECOMPRESSOR_H
#include "SMCodec.h"
#include "setjmp.h"
#include <stdio.h>
extern "C" {
#include "jpeglib.h"
#include "jerror.h"
}

/*
  This seems overdesigned, but for some decompressors it probably needs a whole file to talk about compressing and decompressing. 
  There is no internal buffering to keep things threadsafe. 
*/ 

#define gzdbprintf fprintf(stderr, "%s line %d: ", __FILE__, __LINE__); fprintf

class SMJpegCodec:public SMCodec {
 public:
  SMJpegCodec():mQuality(75) {
    return; 
  }
  
  ~SMJpegCodec() {
    return; 
  }

  void setQuality(int qual) { mQuality = qual; 
    if (mQuality > 100) mQuality = 100; 
    if (mQuality < 0) mQuality = 0; 
  }

  bool Compress(unsigned char *in, uint32_t inputSize, std::vector<unsigned char> &out, uint32_t blockDims[2]);

  bool Decompress(unsigned char *in, uint32_t inputSize, std::vector<unsigned char> &out, uint32_t blockDims[2]);

  int mQuality; 
}; 


#endif

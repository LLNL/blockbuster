#ifndef SM_GZ_DECOMPRESSOR_H
#define SM_GZ_DECOMPRESSOR_H
#include "SMCodec.h"

/*
  This seems overdesigned, but for some decompressors it probably needs a whole file to talk about compressing and decompressing. 
  There is no internal buffering to keep things threadsafe. 
*/ 


class SMGZCodec:public SMCodec {
  SMGZCodec() {
    return; 
  }
  
  ~SMGZCodec() {
    return; 
  }


  void Compress(void *in, void *out, uint32_t inputSize, uint32_t outputSize) {
    return;
  }

  void Decompress(void *in, void *out, uint32_t inputSize) {
    return;
  }

}; 


#endif

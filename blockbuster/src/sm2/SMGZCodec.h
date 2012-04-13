#ifndef SM_GZ_DECOMPRESSOR_H
#define SM_GZ_DECOMPRESSOR_H
#include "SMCodec.h"

/*
  This seems overdesigned, but for some decompressors it probably needs a whole file to talk about compressing and decompressing. 
  There is no internal buffering to keep things threadsafe. 
*/ 


class SMGZCodec:public SMCodec {
 public:
  SMGZCodec() {
    return; 
  }
  
  ~SMGZCodec() {
    return; 
  }


  void Compress(void *in, uint32_t inputSize, std::vector<unsigned char> &out) {
    return;
  }

  void Decompress(void *in, uint32_t inputSize, std::vector<unsigned char> &out) { 
    return;
  }

}; 


#endif

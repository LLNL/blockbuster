#ifndef SM_DECOMPRESSOR_H
#define SM_DECOMPRESSOR_H
#include <vector>
#include <stdint.h>
/*
  This seems overdesigned, but for some decompressors it probably needs a whole file to talk about compressing and decompressing. 
  There is no internal buffering to keep things threadsafe. 
*/ 


class SMCodec {
 public:
  SMCodec() { // no polymorphic construction; it's ok, I don't need it.
    return; 
  }

  virtual ~SMCodec() { // Native support for polymorphic destruction.
    return; 
  }

  virtual bool Compress(unsigned char *in, uint32_t inputSize, std::vector<unsigned char> &out, uint32_t blockDims[2])=0;

  virtual bool Decompress(unsigned char *in, uint32_t inputSize, std::vector<unsigned char> &out, uint32_t blockDims[2])=0;


};

#endif

#ifndef SM_DECOMPRESSOR_H
#define SM_DECOMPRESSOR_H
#include <vector>
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

  virtual void Compress(void *in, uint32_t inputSize, std::vector<unsigned char> &out)=0;

  virtual void Decompress(void *in, uint32_t inputSize, std::vector<unsigned char> &out)=0;


};

#endif

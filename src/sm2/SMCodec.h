#ifndef SM_DECOMPRESSOR_H
#define SM_DECOMPRESSOR_H

/*
  This seems overdesigned, but for some decompressors it probably needs a whole file to talk about compressing and decompressing. 
  There is no internal buffering to keep things threadsafe. 
*/ 


class SMCodec {
 protected:
  SMCodec() { // no polymorphic construction; it's ok, I don't need it.
    return; 
  }

  virtual ~SMCodec() { // Native support for polymorphic destruction.
    return; 
  }

  virtual void Compress(void *in, void *out, uint32_t inputSize, uint32_t outputSize)=0;

  virtual void Decompress(void *in, void *out, uint32_t inputSize)=0;
}; 


#endif

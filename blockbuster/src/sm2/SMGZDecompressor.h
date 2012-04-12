#ifndef SM_GZ_DECOMPRESSOR_H
#define SM_GZ_DECOMPRESSOR_H

/*
  This seems overdesigned, but for some decompressors it probably needs a whole file to talk about compressing and decompressing. 
  There is no internal buffering to keep things threadsafe. 
*/ 

class SMGZDecompressor {
  SMGZDecompressor() {
    return; 
  }
  ~SMGZDecompressor() {
    return; 
  }

  void DecompressData(void *in, void *out, uint32_t inputSize) {
    return;
  }
  void CompressData(void *in, void *out, uint32_t inputSize, uint32_t outputSize) {
    return;
  }

}; 


#endif

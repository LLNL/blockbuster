#ifndef SM_GZ_DECOMPRESSOR_H
#define SM_GZ_DECOMPRESSOR_H
#include "SMCodec.h"
#include "zlib.h"

/*
  This seems overdesigned, but for some decompressors it probably needs a whole file to talk about compressing and decompressing. 
  There is no internal buffering to keep things threadsafe. 
*/ 

#define gzdbprintf fprintf(stderr, "%s line %d: ", __FILE__, __LINE__); fprintf

class SMGZCodec:public SMCodec {
 public:
  SMGZCodec() {
    return; 
  }
  
  ~SMGZCodec() {
    return; 
  }


  bool Compress(unsigned char *in, uint32_t inputSize, std::vector<unsigned char> &out, uint32_t blockDims[2]) {
    return false;    
  }

  bool Decompress(unsigned char *in, uint32_t inputSize, std::vector<unsigned char> &out, uint32_t /*ignore*/[2]) { 
    int 	err;
    z_stream  stream;
    stream.zalloc = (alloc_func)0;
    stream.zfree = (free_func)0;
    stream.opaque = (voidpf)0;
    
    stream.next_in = in;
    stream.avail_in = inputSize;
    stream.next_out = &out[0];
    stream.avail_out = out.size();
    err = inflateInit(&stream);
   if (err != Z_OK) {
     gzdbprintf(stderr,"GZ decompression init error: %d (%s)\n",err, stream.msg);
      return false;
   }

   while(err == Z_OK) {
      err = inflate(&stream, Z_NO_FLUSH);
   }
   if (err != Z_STREAM_END) {
     gzdbprintf(stderr,"GZ decompression error: %d (%s)\n",err, stream.msg);
     return false;
   }
   err = inflateEnd(&stream);
   if (err != Z_OK) {
     gzdbprintf(stderr,"GZ decompression end error: %d (%s)\n",err, stream.msg);
      return false;
   }


   return true;
    
  }

}; 


#endif

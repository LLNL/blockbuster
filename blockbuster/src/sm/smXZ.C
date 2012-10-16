/*
** $RCSfile: smXZ.C,v $
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
** $Id: smXZ.C,v 1.5 2009/05/19 02:52:19 wealthychef Exp $
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
// smXZ.C
//
//
//
//

#include <stdlib.h>
#include "smXZ.h"
#include "lzma.h"

#define lzmadbprintf fprintf(stderr, "%s line %d: ", __FILE__, __LINE__); fprintf

u_int smXZ::typeID = 5;

smXZ::smXZ(const char *_fname, int _nwin)
  :smBase(_fname, _nwin)
{
}

smXZ::~smXZ()
{
}

void smXZ::init(void)
{
   smBase::registerType(typeID, create);
}

smBase *smXZ::create(const char *_fname, int _nwin)
{
   return(new smXZ(_fname, _nwin));
}

bool smXZ::decompBlock(u_char *in,u_char *out,int size,int *dim)
{
   int  dlen = dim[0]*dim[1]*sizeof(u_char[3]);
   lzma_stream  stream = LZMA_STREAM_INIT;
   lzma_ret err = lzma_stream_decoder(&stream, UINT64_MAX, LZMA_CONCATENATED);
   if (err != LZMA_OK) {
     lzmadbprintf(stderr,"XZ decompression init error: %d\n",err);
      return false;
   }
   
   /*   stream.zalloc = (alloc_func)0;
   stream.zfree = (free_func)0;
   stream.opaque = (voidpf)0;
   */ 
   stream.next_in = in;
   stream.avail_in = size;
   stream.next_out = out;
   stream.avail_out = dlen;


   while(err == LZMA_OK) {
      err = lzma_code(&stream, LZMA_RUN);
   }
   if (err != LZMA_STREAM_END) {
     lzmadbprintf(stderr,"XZ decompression error: %d\n",err);
      return false;
   }

   lzma_end(&stream);

   return true;
}

smXZ *smXZ::newFile(const char *_fname, 
                    u_int _width, u_int _height, u_int _nframes, 
                    u_int *_tile, u_int _nres, 
                    int numthreads)
{
   smXZ *r = new smXZ(NULL);

   if (r->smBase::newFile(_fname, _width, _height, _nframes, _tile, _nres, numthreads)) {
	delete r;
	r = NULL;
   }
 
   return(r);
}

void smXZ::compBlock(void *in, void *out, int &size,int *dim)
{
  size = 0; 
   lzma_stream  stream = LZMA_STREAM_INIT;
   lzma_ret err = lzma_easy_encoder(&stream, 9, LZMA_CHECK_CRC64);
   if (err != LZMA_OK) {
     lzmadbprintf(stderr,"XZ compression init error: %d\n",err);
      return ;
   }

   long len = dim[0]*dim[1]*sizeof(u_char[3]);
   long dlen = ((float)len * 1.1)+12; // estimate output length

   stream.next_in = (const uint8_t*)in;
   stream.avail_in = len;
   stream.next_out = ( uint8_t*)out;
   stream.avail_out = dlen;

   while(err == LZMA_OK) {
      err = lzma_code(&stream, LZMA_RUN);
   }
   if (err != LZMA_STREAM_END) {
     lzmadbprintf(stderr,"XZ decompression error: %d\n",err);
      return ;
   }

   size = stream.avail_out;

   lzma_end(&stream);

   return ;

}

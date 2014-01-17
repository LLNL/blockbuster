/*
** $RCSfile: smLZO.C,v $
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
** $Id: smLZO.C,v 1.5 2009/05/19 02:52:19 wealthychef Exp $
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
// smLZO.C
//
//
//
//

#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include "smLZO.h"
#include "lzo1x.h"



#define CHECK(v) \
if(v == NULL) \
lzo_fail_check(__FILE__,__LINE__)

void lzo_fail_check(const char *file,int line)
{
  perror("fail_check");
smdbprintf(0,"Failed at line %d in file %s\n",line,file);
  exit(1); 
}




smLZO::smLZO(int mode, const char *_fname, int _nwin)
  :smBase(mode, _fname, _nwin)
{
  mTypeID = 3;
  if (lzo_init() != LZO_E_OK) {
  smdbprintf(0,"Warning: Unable to initialize LZO library\n");
  }
  eCompressionOpt = LZO_OPT_1;
  if (getenv("LZO_COMPRESSION_OPTION")) {
    if (!strcmp(getenv("LZO_COMPRESSION_OPTION"),
                "999")) eCompressionOpt = LZO_OPT_999;
  }
}

smLZO::~smLZO()
{
}


bool smLZO::decompBlock(u_char *cdata,u_char *image,int size,int *dim)
{
   lzo_uint dlen;
   int  status;

   dlen = dim[0]*dim[1]*sizeof(u_char[3]);
   status = lzo1x_decompress((const lzo_byte *)cdata,size,
		   (lzo_byte *)image,&dlen,NULL);
   if (status != LZO_E_OK) {
	smdbprintf(0,"LZO decompression error: %d\n",status);
    return false; 
   }

   return true;
}

/*smLZO *smLZO::newFile(const char *_fname, 
                      u_int _width, u_int _height, u_int _nframes, 
                      u_int *_tile, u_int _nres, 
                      int numthreads)
{
   smLZO *r = new smLZO(NULL);

   if (r->smBase::newFile(_fname, _width, _height, _nframes, _tile, _nres, numthreads)) {
	delete r;
	r = NULL;
   }
 
   return(r);
   } */ 

void smLZO::compBlock(void *data, void *cdata, int &size,int *dim)
{
   int status;
   lzo_uint dlen,len;
   lzo_byte *wrkmem;

   len = dim[0]*dim[1]*sizeof(u_char[3]);
   /* actually: i + (i/64) + 16 + 3 = 1.015i+19 */
   dlen = (lzo_uint)(len * 1.1)+19;

   if (cdata) {
       if (eCompressionOpt == LZO_OPT_999) {
   	   wrkmem = (lzo_byte *)malloc(LZO1X_999_MEM_COMPRESS);
	   CHECK(wrkmem);
   	   status = lzo1x_999_compress((const lzo_byte *)data,len,
		   (lzo_byte *)cdata,&dlen,wrkmem);
       } else {
   	   wrkmem = (lzo_byte *)malloc(LZO1X_1_MEM_COMPRESS);
	   CHECK(wrkmem);
   	   status = lzo1x_1_compress((const lzo_byte *)data,len,
		   (lzo_byte *)cdata,&dlen,wrkmem);
       }
       free(wrkmem);
       if (status != LZO_E_OK) {
	 smdbprintf(0,"LZO compression error: %d\n",status);
       }
   }
   size = dlen;
}

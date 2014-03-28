/*
** $RCSfile: smGZ.C,v $
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
** $Id: smGZ.C,v 1.5 2009/05/19 02:52:19 wealthychef Exp $
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
// smGZ.C
//
//
//
//

#include <stdlib.h>
#include "smGZ.h"
#include "zlib.h"

// #define smdbprintf smdbprintf(0, "%s line %d: ", __FILE__, __LINE__); fprintf
#define smcerr if (0) cerr 

smGZ::smGZ(int mode, const char *_fname, int _nwin)
  :smBase(mode, _fname, _nwin)
{
  mTypeID = 2;
}

smGZ::~smGZ()
{
}

bool smGZ::decompBlock(u_char *cdata,u_char *image,int size,int *dim)
{
  smcerr << "smGZ::decompBlock("<<size<<", ["<<dim[0]<<", "<<dim[1]<<"])" << endl;
   int 	err;
   int  dlen;
   z_stream  stream;

   dlen = dim[0]*dim[1]*sizeof(u_char[3]);

   stream.zalloc = (alloc_func)0;
   stream.zfree = (free_func)0;
   stream.opaque = (voidpf)0;

   stream.next_in = cdata;
   stream.avail_in = size;
   stream.next_out = image;
   stream.avail_out = dlen;

   err = inflateInit(&stream);
   if (err != Z_OK) {
     smdbprintf(0,"GZ decompression init error: %d (%s)\n",err,stream.msg);
      return false;
   }

   while(err == Z_OK) {
      err = inflate(&stream, Z_NO_FLUSH);
   }
   if (err != Z_STREAM_END) {
     smdbprintf(0,"GZ decompression error: %d (%s)\n",err,stream.msg);
      return false;
   }

   err = inflateEnd(&stream);
   if (err != Z_OK) {
      smdbprintf(0,"GZ decompression end error: %d (%s)\n",err,stream.msg);
      return false;
   }
   smcerr << "decoded " << stream.total_out << " bytes" << endl; 

   return true;
}

/*smGZ *smGZ::newFile(const char *_fname, 
                    u_int _width, u_int _height, u_int _nframes, 
                    u_int *_tile, u_int _nres, 
                    int numthreads)
{
   smGZ *r = new smGZ(NULL);

   if (r->smBase::newFile(_fname, _width, _height, _nframes, _tile, _nres, numthreads)) {
	delete r;
	r = NULL;
   }
 
   return(r);
}
*/
void smGZ::compBlock(void *data, void *cdata, int &size,int *dim)
{
  smcerr << "smGZ::compBlock("<<data<<", "<<cdata<<", "<<size<<", ["<<dim[0]<<", "<<dim[1]<<"])" << endl;
  int status;
   uLongf dlen,len;

   len = dim[0]*dim[1]*sizeof(u_char[3]);
   dlen = (len * 1.1)+12; // estimate length
   if (cdata) {
       status = compress((Bytef *)cdata,&dlen,(Bytef *)data,len);
       if (status != Z_OK) {
       smdbprintf(0,"GZ compression error: %d\n",status);
         exit(1); 
       }
   }
   smcerr << "insize = "<<len<<", outsize = " << dlen<<endl<<endl;
   size = dlen;
}

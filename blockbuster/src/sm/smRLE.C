/*
** $RCSfile: smRLE.C,v $
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
** $Id: smRLE.C,v 1.6 2009/05/19 02:52:19 wealthychef Exp $
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
// smRLE.C
//
//
//
//

#include <stdlib.h>
#include "smRLE.h"

smRLE::smRLE(int mode, const char *_fname, int _nwin)
  : smBase(mode, _fname, _nwin)
{
  mTypeID = 1;
}

smRLE::~smRLE()
{
}


bool smRLE::decompBlock(u_char *cdata,u_char *image,int,int *dim)
{
  int i,n;
  int npix = 0;
  smdbprintf(1,"decompressing rle\n");
  while (npix < dim[0]*dim[1]) {
    
    n = *cdata++;
    //smdbprintf(6,"npix %d : n %d\n",npix,n);
    if (npix + n > getWidth()*getHeight()) 
      {
      smdbprintf(0, "smRLE: rle code error\n");
        n = getWidth()*getHeight() - npix;
      }
    for (i=0; i<n; i++) 
      {
        *image++ = cdata[0];
        *image++ = cdata[1];
        *image++ = cdata[2];
      }
    cdata += 3;
    npix += n;
  }
  return true; 
}


/*smRLE *
smRLE::newFile(const char *_fname, u_int _width, u_int _height,
               u_int _nframes, u_int *_tiles,u_int _nres, 
               int numthreads)
{
   smRLE *r = new smRLE(NULL);

   if (r->smBase::newFile(_fname, _width, _height, _nframes, _tiles, _nres, numthreads)) {
	delete r;
	r = NULL;
   }
 
   return(r);
}
*/
void
smRLE::compBlock(void *data, void *cdata, int &size,int *dim)
{
   if (!cdata) {
       size = (int)(dim[0]*dim[1]*3*1.4); /* 4 instead of 3 is the worst case */
       return;
   }

   u_char *image = (u_char *)data;
   u_char *cimage = (u_char *)cdata;
   u_char red, green, blue;
   int npix;
   int run;

   size = 0;
   npix = 0;

   while (npix < dim[0]*dim[1]) {
      run = 1;
      red = image[0];
      green = image[1];
      blue = image[2];
      image += 3;
      while ((run < 255) && (npix+run < dim[0]*dim[1]) &&
             red==image[0] && green==image[1] && blue==image[2]) {
         image += 3;
         run++;
      }
      cimage[size++] = run;
      cimage[size++] = red;
      cimage[size++] = green;
      cimage[size++] = blue;
      npix += run;
//printf("run: %d\n", run);
   }
//printf("frame size %d\n", size);
}

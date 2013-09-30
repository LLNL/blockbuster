/*
** $RCSfile: smRaw.C,v $
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
** $Id: smRaw.C,v 1.4 2009/05/19 02:52:19 wealthychef Exp $
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
// smRAW.C
//
//
//
//

#include <stdio.h>
#include "smRaw.h"

smRaw::smRaw(int mode, const char *_fname, int _nwin)
  :smBase(mode, _fname, _nwin)
{
  mTypeID = 0; 
}

smRaw::~smRaw()
{
}

/* smRaw *smRaw::newFile(const char *_fname, u_int _width,  u_int _height,
                      u_int _nframes, u_int *_tile, u_int _nres, 
                      int numthreads)
{  
   smRaw *r = new smRaw(NULL);

   if (r->smBase::newFile(_fname, _width, _height, _nframes, _tile, _nres, numthreads)) {
	delete r;
	r = NULL;
   }
 
   return(r);
   } */ 

void smRaw::compBlock(void *data, void *cdata, int &size,int *dim)
{
   size = sizeof(u_char[3])*dim[0]*dim[1];
   if (cdata) memcpy(cdata, data, size);
}

bool smRaw::decompBlock(u_char *cdata,u_char *image,int size,int *dim)
{
   memcpy(image,cdata,size);
   return true; 
}

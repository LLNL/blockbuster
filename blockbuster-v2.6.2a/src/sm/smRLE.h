/*
** $RCSfile: smRLE.h,v $
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
** $Id: smRLE.h,v 1.4 2009/05/12 17:25:14 wealthychef Exp $
**
*/
/*
**
**  Abstract:
**
**  Author:
**
*/


#ifndef _SM_RLE_H
#define _SM_RLE_H

//
// smRLE.h - class for "streamed rle movies"
//
//
//
//

#include <sys/types.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include "smBase.h"

class smRLE : public smBase {
   public:
      smRLE(const char *fname, int _nwin=1);
      virtual ~smRLE();

      static smRLE *newFile(const char *fname, u_int nframes, u_int w, u_int h,
		      u_int *tilesizes = NULL, u_int nres=1);

      static void init(void);

      int getType(void) { return typeID; }

   protected:
      static smBase *create(const char *_fname, int _nwin);
      void compBlock(void *, void *, int&, int *dim);
      void decompBlock(u_char *,u_char *,int, int *dim);

   private:
      static u_int typeID;
};

#endif

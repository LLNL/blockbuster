/*
** $RCSfile: smLZO.h,v $
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
** $Id: smLZO.h,v 1.4 2009/05/12 17:25:14 wealthychef Exp $
**
*/
/*
**
**  Abstract:
**
**  Author:
**
*/


#ifndef _SM_LZO_H
#define _SM_LZO_H

//
// smLZO.h - class for "streamed lzo movies"
//
//
//
//

#include <sys/types.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include "smBase.h"

enum eLZOCompOption { LZO_OPT_1, LZO_OPT_999};

class smLZO : public smBase {
   public:
      smLZO(int mode, const char *fname, int _nwin=1);
      virtual ~smLZO();

      smLZO(const char *_fname, 
                      u_int _width, u_int _height, 
                      u_int _nframes, u_int *_tile = NULL, u_int _nres = 1, 
                      int numthreads=1) : 
        smBase(_fname, _width, _height, _nframes, _tile, _nres, numthreads) {
        mTypeID = 3;
      }

      static void init(void);


   protected:
      void compBlock(void *, void *, int&, int *dim);
      bool decompBlock(u_char *cdata,u_char *image,int size,int *dim);

   private:
      eLZOCompOption    eCompressionOpt;
};

#endif

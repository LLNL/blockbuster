/*
** $RCSfile: smBaseP.h,v $
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
** $Id: smBaseP.h,v 1.4 2007/10/05 22:59:27 wealthychef Exp $
**
*/
/*
**
**  Abstract:
**
**  Author:
**
*/


#ifndef _SM_BASEP_H
#define _SM_BASEP_H

//
// smBaseP.h - private, internal cross platform header/defines
//


// Assume all platforms have 64bit seeks
#define HAS_LSEEK64

#ifdef irix
#define SM_DIO
// Version 2 turns this off
#undef SM_DIO
#endif

// Linux w/o _LFS_LARGEFILE does not have lseek64()
// http://ftp.sas.com/standards/large.file/x_open.20Mar96.html
//
#ifdef linux
#ifndef _LFS_LARGEFILE
#undef HAS_LSEEK64
/* #warning "**********************************************"
   #warning "64bit file support disabled, no _LFS_LARGEFILE"
   #warning "**********************************************"*/
#endif
#endif

// Tru64 does not have (need?) lseek64()
#ifdef __alpha
#undef HAS_LSEEK64
#endif
// mac os x does not have (need?) lseek64()
#ifdef __APPLE__
#undef HAS_LSEEK64
#endif

#ifdef WIN32

#include <io.h>
#define OPEN(P,F) _open((P),(F)|_O_BINARY)
#define OPENC(P,F,M) _open((P),(F)|_O_BINARY,(M))
#define CLOSE _close
#define READ _read
#define WRITE _write
#define LSEEK64 _lseeki64
#define strdup _strdup
#define uint32_t unsigned int

#else

/*#define OPEN(P,F) smdfs_open((P),(F),0)
#define OPENC(P,F,M) smdfs_open((P),(F),(M))
#define CLOSE(I) smdfs_close((I))
#define LSEEK64(I,O,W) smdfs_lseek64((I),(O),(W))
#define READ(I,B,S) smdfs_read((I),(B),(S))
#define WRITE(I,B,S) smdfs_write((I),(B),(S))
*/

// #ifdef NEVER
#define OPEN open
#define OPENC open
#define CLOSE close
#define READ read
#define WRITE write
#ifdef HAS_LSEEK64
#define LSEEK64 lseek64
#else
#define LSEEK64 lseek
#endif
// #endif


#endif


#endif

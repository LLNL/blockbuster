/*
** $RCSfile: smdfcP.h,v $
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
** 	http://www.llnl.gov/icc/sdd/img/viz.shtml
**
**      or contact: asciviz@llnl.gov
**
** For copyright and disclaimer information see:
**      $(ASCIVIS_ROOT)/copyright_notice_1.txt
**
** 	or man llnl_copyright
**
** $Id: smdfcP.h,v 1.4 2007/10/05 22:59:27 wealthychef Exp $
**
*/
/*
**
**  Abstract:
**
**  SM distributed memory file cache.  This system
**  provides a simple read/write/load interface to
**  a virtual "file".  The virtual file is dristibuted
**  using MPI over a number of node.  The file currently
**  is striped over the MPI nodes.
**  The system is bootstrapped via an MPI job that creates
**  a memory buffer on each node.  It provides a local
**  socket interface on each node that accepts application
**  level requests, allowing the smdfs MPI job to be used
**  with non-MPI based applications.  Commands are sent 
**  by applications to the local sockets and broadcast
**  to all the nodes for resolution, at which point,
**  the results are pushed back over the socket to the
**  calling application.
**
**  Author: rjf, LLNL
**
*/

#ifndef __SMDFSP_H__
#define __SMDFSP_H__

#include <pthread.h>

#include <sys/types.h>
#ifndef WIN32
#include <unistd.h>
#else
#define off64_t __int64
#endif

#ifndef off64_t
#define off64_t int64_t
#endif

/* Assume all platforms have 64bit seeks */
#define HAS_LSEEK64

/* Linux w/o _LFS_LARGEFILE does not have lseek64()
   http://ftp.sas.com/standards/large.file/x_open.20Mar96.html
*/
#ifdef linux
#ifndef _LFS_LARGEFILE
#undef HAS_LSEEK64
#warning "**********************************************"
#warning "64bit file support disabled, no _LFS_LARGEFILE"
#warning "**********************************************"
#endif
#endif

/* Tru64 does not have (need?) lseek64() */
#if defined(__alpha) || defined(osx)
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
#define STAT _stati64
#define LSEEK64 _lseeki64
#define strdup _strdup
#define uint32_t unsigned int

#else

#ifdef linux
#define OPEN open64
#define OPENC open64
#define STAT stat64
#else
#define OPEN open
#define OPENC open
#define STAT stat
#endif

#define CLOSE close
#define READ read
#define WRITE write

#ifdef HAS_LSEEK64
#define LSEEK64 lseek64
#else
#define LSEEK64 lseek
#endif

#endif

typedef struct _dfcfd {
	struct _dfcfd *next;
	int	socket;
	off64_t offset;
	off64_t size;
	int	swab;
	off64_t	max_size;
} dfcfd;

#define DFC_CMD_OPEN	1
#define DFC_CMD_CLOSE	2
#define DFC_CMD_READ	3
#define DFC_CMD_WRITE	4
#define DFC_CMD_STAT	5
#define DFC_CMD_KILL	6

/* from smdfc.c */
int parsedfcname(const char *in,char *host,int *port,char *key);
dfcfd *isdfcfd(int fd);
dfcfd *new_fd(char *host,int port);
void free_fd(dfcfd *t);
int send_command(dfcfd *p,int cmd,unsigned int cnt,void *buf,int flg);
int sock_read(int s,int num, int size, char *buffer);
int sock_write(int s,int num,int size,  char *buffer);
void byteswap(void *buffer,long int len,long int swapsize);

#include "smdfc.h"

#endif


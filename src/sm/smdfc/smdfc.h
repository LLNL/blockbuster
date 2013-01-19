/*
** $RCSfile: smdfc.h,v $
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
** $Id: smdfc.h,v 1.3 2007/06/13 19:00:31 wealthychef Exp $
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
**  Mods: Rich Cook
**
*/

#ifndef __SMDFS_H__
#define __SMDFS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#if defined(__APPLE__) && defined(__MACH__)
#include <sys/dtrace.h>
#endif

#ifndef WIN32
#include <unistd.h>
#endif

int smdfs_open(const char *path, int flags, int mode);
off64_t smdfs_lseek64(int fd, off64_t offset, int whence);
int smdfs_read(int fd, void *buf, unsigned int count);
int smdfs_write(int fd, void *buf, unsigned int count);
int smdfs_close(int fd);
int smdfs_kill(int fd);

#ifdef __cplusplus
}
#endif

#endif

/*
** $RCSfile: smservP.h,v $
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
** $Id: smservP.h,v 1.3 2007/06/13 19:00:32 wealthychef Exp $
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

#ifndef __SMSERVP_H__
#define __SMSERVP_H__

#include "smdfcP.h"

#include <mpi.h>

#define DFC_STATUS_PENDING      1
#define DFC_STATUS_WORKING      2
#define DFC_STATUS_COMPLETE     3
                                                                                
#define CMD_TAG 	0x12345678
#define BASE_PORT	13466

typedef struct {
	int 		command;
	unsigned int	length;
	off64_t		location;
	int		srcrank;
	int		sock_tag;
} basecmd;

typedef struct {
	int		status;
	MPI_Request	cmd_req;
	MPI_Request	data_req;
} cmd_rply;

typedef struct _dfccmd {
	struct _dfccmd 	*next;

	basecmd		cmd;

	unsigned char 	*buffer;
	int		status;
	pthread_cond_t	wait_cond;

	cmd_rply	*work;
} dfccmd;

#endif


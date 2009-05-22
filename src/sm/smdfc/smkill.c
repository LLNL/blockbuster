/*
** $RCSfile: smkill.c,v $
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
**      http://www.llnl.gov/icc/sdd/img/viz.shtml
**
**      or contact: asciviz@llnl.gov
**
** For copyright and disclaimer information see:
**      $(ASCIVIS_ROOT)/copyright_notice_1.txt
**
**      or man llnl_copyright
**
** $Id: smkill.c,v 1.3 2007/06/13 19:00:32 wealthychef Exp $
**
*/
/*
**
**  Abstract:  tool to issues a dfc server shutdown command.
**
**  Author: rjf
**
*/
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef WIN32
#include <unistd.h>
#else
#define off64_t __int64
#endif
                                                                                
#ifndef off64_t
#define off64_t int64_t
#endif

#include "smdfc.h"

int main(int argc, char **argv)
{
	int	fd;

	if (argc != 2) {
		fprintf(stderr,"(%s) usage: %s dfcfilename\n",__DATE__,argv[0]);
		exit(1);
	}

	fd = smdfs_open(argv[1],O_RDONLY,0);
	if (fd == -1) {
		fprintf(stderr,"Unable to open dfc file: %s\n",argv[1]);
		exit(1);
	}
	if (smdfs_kill(fd) == -1) {
		fprintf(stderr,"Unable to kill dfc server: %s\n",argv[1]);
		smdfs_close(fd);
	}

	exit(0);
}


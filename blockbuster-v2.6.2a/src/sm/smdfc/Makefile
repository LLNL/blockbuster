# ASCI Visualization Project
#
# Lawrence Livermore National Laboratory
# Information Management and Graphics Group
# P.O. Box 808, Mail Stop L-561
# Livermore, CA 94551-0808
#
# For information about this project see:
#       http://www.llnl.gov/sccd/lc/img/
#
#       or contact: asciviz@llnl.gov
#
# For copyright and disclaimer information see:
#       $(ASCIVIS_ROOT)/copyright_notice_1.txt
#
# $Id: Makefile,v 1.4 2007/06/13 19:00:30 wealthychef Exp $
#
#   Abstract:
#
#   Author:
#
#
#       Makefile for the smdfc Library (which is built into the sm library)
#
#       $Id: Makefile,v 1.4 2007/06/13 19:00:30 wealthychef Exp $
#
include ../../config/config

CLIENT = smdfc.o
LIBS = -L. -lsmdfcclient -lpthread

all: smserv dfctest smkill

dfctest: libsmdfcclient.a dfctest.o
	$(CC) $(CFLAGS) -o $@ dfctest.o $(LIBS)

smkill: libsmdfcclient.a smkill.o
	$(CC) $(CFLAGS) -o $@ smkill.o $(LIBS)

smserv: smserv.c libsmdfcclient.a
	$(MPICC) $(MPI_INCS) $(CFLAGS) smserv.c -o $@ $(LIBS) $(MPI_LIBS)

libsmdfcclient.a : $(CLIENT)
	$(AR) rsc $@ $(CLIENT)

clean:
	rm -f *.a *.o smserv dfctest smkill

tar:
	rm -f smdfc.tgz
	tar cvzf smdfc.tgz *.[ch] Makefile

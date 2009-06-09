#
# $RCSfile: Makefile,v $
# $Name:  $
# 
# ASCI Visualization Project 
#
# Lawrence Livermore National Laboratory
# Information Management and Graphics Group
# P.O. Box 808, Mail Stop L-561
# Livermore, CA 94551-0808
#
# For information about this project see:
# 	http://www.llnl.gov/sccd/lc/img/ 
#
# 	or contact: asciviz@llnl.gov
#
# For copyright and disclaimer information see:
#	$(ASCIVIS_ROOT)/copyright_notice_1.txt
#
# $Id: Makefile,v 1.1 2007/06/13 18:59:35 wealthychef Exp $
# 
#   Abstract:
#
#   Author:
#
#
#	Makefile for sndplay lib
#
#       $Id: Makefile,v 1.1 2007/06/13 18:59:35 wealthychef Exp $
#
include ../config/config

FILES = Makefile sndplay.c stubmain.c test.c pop.aiff laser.aiff fire.aiff \
	sndplay.h sndplay_p.h

OFILES = sndplay.o
HFILES = sndplay.h
LIBS = -L. -lsndplay $(AUDIO_LIBS) $(PTHREADS_LIBS) -lm -lpthread

all: $(INSTALL_DIR)/lib/libsndplay.a stubsndplay test

$(INSTALL_DIR)/lib/libsndplay.a: libsndplay.a
	$(call MAKE-INSTALL-DIRS)
	cp $^ $(INSTALL_DIR)/lib
	cp $(HFILES) $(INSTALL_DIR)/include/sndplay

libsndplay.a : $(OFILES)
	$(AR) -rsc $@ $(OFILES)

stubsndplay : libsndplay.a stubmain.o
	$(CC) $(CFLAGS) stubmain.o -o $@ $(LIBS)

test : libsndplay.a test.o
	$(CC) $(CFLAGS) test.o -o $@ $(LIBS)

clean: 
	rm -rf $(CC_CACHE) *.o *.a stubsndplay test core
	rm -rf $(INSTALL_DIR)/lib/libsndplay.a $(INSTALL_DIR)/include/sndplay


tar : $(FILES)
	rm -f sndplay.tar sndplay.tar.gz
	tar cvf sndplay.tar $(FILES)
	gzip sndplay.tar

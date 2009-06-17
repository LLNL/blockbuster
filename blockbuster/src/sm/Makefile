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
# $Id: Makefile,v 1.10 2009/05/12 17:48:36 wealthychef Exp $
# 
#   Abstract:
#
#   Author:
#
#
#	Makefile for the sm Library
#
#       $Id: Makefile,v 1.10 2009/05/12 17:48:36 wealthychef Exp $
#
include ../config/config

SRC_FILES = smBase.C smRLE.C smRaw.C smGZ.C smLZO.C smJPG.C smdfc/smdfc.c
OFILES = smBase.o smRLE.o smRaw.o smGZ.o smLZO.o smJPG.o smdfc.o
HFILES = smBase.h smRLE.h smRaw.h smGZ.h smLZO.h smJPG.h

CPPFLAGS += -g
all: lib doc include

lib: $(INSTALL_DIR)/lib/libsmovie.a

$(INSTALL_DIR)/lib/libsmovie.a:  libsmovie.a 
	$(call MAKE-INSTALL-DIRS)
	cp $^ $(INSTALL_DIR)/lib

doc: $(INSTALL_DIR)/man/man4/sm.4

$(INSTALL_DIR)/man/man4/sm.4:
	mkdir -p $(INSTALL_DIR)/man/man{3,4}
	cp *.3 $(INSTALL_DIR)/man/man3/
	cp *.4 $(INSTALL_DIR)/man/man4/

include: $(INSTALL_DIR)/include/smBase.h

$(INSTALL_DIR)/include/smBase.h: $(HFILES)
	mkdir -p $(INSTALL_DIR)/include/sm
	cp $(HFILES) $(INSTALL_DIR)/include/sm


$(OFILES): $(HFILES) $(SRC_FILES)

libsmovie.a: $(OFILES)
	$(AR) rsc $@ $(OFILES)
	if [ $(HAS_MPI) ]; then \
		cd smdfc; $(MAKE); \
	fi

smdfc.o: smdfc/smdfc.c smdfc/smdfcP.h
	$(CC) $(CFLAGS) -Ismdfc -c smdfc/smdfc.c
	$(CXX) $(CFLAGS) -Ismdfc -c smdfc/smdfc.c

clean: 
	rm -rf $(CC_CACHE) *.o core *.a
	cd smdfc; $(MAKE) clean
	rm -rf $(INSTALL_DIR)/lib/libsmovie.a $(INSTALL_DIR)/sm

depend:
	makedepend $(INCLUDES) $(SRC_FILES)

# DO NOT DELETE THIS LINE -- make depend depends on it.

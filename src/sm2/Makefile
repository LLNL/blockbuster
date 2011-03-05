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

SRC_FILES = sm2.C # smRLE.C smRaw.C smGZ.C smLZO.C smJPG.C 
OFILES = sm2.o # smRLE.o smRaw.o smGZ.o smLZO.o smJPG.o 
HFILES = sm2.h # smRLE.h smRaw.h smGZ.h smLZO.h smJPG.h

CPPFLAGS += -g
all: lib doc include

lib: $(INSTALL_DIR)/lib/libsm2.a

$(INSTALL_DIR)/lib/libsm2.a:  libsm2.a 
	$(call MAKE-INSTALL-DIRS)
	cp $^ $(INSTALL_DIR)/lib

doc: 

include: $(INSTALL_DIR)/include/sm2.h

$(INSTALL_DIR)/include/sm2.h: $(HFILES)
	mkdir -p $(INSTALL_DIR)/include/sm2
	cp $(HFILES) $(INSTALL_DIR)/include/sm2


#$(OFILES): $(HFILES) $(SRC_FILES)

libsm2.a: $(OFILES)
	$(AR) rsc $@ $(OFILES)


clean: 
	rm -rf $(CC_CACHE) *.o core *.a
	rm -rf $(INSTALL_DIR)/lib/libsm2.a $(INSTALL_DIR)/sm2

#use with caution:
depend: 
	makedepend -Y. -Y.. $(INCLUDES) $(SRC_FILES)

# DO NOT DELETE THIS LINE -- make depend depends on it.


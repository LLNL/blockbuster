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
#	$(INSTALL_DIR)/copyright_notice_1.txt
#
# $Id: Makefile,v 1.1 2007/06/13 18:59:29 wealthychef Exp $
# 
#   Abstract:
#
#   Author:
#
#
#	Makefile for the libimage Library
#
#	$Id: Makefile,v 1.1 2007/06/13 18:59:29 wealthychef Exp $
#
include ../config/config

#OFILES = sgilib.o 
OFILES = sgilib.o open.o close.o filbuf.o flsbuf.o name.o pix.o rdwr.o rle.o row.o
HFILES = $(INSTALL_DIR)/include/libimage/sgilib.h
ARCHIVES = $(INSTALL_DIR)/lib/libimage.a $(INSTALL_DIR)/lib/libFL.a

all: $(ARCHIVES) $(HFILES)

$(INSTALL_DIR)/include/libimage/%.h: %.h
	mkdir -p $(INSTALL_DIR)/include/libimage
	cp $^ $@

$(INSTALL_DIR)/lib/%.a: %.a
	cp $^ $@

libFL.a : dummy.o
	$(AR) rsc $@ dummy.o
	cp $@ $(INSTALL_DIR)/lib

libimage.a: $(OFILES)
	$(AR) rsc $@ $(OFILES)

clean:
	rm -rf $(CC_CACHE) $(OFILES) libimage.a dummy.o libFL.a 
	rm -rf $(ARCHIVES) $(INSTALL_DIR)/include/libimage
	makedepend

depend:
	makedepend $(INCLUDES) *.c

# DO NOT DELETE THIS LINE -- make depend depends on it.

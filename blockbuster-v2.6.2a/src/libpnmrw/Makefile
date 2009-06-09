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
# $Id: Makefile,v 1.1 2007/06/13 18:59:31 wealthychef Exp $
# 
#   Abstract:  Makefile for the libpnmrw.a library
#
#   Author: rjf
#
#
#       $Id: Makefile,v 1.1 2007/06/13 18:59:31 wealthychef Exp $
#
include ../config/config

SRC_FILES = libpnmrw.c
OFILES = libpnmrw.o
HFILES = libpnmrw.h

default: $(INSTALL_DIR)/lib/libpnmrw.a $(INSTALL_DIR)/include/libpnmrw/libpnmrw.h

libpnmrw.a: $(OFILES)
	$(AR) rsc $@ $(OFILES)


$(INSTALL_DIR)/include/libpnmrw/%.h: %.h
	mkdir -p $(INSTALL_DIR)/include/libpnmrw && cp $^ $@

$(INSTALL_DIR)/lib/%.a: %.a
	cp $^ $@


clean:
	rm -rf $(CC_CACHE) $(OFILES) libpnmrw.a
	rm -rf $(INSTALL_DIR)/lib/libpnmrw.a $(INSTALL_DIR)/include/libpnmrw

depend:
	makedepend $(INCLUDES) $(SRC_FILES)

# DO NOT DELETE THIS LINE -- make depend depends on it.

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
#	Makefile for patchelf, a utility for setting rpaths
#
#	$Id: Makefile,v 1.1 2007/06/13 18:59:35 wealthychef Exp $
#
include ../config/config

all: $(INSTALL_DIR)/bin/patchelf

$(INSTALL_DIR)/bin/patchelf:
	[ -d  patchelf-0.5 ] || tar -xzf patchelf-0.5.tar.gz
	cd  patchelf-0.5; \
	CC="$(CC)" CFLAGS="$(CFLAGS)" ./configure --prefix=$(INSTALL_DIR); \
		$(MAKE) install

clean:
	rm -rf patchelf-0.5 $(INSTALL_DIR)/bin/patchelf

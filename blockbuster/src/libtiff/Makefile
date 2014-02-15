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
# $Id: Makefile,v 1.2 2009/01/07 19:22:19 wealthychef Exp $
# 
#   Abstract:
#
#   Author: Rich Cook
#
#	Makefile for libtiff 4.0.3
#
#	$Id: Makefile,v 1.2 2009/01/07 19:22:19 wealthychef Exp $
#

TIFF_VERSION=4.0.3

include ../config/config

INSTALL_DIR ?= $(shell pwd)

default: install

all: install


install: $(INSTALL_DIR)/lib/libtiff.a

$(INSTALL_DIR)/lib/libtiff.a: tiff-$(TIFF_VERSION)/libtiff/.libs/libtiff.a
	cp $< $@

tiff-$(TIFF_VERSION)/libtiff/.libs/libtiff.a:
	rm -rf tiff-$(TIFF_VERSION)
	tar -xvzf tiff-$(TIFF_VERSION).tar.gz
	cd tiff-$(TIFF_VERSION) && CC="$(CC)" CFLAGS="$$(echo $(CFLAGS) |sed s/O2/O3/g) -O3" \
	 CXXFLAGS="$${CFLAGS}" LDFLAGS="$${LDFLAGS} -L/usr/lib64" \
	./configure --prefix=$(INSTALL_DIR) --enable-rpath --disable-jbig  ; \
		$(MAKE) -j 20 install


clean:
	rm -rf tiff-$(TIFF_VERSION) $(INSTALL_DIR)/lib/libtiff* $(INSTALL_DIR)/include/tiff*.h


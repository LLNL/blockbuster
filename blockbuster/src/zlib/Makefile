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
#	Makefile for zlib (1.1.3)
#
#	$Id: Makefile,v 1.1 2007/06/13 18:59:35 wealthychef Exp $
#
ZLIB_VERSION=1.2.8

include ../config/config

all: $(INSTALL_DIR)/lib/libz.a

$(INSTALL_DIR)/lib/libz.a:
	[ -d zlib-$(ZLIB_VERSION) ] || tar -xvzf zlib-$(ZLIB_VERSION).tar.gz
	cd zlib-$(ZLIB_VERSION) ; \
	CC="$(CC)" CFLAGS="$(CFLAGS)" ./configure --prefix=$(INSTALL_DIR) --shared; \
		$(MAKE) install

clean:
	rm -rf zlib-1.{2.8,1.3} $(INSTALL_DIR)/lib/libz.{a,so}.*

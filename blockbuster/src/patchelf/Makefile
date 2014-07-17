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

PATCHELF_VERSION=0.8

include ../config/config

all: $(INSTALL_DIR)/bin/patchelf

$(INSTALL_DIR)/bin/patchelf: patchelf-$(PATCHELF_VERSION)/src/patchelf
	cp $< $@

patchelf-$(PATCHELF_VERSION)/src/patchelf:
	[ -d  patchelf-$(PATCHELF_VERSION) ] || tar -xzf patchelf-$(PATCHELF_VERSION).tar.gz
	cd  patchelf-$(PATCHELF_VERSION); \
	CC="$(CC)" CFLAGS="$(CFLAGS)" ./configure --prefix=$(INSTALL_DIR); \
		$(MAKE) install

clean:
	rm -rf patchelf-$(PATCHELF_VERSION) $(INSTALL_DIR)/bin/patchelf

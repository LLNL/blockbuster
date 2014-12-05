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
# $Id: Makefile,v 1.3 2009/05/14 02:31:33 wealthychef Exp $
# 
#   Abstract:
#
#   Author: Rich Cook
#
#	Makefile for Exiv2 from http://www.exiv2.org
#   Used for metadata in jpegs.   
#
#	$Id: Makefile,v 1.3 2009/05/14 02:31:33 wealthychef Exp $
#
include ../config/config

default: install

INSTALL_DIR ?= $(shell pwd)


all: install

FORCE: 

TARGET= $(INSTALL_DIR)/lib/libexiv2.so.13.0.0
install: $(TARGET)

FORCE: 

$(TARGET): 
	tar -xzf exiv2-0.24.tar.gz
	cd exiv2-0.24 && CC="$(CC)" CFLAGS="$(CFLAGS)" \
	./configure --prefix=$(INSTALL_DIR) --disable-xmp && \
	$(MAKE) -j 20 install 


clean:
	rm -rf exiv2-0.24  $(TARGET)

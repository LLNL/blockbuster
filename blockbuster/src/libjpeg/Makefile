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
#	Makefile for libjpeg 9a 
#
#	$Id: Makefile,v 1.3 2009/05/14 02:31:33 wealthychef Exp $
#
include ../config/config

default: install

INSTALL_DIR ?= $(shell pwd)

DIRS = man/man1 man/man2 man/man3 man/man4 man/man5 lib bin include share

all: install

FORCE: 

install: $(INSTALL_DIR)/lib/libjpeg.a

FORCE: 

$(INSTALL_DIR)/lib/libjpeg.a: 
	mkdir -p $(DIRS)
	tar -xzf jpegsrc.v9a.t*gz
	cd jpeg-9a && CC="$(CC)" CFLAGS="$(CFLAGS)" \
	./configure --prefix=$(INSTALL_DIR) && \
	$(MAKE) -j 20 install 


clean:
	rm -rf jpeg-9a  $(INSTALL_DIR)/lib/libjpeg.a 

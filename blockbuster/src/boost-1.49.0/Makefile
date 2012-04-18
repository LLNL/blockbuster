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
#	Makefile for libtiff 3.9.1
#
#	$Id: Makefile,v 1.2 2009/01/07 19:22:19 wealthychef Exp $
#
include ../config/config

INSTALL_DIR ?= $(shell pwd)

INSTALL_EVIDENCE = $(INSTALL_DIR)/include/boost/random.hpp # random header :-)

default: install

all: install

install: $(INSTALL_EVIDENCE)

$(INSTALL_EVIDENCE): boost_1_49_0/boost/random.hpp
	echo apparently $< is older than $@; ls -l $<; ls -l $@
	cd boost_1_49_0 && ./bootstrap.sh --with-libraries=thread && ./bjam install --prefix=$(INSTALL_DIR)

boost_1_49_0/boost/random.hpp:
	rm -rf boost_1_49_0
	tar -xvf boost_1_49_0.tar.gz
#	cp -r boost_1_49_0/boost $(INSTALL_DIR)/include

clean:
	rm -rf boost_1_49_0 $(INSTALL_DIR)/lib/libboost* $(INSTALL_DIR)/include/boost


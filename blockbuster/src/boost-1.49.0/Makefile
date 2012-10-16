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

HEADER_EVIDENCE = $(INSTALL_DIR)/include/boost/random.hpp 
LIB_EVIDENCE = $(INSTALL_DIR)/lib/libboost_thread.a # random header :-)

all: lib

header: $(HEADER_EVIDENCE)

$(HEADER_EVIDENCE): boost_1_49_0/boost/random.hpp
	cp -r boost_1_49_0/boost $(INSTALL_DIR)/include/

lib: $(LIB_EVIDENCE)

$(LIB_EVIDENCE): boost_1_49_0/boost/random.hpp
	-echo apparently $< is older than $@; ls -l $<; ls -l $@
# export INSTALL_DIR=$(INSTALL_DIR); if ! find_boost.sh; then cd boost_1_49_0 && ./bootstrap.sh --with-libraries=thread && ./bjam install --prefix=$(INSTALL_DIR); fi
	cd boost_1_49_0 && ./bootstrap.sh --with-libraries=thread && ./bjam -j 20 install --prefix=$(INSTALL_DIR)

boost_1_49_0/boost/random.hpp:
	tar -xzf boost_1_49_0.tar.gz 

clean:
	rm -rf boost_1_49_0 $(INSTALL_DIR)/lib/libboost* $(INSTALL_DIR)/include/boost


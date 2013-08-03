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

BOOST_VERSION = 1_53_0
INSTALL_DIR ?= $(shell pwd)

HEADER_EVIDENCE = $(INSTALL_DIR)/include/boost/random.hpp 
LIB_EVIDENCE = $(INSTALL_DIR)/lib/libboost_thread.a  $(INSTALL_DIR)/lib/libboost_system.a  $(INSTALL_DIR)/lib/libboost_regex.a $(INSTALL_DIR)/lib/libboost_date_time.a
UNTAR_BOOST = boost_$(BOOST_VERSION)/boost/random.hpp

all: $(LIB_EVIDENCE)

boost_$(BOOST_VERSION)/boost/random.hpp: 
	@echo creating $(UNTAR_BOOST)
	tar -xzf boost_$(BOOST_VERSION).tar.gz 
	[ -f $@ ] && touch $@
	@echo '********************' finished with UNTAR_BOOST $@; echo

$(INSTALL_DIR)/include/boost/random.hpp: boost_$(BOOST_VERSION)/boost/random.hpp
	-echo apparently $< is newer than $@; ls -l $<; ls -l $@
	cp -r boost_$(BOOST_VERSION)/boost $(INSTALL_DIR)/include/
	@echo '********************' finished with HEADER_EVIDENCE $@; echo

$(LIB_EVIDENCE): $(INSTALL_DIR)/include/boost/random.hpp
	-echo apparently $< is newer than $@; ls -l $<; ls -l $@
	rm -f $@  # ensure all libs are newer than headers when done
	cd boost_$(BOOST_VERSION) && tar -xzf ../process-0.5.tgz && ./bootstrap.sh --with-libraries=thread,regex,system,date_time && ./bjam -j 20 install --prefix=$(INSTALL_DIR)
	@echo '********************' finished with LIB_EVIDENCE $@; echo

clean:
	rm -rf boost_$(BOOST_VERSION) $(INSTALL_DIR)/lib/libboost* $(INSTALL_DIR)/include/boost

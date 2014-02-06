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

BOOST_VERSION = 1_54_0
INSTALL_DIR ?= $(shell pwd)
BOOST_DIR ?= $(shell findboost.sh)

all: 
	echo $(BOOST_DIR) >  BOOSTDIR


build_boost: $(INSTALL_DIR)/include/boost/random.hpp

$(INSTALL_DIR)/include/boost/random.hpp: boost_$(BOOST_VERSION)/boost/random.hpp
	rm -f $@  # ensure all libs are newer than headers when done
	cd boost_$(BOOST_VERSION) && tar -xzf ../process-0.5.tgz && ./bootstrap.sh --with-libraries=atomic,filesystem,thread,regex,system,date_time && ./bjam -j 20 install --prefix=$(INSTALL_DIR)
	touch $@
	@echo '********************' finished with LIB_EVIDENCE $@; echo

boost_$(BOOST_VERSION)/boost/random.hpp: 
	if [ ! -f $@ ]; then \
		tar -xzf boost_$(BOOST_VERSION).tar.gz ; \
	fi	
	[ -f $@ ] && touch $@; 
	@echo '********************' finished with UNTAR_BOOST $@; echo

clean:
	rm -rf boost_$(BOOST_VERSION) $(INSTALL_DIR)/lib/libboost* $(INSTALL_DIR)/include/boost


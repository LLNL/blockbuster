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
#	$(INSTALL_DIR)/copyright_notice_1.txt
#
# $Id: Makefile,v 1.2 2009/05/14 02:34:08 wealthychef Exp $
# 
#   Abstract:
#
#   Author:
#
#	Makefile for zlib (1.1.3)
#
#	$Id: Makefile,v 1.2 2009/05/14 02:34:08 wealthychef Exp $
#
# 
include ../config/config

LZODIR = lzo-2.03
INSTALL_LIB = $(INSTALL_DIR)/lib/liblzo2.a

all: $(INSTALL_LIB)



$(INSTALL_LIB): $(LZODIR)/src/.libs/liblzo2.a
	cd $(LZODIR) ; 	export CC=$(CC); ./configure --prefix=$(INSTALL_DIR); $(MAKE) -j 20 install

$(LZODIR)/src/.libs/liblzo2.a: 
	tar -xvzf  $(LZODIR).tar.gz ; cd $(LZODIR) ; export CC=$(CC); ./configure --prefix=$(INSTALL_DIR); $(MAKE) -j 20 install; 

clean:
	rm -rf $(LZODIR) 
	rm -f $(INSTALL_DIR)/lib/liblzo.a $(INSTALL_DIR)/include/lzo*.h

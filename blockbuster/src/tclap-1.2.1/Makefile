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
# $Id: Makefile,v 1.9 2007/11/07 01:31:00 wealthychef Exp $
# 
#   Abstract:
#
#   Author:
#
#	Makefile for tclap (1.2.1), a command line parser
#
#	$Id: Makefile,v 1.9 2007/11/07 01:31:00 wealthychef Exp $
#
include ../config/config

INSTALL_HEADER = $(INSTALL_DIR)/include/tclap/CmdLine.h

all:  install

install: $(INSTALL_HEADER)

$(INSTALL_HEADER): 
	tar -xzf tclap-1.2.1.tar.gz
	pushd tclap-1.2.1; \
	 	./configure --prefix=$(INSTALL_DIR); \
		mv Makefile Makefile.tmp; \
		sed -e 's/tests//' -e 's/examples//' Makefile.tmp > Makefile; \
		make install; \
		popd

clean:
	rm -rf tclap.1.2.1 $(INSTALL_HEADER)

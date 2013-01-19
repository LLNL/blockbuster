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
xzname=xz-5.0.4
libname=$(INSTALL_DIR)/lib/liblzma
include ../config/config

all: $(libname).a

$(libname).a:
	[ -d ${xzname} ] || tar -xjf ${xzname}.tar.bz2
	cd  ${xzname}; \
	CC="$(CC)" CFLAGS="$(CFLAGS)" ./configure --prefix=$(INSTALL_DIR); \
		$(MAKE) -j 20 install

clean:
	rm -rf ${xzname}  $(libname).*

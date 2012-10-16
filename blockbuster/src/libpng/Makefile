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
#	Makefile for libpng (1.2.8)
#
#	$Id: Makefile,v 1.9 2007/11/07 01:31:00 wealthychef Exp $
#
include ../config/config

LIBTARGET=$(INSTALL_DIR)/lib/libsrwpng.a

VPATH = libpng-1.2.8

INSTALL_HEADERS = $(INSTALL_DIR)/include/pngsimple.h $(INSTALL_DIR)/include/png.h $(INSTALL_DIR)/include/pngconf.h 

all: $(LIBTARGET) 

#export CPPFLAGS += $(shell if [ $$(uname) == AIX ]; then echo -DPNG_SETJMP_NOT_SUPPORTED; fi)
#export CFLAGS += $(shell if [ $$(uname) == AIX ]; then echo -DPNG_SETJMP_NOT_SUPPORTED; fi)
export CPPFLAGS += -Ilibpng-1.2.8
export CFLAGS += -Ilibpng-1.2.8


$(LIBTARGET): libsrwpng.a 
	cp libsrwpng.a $(INSTALL_DIR)/lib

# change to srwpng for "simplereadwritepng", to avoid conflicts with other libpng's which might be installed
libsrwpng.a: libpng-1.2.8/.libs/libpng.a $(INSTALL_HEADERS) pngread.o pngwutil.o simple.o
	echo making libpng.a 
	cp libpng-1.2.8/.libs/libpng.a .
	$(AR) rsc libpng.a  pngread.o pngwutil.o simple.o
	mv libpng.a libsrwpng.a

libpng-1.2.8/.libs/libpng.a:
	if [ -d libpng-1.2.8 ]; then rm -rf libpng-1.2.8; fi
	tar -xvzf libpng-1.2.8.tgz
	cd libpng-1.2.8 ; \
	export CC="$(CC)"; export CFLAGS="$(CFLAGS)"; \
	export CPPFLAGS="$(CPPFLAGS)"; \
	./configure --prefix=$(INSTALL_DIR) && \
	CC="$(CC)" CFLAGS="$(CFLAGS)"  $(MAKE) -j 20 

$(INSTALL_DIR)/include/pngsimple.h: pngsimple.h
	cp $< $@

$(INSTALL_DIR)/include/png.h: libpng-1.2.8/png.h
	cp $< $@

$(INSTALL_DIR)/include/pngconf.h: libpng-1.2.8/pngconf.h
	cp $< $@

clean:
	rm -rf libpng-1.2.8 $(LIBTARGET) *.a *.o $(INSTALL_DIR)/include/pngsimple.h

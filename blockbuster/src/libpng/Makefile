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
#	Makefile for libpng 
#
#	$Id: Makefile,v 1.9 2007/11/07 01:31:00 wealthychef Exp $
#
include ../config/config

LIBTARGET=$(INSTALL_DIR)/lib/libsrwpng.a
PNG_VERSION = 1.2.51
PNG_SHORTVERSION=12
VPATH = libpng-$(PNG_VERSION)

INSTALL_HEADERS = $(INSTALL_DIR)/include/pngsimple.h $(INSTALL_DIR)/include/png.h $(INSTALL_DIR)/include/pngconf.h 

all: $(LIBTARGET) 
	echo $@ succeeded

#export CPPFLAGS += $(shell if [ $$(uname) == AIX ]; then echo -DPNG_SETJMP_NOT_SUPPORTED; fi)
#export CFLAGS += $(shell if [ $$(uname) == AIX ]; then echo -DPNG_SETJMP_NOT_SUPPORTED; fi)
export CPPFLAGS += -Ilibpng-$(PNG_VERSION)
export CFLAGS += -Ilibpng-$(PNG_VERSION)

BUILT_LIBPNG=libpng-$(PNG_VERSION)/.libs/libpng$(PNG_SHORTVERSION).a

$(LIBTARGET): libsrwpng.a 
	echo making $@ due to $?
	cp libsrwpng.a $(INSTALL_DIR)/lib
	echo $@ succeeded

# change to srwpng for "simplereadwritepng", to avoid conflicts with other libpng's which might be installed
libsrwpng.a: $(BUILT_LIBPNG) $(INSTALL_HEADERS) pngread.o pngwutil.o simple.o
	echo making $@ due to $?
	cp $(BUILT_LIBPNG) .
	$(AR) rsc libpng$(PNG_SHORTVERSION).a  pngread.o pngwutil.o simple.o
	mv libpng$(PNG_SHORTVERSION).a libsrwpng.a
	echo $@ succeeded

$(BUILT_LIBPNG): libpng-$(PNG_VERSION)/png.h
	echo making $@ due to $?
	if [ -d libpng-$(PNG_VERSION) ]; then rm -rf libpng-$(PNG_VERSION); fi
	tar -xzf libpng-$(PNG_VERSION).t*gz
	cd libpng-$(PNG_VERSION) ; \
	export CC="$(CC)"; export CFLAGS="$(CFLAGS)"; \
	export CPPFLAGS="$(CPPFLAGS)"; \
	./configure --prefix=$(INSTALL_DIR) && \
	$(MAKE) -j 20 
	echo $@ succeeded

libpng-$(PNG_VERSION)/png.h libpng-$(PNG_VERSION)/pngconf.h:
	tar -xvzf libpng-$(PNG_VERSION).t*gz

$(INSTALL_DIR)/include/pngsimple.h: pngsimple.h 
	cp $< $@

$(INSTALL_DIR)/include/png.h: libpng-$(PNG_VERSION)/png.h
	cp $< $@

$(INSTALL_DIR)/include/pngconf.h: libpng-$(PNG_VERSION)/pngconf.h
	cp $< $@

clean:
	rm -rf libpng-$(PNG_VERSION) $(LIBTARGET) *.a *.o $(INSTALL_DIR)/include/pngsimple.h

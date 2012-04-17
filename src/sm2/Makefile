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
# $Id: Makefile,v 1.10 2009/05/12 17:48:36 wealthychef Exp $
# 
#   Abstract:
#
#   Author:
#
#
#	Makefile for the sm Library
#
#       $Id: Makefile,v 1.10 2009/05/12 17:48:36 wealthychef Exp $
#
include ../config/config

SRC_FILES = StreamingMovie.C SMJpegCodec.C getframe.C # sm2.C # smRLE.C smRaw.C smGZ.C smLZO.C smJPG.C 
OFILES = StreamingMovie.o SMJpegCodec.o getframe.o # sm2.o # smRLE.o smRaw.o smGZ.o smLZO.o smJPG.o 
HFILES = # sm2.h # smRLE.h smRaw.h smGZ.h smLZO.h smJPG.h
INSTALL_DIR = ../../$(SYS_TYPE)/
$(warning INSTALL_DIR is $(INSTALL_DIR))
MAGICK_DIR= /opt/local

CPPFLAGS = -g -Dcimg_use_magick $(shell $(MAGICK_DIR)/bin/Magick++-config --cppflags) $(shell $(MAGICK_DIR)/bin/Magick++-config --cxxflags) -I../CImg-1.5.0_beta -I$(INSTALL_DIR)/include -I../common
LDFLAGS = -L$(INSTALL_DIR)/lib $(shell $(MAGICK_DIR)/bin/Magick++-config --ldflags) $(shell $(MAGICK_DIR)/bin/Magick++-config --libs)  -lX11 -lz $(INSTALL_DIR)/lib/libjpeg.a
# all: lib doc include
all: getframe

getframe: $(OFILES)
	$(CXX) $(LDFLAGS)  -o $@ $^

%.o: %.C
	$(CXX) $(CPPFLAGS) -c -o $@ $<

lib: $(INSTALL_DIR)/lib/libsm2.a

$(INSTALL_DIR)/lib/libsm2.a:  libsm2.a 
	$(call MAKE-INSTALL-DIRS)
	cp $^ $(INSTALL_DIR)/lib

doc: 

include: $(INSTALL_DIR)/include/sm2.h

$(INSTALL_DIR)/include/sm2.h: $(HFILES)
	mkdir -p $(INSTALL_DIR)/include/sm2
	cp $(HFILES) $(INSTALL_DIR)/include/sm2


#$(OFILES): $(HFILES) $(SRC_FILES)

libsm2.a: $(OFILES)
	$(AR) rsc $@ $(OFILES)


clean: 
	rm -rf $(CC_CACHE) *.o core *.a
	rm -rf $(INSTALL_DIR)/lib/libsm2.a $(INSTALL_DIR)/sm2

#use with caution:
depend: 
	makedepend -Y. -Y.. $(INCLUDES) $(SRC_FILES)

# DO NOT DELETE THIS LINE -- make depend depends on it.

StreamingMovie.o: StreamingMovie.h SMCodec.h ../common/stringutil.h
StreamingMovie.o: ../common/timer.h ../common/stringutil.h iodefines.h
StreamingMovie.o: SMJpegCodec.h SMGZCodec.h
SMJpegCodec.o: SMJpegCodec.h SMCodec.h
getframe.o: StreamingMovie.h SMCodec.h ../common/stringutil.h
getframe.o: ../common/timer.h ../common/stringutil.h

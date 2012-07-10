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

MKDEPEND_SRC_FILES = BufferedStreamingMovie.C playmovie.C StreamingMovie.C SMJpegCodec.C getframe.C 
GETFRAME_OFILES = StreamingMovie.o SMJpegCodec.o getframe.o
BUFFERED_OFILES = BufferedStreamingMovie.o StreamingMovie.o SMJpegCodec.o playmovie.o
MKDEPEND_HFILES = BufferedStreamingMovie.h StreamingMovie.h 
INSTALL_DIR ?= ../../$(SYS_TYPE)/
$(warning INSTALL_DIR is $(INSTALL_DIR))

CPPFLAGS = -g -Dcimg_use_magick $(shell Magick++-config --cppflags) $(shell Magick++-config --cxxflags) -I../CImg-1.5.0_beta -I$(INSTALL_DIR)/include -I../common
LDFLAGS = -L$(INSTALL_DIR)/lib $(shell Magick++-config --ldflags --libs)  -lX11 -lz $(INSTALL_DIR)/lib/libjpeg.a $(INSTALL_DIR)/lib/libboost_thread.a
# all: lib doc include

default: all

all: getframe playmovie 

playmovie: $(BUFFERED_OFILES) 
	$(CXX)  -o $@ $^ $(LDFLAGS)

getframe: $(GETFRAME_OFILES)
	$(CXX)  -o $@ $^ $(LDFLAGS) 

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


clean: 
	rm -rf $(CC_CACHE) *.o core *.a
	rm -rf $(INSTALL_DIR)/lib/libsm2.a $(INSTALL_DIR)/sm2

#use with caution:
depend: 
	makedepend -Y. -Y.. $(INCLUDES) $(MKDEPEND_SRC_FILES)

# DO NOT DELETE THIS LINE -- make depend depends on it.

BufferedStreamingMovie.o: BufferedStreamingMovie.h StreamingMovie.h SMCodec.h
BufferedStreamingMovie.o: ../common/stringutil.h ../common/timer.h
BufferedStreamingMovie.o: ../common/stringutil.h iodefines.h
playmovie.o: BufferedStreamingMovie.h StreamingMovie.h SMCodec.h
playmovie.o: ../common/stringutil.h ../common/timer.h ../common/stringutil.h
StreamingMovie.o: StreamingMovie.h SMCodec.h ../common/stringutil.h
StreamingMovie.o: ../common/timer.h ../common/stringutil.h iodefines.h
StreamingMovie.o: SMJpegCodec.h SMGZCodec.h
SMJpegCodec.o: SMJpegCodec.h SMCodec.h
getframe.o: StreamingMovie.h SMCodec.h ../common/stringutil.h
getframe.o: ../common/timer.h ../common/stringutil.h

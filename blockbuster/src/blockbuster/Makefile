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
# $Id: Makefile,v 1.48 2009/05/18 20:10:05 wealthychef Exp $
#
#   Abstract: Makefile for blockuster
#
#   Author: Rich Cook
#

include ../config/config 

BBSRCDIR=$(shell pwd)

LDFLAGS += -lboost_regex -lreadline -ltermcap 
ifeq ($(shell uname),Darwin)
	blockbuster = blockbuster.app
	X11_LIBS += -lXext -lXtst -lXinerama
	GPROF_HELPER=
else
	X11_LIBS +=  -lXtst -lXinerama
	blockbuster = blockbuster 
	GPROF_HELPER=$(INSTALL_DIR)/lib/gprof-helper.so
endif

ifndef NO_MPI
	MPI_DEFINES = USE_MPI=1 USE_STDARG HAVE_STDLIB_H=1 HAVE_STRING_H=1 HAVE_UNISTD_H=1 HAVE_STDARG_H=1 USE_STDARG=1 MALLOC_RET_VOID=1 
	export CC := mpicc
	export CXX := mpiCC

else
	MPI_DEFINES = 
endif


blockbuster_srun = $(INSTALL_DIR)/bin/blockbuster_srun.sh $(GPROF_HELPER)

all: $(blockbuster) $(blockbuster_srun) 

FORCE: 

.PHONY: FORCE

.PHONY: all

# QMAKE: allow user to set in environment or have in PATH: 
QMAKE ?= $(shell which qmake-5.4.2 || which qmake)
$(warning QMAKE from environment is $(QMAKE))
ifeq ($(QMAKE),)
# if still no qmake, look in canonical place 
	CANONICAL=/usr/gapps/asciviz/Trolltech/latest/$(SYS_TYPE)/bin/qmake
	QMAKE = $(shell [ -x $(CANONICAL) ] && echo $(CANONICAL))
endif
ifeq ($(QMAKE),) 
# finally, tag to make it obvious what happened if not found:
	QMAKE = QMAKE_NOT_FOUND
endif
$(warning QMAKE is $(QMAKE))
QBINDIR = $(shell dirname $(QMAKE))

# no matter what, this is a good idea:
# export OPT += -g -Wl,-rpath,$(shell cat $(INSTALL_DIR)/lib/BOOST_DIR)/lib
ifdef GPROF
export OPT = -g -pg
endif

ifdef DEBUG
# the following adds the magic -g flag: 
DEBUG_QT = CONFIG += debug
export DEBUG=DEBUG
else
DEBUG_QT = 
DEBUG=
endif

CPP_LIB_OFILES = ../RC_cpp_lib/Prefs.o ../RC_cpp_lib/timer.o

SOURCES=main.cpp blockbuster_qt.cpp movie.cpp \
	 pure_C.c frames.cpp  cache.cpp  common.cpp \
	sgi-rgbFrame.cpp convert.cpp smFrame.cpp pngFrame.cpp \
	splash.cpp pnmFrame.cpp tiffFrame.cpp \
	libdmx/dmx.c MovieCues.cpp events.cpp ImageCache.cpp  \
	Renderer.cpp util.cpp settings.cpp errmsg.cpp \
	SidecarServer.cpp x11Renderer.cpp glRenderer.cpp dmxRenderer.cpp slave.cpp  

HEADERS= errmsg.h events.h   ImageCache.h \
	../RC_cpp_lib/stringutil.h ../RC_cpp_lib/timer.h ../config/version.h \
	MovieCues.h errmsg.h pure_C.h util.h blockbuster_qt.h  \
	sgi-rgbFrame.h convert.h smFrame.h ../RC_cpp_lib/pathutil.h pngFrame.h \
	splash.h pnmFrame.h tiffFrame.h ../RC_cpp_lib/Prefs.h \
	Renderer.h glRenderer.h dmxRenderer.h 

FORMS = MovieCueWidget.ui BlockbusterControl.ui InfoWindow.ui

UI_HEADERS = ui_MovieCueWidget.h ui_BlockbusterControl.h ui_InfoWindow.h

LIBDMX = libdmx/libdmx.a 


iotest: iotest.C
	$(CXX) iotest.C -g -o iotest

libdmx/libdmx.a: libdmx/dmx.cpp
	$(CXX) -O -I. -c libdmx/dmx.cpp -o libdmx/dmx.o
	$(AR) rsc $@ libdmx/dmx.o


$(INSTALL_DIR)/lib/gprof-helper.so: gprof-helper.c
	$(CC) -shared -nostdlib -fPIC $^ -o $@ -lpthread -ldl


$(DEPENDENT_LIBS): 
	@echo	
	@echo '************************ ERROR ****************************'
	@echo ATTENTION: Please do not attempt to do a Make in the blockbuster subdirectory.  
	@echo Builds are done from the top level only.  
	@echo This message is because of $(DEPENDENT_LIBS)
	@echo INSTALL_DIR is $(INSTALL_DIR)
	@echo '************************ ERROR ****************************'
	@echo	
	exit 1

DEPENDENT_LIBS = $(INSTALL_DIR)/lib/libboost_thread.a $(INSTALL_DIR)/lib/libz.a $(INSTALL_DIR)/lib/libsrwpng.a $(INSTALL_DIR)/lib/libjpeg.a  $(INSTALL_DIR)/lib/libpnmrw.a $(INSTALL_DIR)/lib/liblzo2.a $(INSTALL_DIR)/lib/liblzma.a $(INSTALL_DIR)/lib/libtiff.a 

blockbuster.pro :   $(FORMS) $(SOURCES) $(HEADERS)  Makefile
	echo QMAKE IS \"$(QMAKE)\" and INSTALL_DIR is $(INSTALL_DIR)
	INSTALL_DIR=$(INSTALL_DIR) $(QMAKE) -project  -nopwd -Wall 'LIBS += $(CPP_LIB_OFILES) $(DEPENDENT_LIBS) $(LDFLAGS) -L$(INSTALL_DIR)/lib -L/sw/lib  $(X11_LIBS) $(GL_LIBS) $(PTHREADS_LIBS) -lsmovie  -lsrwpng -ljpeg -llzo2 -lz -llzma -lm  ' "INCLUDEPATH += libdmx ../common ../RC_cpp_lib ../config $(INSTALL_DIR)/include  $$(echo $(X11_INCLUDES)  | sed -e s/-I//g ) " "MAKEFILE = Makefile.qt.include" "DEFINES +=  NO_BOOST $(MPI_DEFINES) $(USE_DMX) USE_QT $(DEBUG)" "QT += network"  "QT += opengl" "HEADERS = $(HEADERS)" "FORMS += $(FORMS)" "$(DEBUG_QT)"  "RESOURCES += images.qrc"  $(SOURCES)  

qmake-force:
	$(QMAKE) -Wall 
	if [ $$(uname -s) == "Darwin" ]; then inplace='""'; fi; sed -i $$inplace -e 's~-O2~$(OPT) ~g' -e 's~-O1~$(OPT) ~g'  -e 's~-g ~$(OPT) ~g' -e 's/CC.*=.*gcc/CC = $(CC) /g' -e 's/CXX.*=.*g++/CXX = $(CXX) /g' -e 's/LINK.*=.*g++/LINK = $(CXX) /g' -e 's/\(-o Prefs.o Prefs.C\)/-Wno-type-limits \1/' Makefile.qt.include
	echo sm.o: ../sm/smBase.h >> Makefile.qt.include

Makefile.qt.include: blockbuster.pro
	echo running qmake because $@
	$(MAKE) qmake-force
	makedepend -I../config -I../common -I../RC_cpp_lib -fMakefile.qt.include -Y. $(SOURCES)  

.force:

.PHONY : .force

relink: $(DEPENDENT_LIBS)
	rm -rf $(blockbuster)

# for now, have to copy blockbuster up to allow command line usage
$(blockbuster): relink Makefile.qt.include  .force
	INSTALL_DIR=$(INSTALL_DIR) $(MAKE) -j 20 -f Makefile.qt
	mkdir -p $(INSTALL_DIR)/bin && cp -r $(blockbuster) $(INSTALL_DIR)/bin/

$(blockbuster_srun): blockbuster_srun.sh 
	cp $< $@

$(CPP_LIB_OFILES) : %.o : %.C
	g++ -c -o $@ $<

project: blockbuster.pro 

qmake-clean: 
	rm -f blockbuster.pro
	$(MAKE) blockbuster.pro
	rm -f Makefile.qt.include
	INSTALL_DIR=$(INSTALL_DIR) $(MAKE) -j 20 -f Makefile.qt.include 

project-clean:


clean: 
	rm -f blockbuster blockbuster.pro ui_*.h *.o moc_*
	rm -f libdmx/*.{o,a}


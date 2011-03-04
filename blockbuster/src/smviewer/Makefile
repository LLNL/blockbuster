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

#==================================
export OPT=-g
ifdef GPROF
export OPT = -g -pg
endif
INSTALL_DIR ?= /usr/gapps/asciviz/blockbuster/dev/chaos_4_x86_64_ib/

#==================================
include ../config/config

#==================================
QMAKE ?= $(shell which qmake)
ifeq ($(QMAKE),) 
# if still no qmake, look in canonical place 
	CANONICAL=/usr/gapps/asciviz/Trolltech/4.4.0/$(SYS_TYPE)/bin/qmake
	QMAKE = $(shell [ -x $(CANONICAL) ] && echo $(CANONICAL))
endif
ifeq ($(QMAKE),) 
# finally, tag to make it obvious what happened if not found:
	QMAKE = QMAKE_NOT_FOUND
endif
$(warning QMAKE is $(QMAKE))
#==================================
SOURCES = smviewer.cpp
HEADERS = 
FORMS = 
UI_HEADERS = 

DEPENDENT_LIBS = $(INSTALL_DIR)/lib/libz.a $(INSTALL_DIR)/lib/libsrwpng.a $(INSTALL_DIR)/lib/libjpeg.a  $(INSTALL_DIR)/lib/libpnmrw.a $(INSTALL_DIR)/lib/liblzo2.a $(INSTALL_DIR)/lib/libtiff.a 

all:  smviewer

smviewer: Makefile.qt $(SOURCES)
	gmake -f Makefile.qt
#		g++ -g smviewer.cpp -o smviewer $(CPPFLAGS) $(LDFLAGS) -lsmovie -lz -ljpeg -llzo2


smviewer.pro :   $(FORMS)
	echo QMAKE IS \"$(QMAKE)\" and INSTALL_DIR is $(INSTALL_DIR)
	INSTALL_DIR=$(INSTALL_DIR) $(QMAKE) -project -nopwd 'LIBS += $(LDFLAGS) -L$(INSTALL_DIR)/lib -L/sw/lib  $(X11_LIBS) $(GL_LIBS) $(PTHREADS_LIBS) -lsmovie  -lsrwpng -ltiff -ljpeg -llzo2 -lz  -lm ' "INCLUDEPATH += libdmx ../common ../config $(INSTALL_DIR)/include  $$(echo $(X11_INCLUDES)  | sed -e s/-I//g ) " "MAKEFILE = Makefile.qt" "DEFINES +=  NO_BOOST $(MPI_DEFINES) $(USE_DMX) USE_QT $(DEBUG)" "QT += network"  "QT += opengl" "HEADERS = $(HEADERS)" "FORMS += $(FORMS)" "$(DEBUG_QT)" $(SOURCES)  

qmake-force:
	$(QMAKE) 
	$(call SEDFILES,s/-O2/$(OPT) /g,Makefile.qt)
	$(call SEDFILES,s/-O1/$(OPT) /g,Makefile.qt)
	$(call SEDFILES,s/-g /$(OPT) /g,Makefile.qt)
	$(call SEDFILES,s/CC.*=.*gcc/CC = $(CC) /g,Makefile.qt)
	$(call SEDFILES,s/CXX.*=.*g++/CXX = $(CXX) /g,Makefile.qt)
	$(call SEDFILES,s/LINK.*=.*g++/LINK = $(CXX) /g,Makefile.qt)
	echo sm.o: ../sm/smBase.h >> Makefile.qt

Makefile.qt: smviewer.pro
	echo running qmake because $@
	$(MAKE) qmake-force

.force:

relink: $(DEPENDENT_LIBS)
	rm -rf $(smviewer)

qmake-clean: 
	rm -f smviewer.pro
	$(MAKE) smviewer.pro
	rm -f Makefile.qt
	INSTALL_DIR=$(INSTALL_DIR) $(MAKE) Makefile.qt 

project-clean:

clean: 
	rm -f smviewer smviewer.pro ui_*.h *.o moc_*

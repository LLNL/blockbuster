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
  # $Id: Makefile,v 1.40 2009/04/08 19:58:29 wealthychef Exp $
  #
  #   Abstract: Sidecar is a "remote control" for blockbuster.  
  #  It lives in a subdirectory because Qt does not like to build multiple 
  #  GUI applications in the same directory, or I haven't figured out how to 
  #  do so yet. 
  #
  #   Author: Rich Cook
  #
# This work performed under the auspices of the U.S. Department of Energy by Lawrence Livermore National Laboratory under Contract DE-AC52-07NA27344.
# This document was prepared as an account of work sponsored by an agency of the United States government. Neither the United States government nor Lawrence Livermore National Security, LLC, nor any of their employees makes any warranty, expressed or implied, or assumes any legal liability or responsibility for the accuracy, completeness, or usefulness of any information, apparatus, product, or process disclosed, or represents that its use would not infringe privately owned rights. Reference herein to any specific commercial product, process, or service by trade name, trademark, manufacturer, or otherwise does not necessarily constitute or imply its endorsement, recommendation, or favoring by the United States government or Lawrence Livermore National Security, LLC. The views and opinions of authors expressed herein do not necessarily state or reflect those of the United States government or Lawrence Livermore National Security, LLC, and shall not be used for advertising or product endorsement purposes.
#

SOURCE_ROOT=$(shell cd ../; pwd)

# just build with -g for now
OPT=-g -O0
ifdef GPROF
OPT = -g -pg
endif

include $(SOURCE_ROOT)/config/config

ifeq ($(shell uname),Darwin)
	sidecar = sidecar.app 
else
	sidecar = sidecar
endif

all: $(sidecar)

FORCE: 

.PHONY: FORCE

.PHONY: all

# QMAKE: allow user to set in environment or have in PATH: 
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

# this builds against Qt debuglibraries, which seems to break the auto-signal mechanism...
#DEBUG_QT  = yes

# export OPT += -g -Wl,-rpath,$(shell cat $(INSTALL_DIR)/lib/BOOST_DIR)/lib

ifdef DEBUG
DEBUG_QT = CONFIG += debug 
RELEASE_QT =  CONFIG -= release
QT = QT += network qt
export DEBUG=DEBUG
else 
QT = QT += network
RELEASE_QT =  CONFIG += release
DEBUG=
endif

LDFLAGS +=  -ltermcap -lsmovie  -lsrwpng -ljpeg -llzo2 -lz -llzma -lm -lboost_filesystem -lboost_system -lboost_regex -lreadline

SOURCES = common.cpp events.cpp errmsg.cpp util.cpp sidecar_main.cpp settings.cpp MovieCues.cpp Prefs.C RemoteControl.cpp timer.C sidecar.cpp
HEADERS = sidecar.h  ../RC_cpp_lib/Prefs.h RemoteControl.h common.h errmsg.h Lockables.h MovieCues.h settings.h ../sm/smBase.h ../RC_cpp_lib/stringutil.h 
FORMS = BlockbusterControl.ui blockbusterLaunchDialog.ui InfoWindow.ui MovieCueWidget.ui sidecar.ui
UI_HEADERS = ui_MovieCueWidget.h ui_BlockbusterControl.h ui_InfoWindow.h ui_sidecar.h ui_blockbusterLaunchDialog.h

# NOTE:  Modify sidecar.pro by hand, to avoid picking up all of ../blockbuster or something .  

qmake-force:
	$(QMAKE) -Wall 
	if [ $$(uname -s) == "Darwin" ]; then inplace='""'; fi; sed -i $$inplace -e 's~-O2~$(OPT) ~g' -e 's~-O1~$(OPT) ~g'  -e 's~-g ~$(OPT) ~g' -e 's/CC.*=.*gcc/CC = $(CC) /g' -e 's/CXX.*=.*g++/CXX = $(CXX) /g' -e 's/LINK.*=.*g++/LINK = $(CXX) /g' -e 's/\(-o Prefs.o Prefs.C\)/-Wno-type-limits \1/' Makefile.qt.include
	echo sm.o: ../sm/smBase.h >> Makefile.qt.include

Makefile.qt.include: sidecar.pro 
	echo running qmake because $@
	$(MAKE) qmake-force
	makedepend -I../config -I../common -I../RC_cpp_lib -fMakefile.qt.include -Y. $(SOURCES)  

sidecar.pro: $(SOURCES) $(HEADERS) $(FORMS) Makefile images.qrc images
	echo QMAKE IS \"$(QMAKE)\" and INSTALL_DIR is $(INSTALL_DIR)
	INSTALL_DIR=$(INSTALL_DIR) $(QMAKE) -project -nopwd -Wall 'LIBS += $(DEPENDENT_LIBS) $(LDFLAGS) -L$(INSTALL_DIR)/lib -L/sw/lib  $(X11_LIBS) $(GL_LIBS) $(PTHREADS_LIBS)  ' "INCLUDEPATH += libdmx ../common  ../RC_cpp_lib ../blockbuster ../config $(INSTALL_DIR)/include  $$(echo $(X11_INCLUDES)  | sed -e s/-I//g ) " "MAKEFILE = Makefile.qt.include" "DEFINES +=  NO_BOOST $(MPI_DEFINES) $(USE_DMX) USE_QT $(DEBUG)" "QT += network opengl widgets" "HEADERS = $(HEADERS)" "FORMS += $(FORMS)" "$(DEBUG_QT)"  "RESOURCES += images.qrc"  $(SOURCES)  
	echo contents of sidecar.pro are $$(cat sidecar.pro)


%.h :$(SOURCE_ROOT)/blockbuster/%.h
	cp $< $@

%.cpp : $(SOURCE_ROOT)/blockbuster/%.cpp
	cp $< $@

./%.C : $(SOURCE_ROOT)/blockbuster/%.C
	cp $< $@

%.h : $(SOURCE_ROOT)/RC_cpp_lib/%.h
	cp $< $@

%.C : $(SOURCE_ROOT)/RC_cpp_lib/%.C
	cp $< $@

%.ui: $(SOURCE_ROOT)/blockbuster/%.ui
	cp $< $@ 

# this is the images directory and images.qrc:
images.qrc: $(SOURCE_ROOT)/blockbuster/images.qrc
	cp  $< $@

images: $(SOURCE_ROOT)/blockbuster/images
	mkdir -p images
	cp -f $(SOURCE_ROOT)/blockbuster/images/*png images/

ui_%.h: %.ui 
	echo making $@ due to $?
	uic $< -o $@

$(sidecar): Makefile.qt.include $(UI_HEADERS) $(COPYFILES) FORCE 
	$(MAKE) -j 1  -f Makefile.qt
	cp -r $(sidecar) $(INSTALL_DIR)/bin/

clean:
	rm -rf sidecar  ui_*.h Makefile.qt.include moc_*cpp *.o $(xxCOPYFILES)


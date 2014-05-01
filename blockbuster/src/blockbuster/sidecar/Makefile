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

# just build with -g for now
OPT=-g -O0
ifdef GPROF
OPT = -g -pg
endif

include ../../config/config

ifeq ($(shell uname),Darwin)
	sidecar = sidecar.app 
else
	sidecar = sidecar
endif

all: $(sidecar)

FORCE: 

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

SOURCES = sidecar_main.cpp sidecar.cpp MovieCues.cpp events.cpp Prefs.C RemoteControl.cpp settings.cpp ./timer.C
HEADERS = sidecar.h MovieCues.h events.h common.h settings.h Prefs.h ./timer.h stringutil.h
FORMS = MovieCueWidget.ui BlockbusterControl.ui InfoWindow.ui sidecar.ui blockbusterLaunchDialog.ui
UI_HEADERS = ui_MovieCueWidget.h ui_BlockbusterControl.h ui_InfoWindow.h ui_sidecar.h ui_blockbusterLaunchDialog.h

# NOTE:  Modify sidecar.pro by hand, to avoid picking up all of ../blockbuster or something .  

Makefile.qt.include: sidecar.pro $(SOURCES) $(HEADERS) Makefile
	$(QMAKE) 
	../sedfiles.sh -e "s:INSTALLDIR_PLACEHOLDER:$(INSTALL_DIR):"  -e 's/\(-o Prefs.o Prefs.C\)/-Wno-type-limits \1/' Makefile.qt.include
	makedepend -fMakefile.qt.include -I../../config -I../../sm -I../../common -I../../RC_cpp_lib -Y. -I../../config -Y. $(SOURCES) 


%.h : ../%.h
	cp $< $@

%.cpp : ../%.cpp
	cp $< $@

%.C : ../%.C
	cp $< $@

%.h : ../../RC_cpp_lib/%.h
	cp $< $@

%.C : ../../RC_cpp_lib/%.C
	cp $< $@

%.ui: ../%.ui
	cp $< $@ 

# this is the images directory and images.qrc:
images.qrc: ../images.qrc
	cp  $< $@

images: ../images
	mkdir -p images
	cp -f ../images/*png images/

ui_%.h: Makefile.qt.include %.ui 
	echo making $@ due to $?
	make -f Makefile.qt $@

$(sidecar): Makefile.qt.include $(UI_HEADERS) $(COPYFILES) FORCE 
	$(MAKE) -j 20  -f Makefile.qt
	cp -r $(sidecar) $(INSTALL_DIR)/bin/

clean:
	rm -rf sidecar  ui_*.h Makefile.qt.include moc_*cpp *.o $(xxCOPYFILES)


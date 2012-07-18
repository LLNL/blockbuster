# Top-level makefile for blockbuster. 

SHELL = /bin/bash

SYS_TYPE ?= $(shell uname)

export INSTALL_DIR ?= $(shell if pwd | grep -e /viz/blockbuster -e /usr/gapps/asciviz>/dev/null;  then cd .. && echo `pwd`; else echo `pwd`/$(SYS_TYPE); fi)
$(warning INSTALL_DIR is $(INSTALL_DIR) )
export NO_DMX
export DEBUG

all:
	mkdir -p $(INSTALL_DIR)/man/man1 
	[ -d $(INSTALL_DIR) ] && cd src  && $(MAKE) -e all
	mkdir -p $(INSTALL_DIR)/doc/blockbuster && \
		cp -rf doc/* $(INSTALL_DIR)/doc/blockbuster
	INSTALL_DIR=$(INSTALL_DIR) ./install-Qt-libs.sh

remake: 
	SYS_TYPE=$(SYS_TYPE) ./remake.sh

bindist: 
	rm -rf *dmg $(INSTALL_DIR)/bin/{blockbuster,sidecar}*
	$(MAKE) all 
	SYS_TYPE=$(SYS_TYPE) ./make-bindist.sh 

old-bindist-deleteme: 
	INSTALL_DIR=linux-dmx remake.sh all
	INSTALL_DIR=install-linux-DMX ./make-bindist.sh 
	rm -rf linux-dmx
	INSTALL_DIR=linux-basic-nodmx remake.sh nodmx
	INSTALL_DIR=install-linux-basic ./make-bindist.sh 
	rm -rf linux-basic-nodmx

nodmx:
	NO_DMX=1 NO_MPI=true $(MAKE) all 

debug:
	DEBUG=1 $(MAKE) all

gprof: 
	DEBUG=1 GPROF=1 $(MAKE) all

nompi: 
	NO_MPI=true $(MAKE) all 

clean: 
	cd src &&	$(MAKE) clean

# DO NOT DELETE

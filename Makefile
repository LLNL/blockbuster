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

bindist: all
	INSTALL_DIR=$(INSTALL_DIR) ./make-bindist.sh

nodmx:
	NO_DMX=1 $(MAKE) all

debug:
	DEBUG=1 $(MAKE) all

gprof: 
	DEBUG=1 GPROF=1 $(MAKE) all

nompi: 
	NO_MPI=true $(MAKE) all

clean: 
	cd src &&	$(MAKE) clean


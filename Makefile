# Top-level makefile for blockbuster. 

SHELL = /bin/bash

SYS_TYPE ?= $(shell uname)

export INSTALL_DIR ?= $(shell if pwd | grep -e /usr/gapps/asciviz -e /dvsviz/blockbuster >/dev/null;  then echo `pwd`/..; else echo `pwd`/$(SYS_TYPE); fi)
$(warning INSTALL_DIR is $(INSTALL_DIR) )
export NO_DMX
export DEBUG

all:
	mkdir -p $(INSTALL_DIR)/man/man1 
	[ -d $(INSTALL_DIR) ] && cd src  && $(MAKE) -e all
	mkdir -p $(INSTALL_DIR)/doc/blockbuster && \
		cp -rf doc/* $(INSTALL_DIR)/doc/blockbuster

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

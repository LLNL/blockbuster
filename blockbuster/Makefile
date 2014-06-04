# Top-level makefile for blockbuster. 

SHELL = /bin/bash

SYS_TYPE ?= $(shell uname)

export INSTALL_DIR ?= $(shell if pwd | grep -e /viz/blockbuster -e /usr/gapps/asciviz>/dev/null;  then cd ../.. && echo `pwd`; else echo `pwd`/$(SYS_TYPE); fi)
$(warning INSTALL_DIR is $(INSTALL_DIR) )

# hacky way to get configurations to stick
GET_VAR_VALUE = $(shell [ -f src/config/$(1) ] && grep YES src/config/$(1))
export NO_DMX ?= $(call GET_VAR_VALUE,nodmx)
export DEBUG ?= $(call GET_VAR_VALUE,debug)
export MPI ?= $(call GET_VAR_VALUE,mpi)
export GPROF ?= $(call GET_VAR_VALUE,gprof)
export NOBOOSTLINK ?= $(call GET_VAR_VALUE,noboostlink)

all:
	mkdir -p $(INSTALL_DIR)/man/man1 
	[ -d $(INSTALL_DIR) ] && cd src  && $(MAKE) -e all
	mkdir -p $(INSTALL_DIR)/doc/blockbuster && \
		cp -rf doc/* $(INSTALL_DIR)/doc/blockbuster
	INSTALL_DIR=$(INSTALL_DIR) ./install-Qt-libs.sh
#	make testit

test: 
	./tests/smtoolsTest.py 

src/%: 
	cd $@ && $(MAKE) -e all

remake: 
	SYS_TYPE=$(SYS_TYPE) ./remake.sh 

bindist: 
	rm -rf *dmg $(INSTALL_DIR)/bin/{blockbuster,sidecar}*
	$(MAKE) all 
	SYS_TYPE=$(SYS_TYPE) ./make-bindist.sh 

dmx: mpi
	echo YES > src/config/dmx

nodmx: nompi
	rm -f src/config/dmx

debug:
	echo YES > src/config/debug

opt:
	rm -f src/config/debug

gprof: debug
	echo YES > src/config/gprof

nogprof: 
	rm -f src/config/gprof

mpi: 
	rm -f src/config/nompi

nompi: 
	echo YES > src/config/nompi

boostlink:
	rm -f src/config/noboostlink

noboostlink:
	echo YES > src/config/noboostlink


clean: 
	cd src &&	$(MAKE) clean

# DO NOT DELETE

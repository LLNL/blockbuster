# Makefile for blockbuster project top level "src" directory
# Richard Cook, June 1, 2007
export INSTALL_DIR ?= $(shell pwd)/../$(SYS_TYPE)

include config/config

# if you wish to build with DMX, set DMX in your environment:
ifndef NO_DMX
export USE_DMX = USE_DMX
else
export USE_DMX =
endif


DEFAULT: all

debug: 
	DEBUG=1 $(MAKE) all


$(warning INSTALL_DIR is $(INSTALL_DIR)) 
QTHOME ?= /usr/mic/dvsviz/Trolltech/latest/$(SYS_TYPE)
export PATH=$(QTHOME)/bin:$(shell echo $${PATH})
export BUILD_ROOT ?= $(shell pwd)

BLOCKBUSTERBUILDDIRS =  boost-1.49.0 tclap-1.2.1 xzlib zlib lzolib pt  sndplay \
	libimage libpnmrw libpng libjpeg libtiff mpegdir \
	evolib sm sm2 smapp blockbuster


BUILD-ALL = \
	export INSTALL_DIR=$${INSTALL_DIR}; \
	for dir in $(BLOCKBUSTERBUILDDIRS); do \
		echo building dir $$dir; \
		if ! cd $(BUILD_ROOT)/$${dir} || ! $(MAKE) -j 20 $(1);  then \
						echo Make failed in $${dir}; \
			exit 1; \
		fi; \
	done


all: sidecar

sidecar: $(INSTALL_DIR)/bin/sidecar

FORCE:

$(INSTALL_DIR)/bin/sidecar: FORCE
	$(call MAKE-INSTALL-DIRS)
	$(call BUILD-ALL,)

clean:
	$(call BUILD-ALL,clean)

blockbuster-clean:
	cd blockbuster && $(MAKE) clean


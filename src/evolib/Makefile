#
#       Makefile for the evo Library
#

include ../config/config

ARCHIVES=$(INSTALL_DIR)/lib/libevomovie.a
HFILES = $(INSTALL_DIR)/include/evomovie.h
OBJ = evomovie.o


all: $(ARCHIVES) $(HFILES)

$(INSTALL_DIR)/include/%.h: %.h
	cp $^ $@

$(INSTALL_DIR)/lib/%.a: %.a
	cp $^ $@

libevomovie.a: $(OBJ)
	$(AR) rsc $@ $^

clean:
	-rm -f $(OBJ) $(LIBNAME) *~ $(ARCHIVES) $(HFILES)


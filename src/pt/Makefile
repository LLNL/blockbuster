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
# $Id: Makefile,v 1.1 2007/06/13 18:59:32 wealthychef Exp $
# 
#   Abstract:
#
#   Author:
#
#
#	Makefile for the pt Library
#
#       $Id: Makefile,v 1.1 2007/06/13 18:59:32 wealthychef Exp $
#
include ../config/config

SRC_FILES = pt_gate.c pt_oneshot.c pt_pool.c pt_rw.c
OFILES = pt_gate.o pt_oneshot.o pt_pool.o pt_rw.o
HFILES = pt.h
MANFILES = pt_gate_intro.3 pt_rw_intro.3 pt_pool_intro.3

MANLINKFILES = pt_gate_init.3 pt_gate_destroy.3 pt_gate_sync.3 \
	pt_rw_init.3 pt_rw_read_lock.3 pt_rw_read_unlock.3 \
	pt_rw_write_lock.3 pt_rw_write_unlock.3 pt_rw_destroy.3 \
	pt_pool_init.3 pt_pool_destroy.3 pt_pool_add_work.3

all:  $(INSTALL_DIR)/lib/libptutil.a CPMANPAGES

$(INSTALL_DIR)/lib/libptutil.a: $(OFILES) $(HFILES)
	mkdir -p $(INSTALL_DIR)/lib
	$(AR) rsc $@ $(OFILES)
	mkdir -p $(INSTALL_DIR)/include/pt
	cp $(HFILES) $(INSTALL_DIR)/include/pt

clean:
	rm -rf $(CC_CACHE) *.o libptutil.a $(MANLINKFILES)
	list='$(MANFILES)'; \
	for j in $$list; do \
		grep LINK $$j | cut -d ' ' -f2- | xargs -n 1 rm -f ; \
	done
	rm -rf MANLINKS $(INSTALL_DIR)/include/pt $(INSTALL_DIR)/lib/libptutil.a
	makedepend

CPMANPAGES: $(MANLINKFILES)
	mkdir -p $(INSTALL_DIR)/man/man3
	cp pt_*.3 $(INSTALL_DIR)/man/man3
	date > CPMANPAGES

$(MANLINKFILES):
	basename=$$(echo $@ | sed -e 's/\(pt_gate_\).*/\1intro.3/' -e 's/\(pt_rw_\).*/\1intro.3/'  -e 's/\(pt_pool_\).*/\1intro.3/'); \
	ln -f -s $$basename $@; echo linked $$basename to $@

depend:
	makedepend -Y. -Y.. $(INCLUDES) $(SRC_FILES)

# DO NOT DELETE THIS LINE -- make depend depends on it.

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
# $Id: Makefile,v 1.10 2009/05/12 17:48:36 wealthychef Exp $
# 
#   Abstract:
#
#   Author:
#
#
#	Makefile for the sm Library
#
#       $Id: Makefile,v 1.10 2009/05/12 17:48:36 wealthychef Exp $
#

include ../config/config
export CPPFLAGS += -Ismdfc
SRC_FILES = smBase.C smXZ.C smRLE.C smRaw.C smGZ.C smLZO.C smJPG.C smdfc/smdfc.c ./timer.C
OFILES = smBase.o smXZ.o smRLE.o smRaw.o smGZ.o smLZO.o smJPG.o smdfc.o timer.o
HFILES = sm.h smBase.h smBaseP.h smXZ.h smRLE.h smRaw.h smGZ.h smLZO.h smJPG.h smdfc/smdfc.h

all:  include lib doc

lib: 
	make -j 10 $(INSTALL_DIR)/lib/libsmovie.a

$(INSTALL_DIR)/lib/libsmovie.a:  libsmovie.a 
	$(call MAKE-INSTALL-DIRS)
	cp $^ $(INSTALL_DIR)/lib

doc: $(INSTALL_DIR)/man/man4/sm.4

$(INSTALL_DIR)/man/man4/sm.4:
	mkdir -p $(INSTALL_DIR)/man/man{3,4}
	cp *.3 $(INSTALL_DIR)/man/man3/
	cp *.4 $(INSTALL_DIR)/man/man4/

include: $(INSTALL_DIR)/include/smBase.h

$(INSTALL_DIR)/include/smBase.h: $(HFILES)
	mkdir -p $(INSTALL_DIR)/include/sm
	cp $(HFILES) $(INSTALL_DIR)/include/sm


#$(OFILES): $(HFILES) $(SRC_FILES)

libsmovie.a: $(OFILES)
	@echo '***********************************************'
	$(AR) rsc $@ $(OFILES)
	@echo "libsmovie.a CREATED"
	if [ $(HAS_MPI) ]; then \
		cd smdfc; $(MAKE) -j 8; \
	fi
	@echo '***********************************************'

./timer.C: ../RC_cpp_lib/timer.C
	cp $< $@

smdfc.o: smdfc/smdfc.c smdfc/smdfcP.h
	$(CC) $(CFLAGS) -Ismdfc -c smdfc/smdfc.c
	$(CXX) $(CFLAGS) -Ismdfc -c smdfc/smdfc.c

clean: 
	rm -rf $(CC_CACHE) *.o core *.a
	cd smdfc; $(MAKE) clean
	rm -rf $(INSTALL_DIR)/lib/libsmovie.a $(INSTALL_DIR)/sm

#use with caution:
depend: 
	makedepend -Y. -Y.. -Y../RC_cpp_lib $(INCLUDES) $(SRC_FILES) $(HFILES)

# DO NOT DELETE THIS LINE -- make depend depends on it.

smBase.o: ../RC_cpp_lib/stringutil.h smBaseP.h smRaw.h smBase.h
smBase.o: ../RC_cpp_lib/stringutil.h ../RC_cpp_lib/timer.h smRLE.h smGZ.h
smBase.o: smXZ.h smLZO.h smJPG.h
smXZ.o: smXZ.h smBase.h smBaseP.h ../RC_cpp_lib/stringutil.h
smXZ.o: ../RC_cpp_lib/timer.h ../RC_cpp_lib/stringutil.h
smRLE.o: smRLE.h smBase.h smBaseP.h ../RC_cpp_lib/stringutil.h
smRLE.o: ../RC_cpp_lib/timer.h ../RC_cpp_lib/stringutil.h
smRaw.o: smRaw.h smBase.h smBaseP.h ../RC_cpp_lib/stringutil.h
smRaw.o: ../RC_cpp_lib/timer.h ../RC_cpp_lib/stringutil.h
smGZ.o: smGZ.h smBase.h smBaseP.h ../RC_cpp_lib/stringutil.h
smGZ.o: ../RC_cpp_lib/timer.h ../RC_cpp_lib/stringutil.h
smLZO.o: smLZO.h smBase.h smBaseP.h ../RC_cpp_lib/stringutil.h
smLZO.o: ../RC_cpp_lib/timer.h ../RC_cpp_lib/stringutil.h
smJPG.o: smJPG.h smBase.h smBaseP.h ../RC_cpp_lib/stringutil.h
smJPG.o: ../RC_cpp_lib/timer.h ../RC_cpp_lib/stringutil.h
smdfc/smdfc.o: smdfc/smdfcP.h smdfc/smdfc.h
smBase.o: smBaseP.h smdfc/smdfc.h ../RC_cpp_lib/stringutil.h
smBase.o: ../RC_cpp_lib/timer.h ../RC_cpp_lib/stringutil.h
smXZ.o: smBase.h smBaseP.h ../RC_cpp_lib/stringutil.h ../RC_cpp_lib/timer.h
smXZ.o: ../RC_cpp_lib/stringutil.h
smRLE.o: smBase.h smBaseP.h ../RC_cpp_lib/stringutil.h ../RC_cpp_lib/timer.h
smRLE.o: ../RC_cpp_lib/stringutil.h
smRaw.o: smBase.h smBaseP.h ../RC_cpp_lib/stringutil.h ../RC_cpp_lib/timer.h
smRaw.o: ../RC_cpp_lib/stringutil.h
smGZ.o: smBase.h smBaseP.h ../RC_cpp_lib/stringutil.h ../RC_cpp_lib/timer.h
smGZ.o: ../RC_cpp_lib/stringutil.h
smLZO.o: smBase.h smBaseP.h ../RC_cpp_lib/stringutil.h ../RC_cpp_lib/timer.h
smLZO.o: ../RC_cpp_lib/stringutil.h
smJPG.o: smBase.h smBaseP.h ../RC_cpp_lib/stringutil.h ../RC_cpp_lib/timer.h
smJPG.o: ../RC_cpp_lib/stringutil.h

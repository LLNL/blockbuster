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
# $Id: Makefile,v 1.4 2009/05/14 02:34:38 wealthychef Exp $
# 
#   Abstract:
#
#   Author:
#
#
#  Makefile for sm utility applications
#
#       $Id: Makefile,v 1.4 2009/05/14 02:34:38 wealthychef Exp $
#
export OPT = -g
include ../config/config

LIBS = $(LDFLAGS) -L$(INSTALL_DIR)/lib -lsmovie -lptutil  -levomovie $(IFL_LIBS) -lpthread -lm -ltiff -lpnmrw -lsrwpng -lz -llzo2 -llzma -limage -lm $(GZIP_LIBS) $(TIFF_LIBS) $(PNG_LIBS) $(PNM_LIBS) -ljpeg 

SMLIB = $(INSTALL_DIR)/lib/libsmovie.a $(INSTALL_DIR)/lib/libptutil.a
SFILES = smtag.C smquery.C smmake.C smtest.C ifl2sm.C img2sm.C sm2img.C rename.C smcat.C smevo.C debugutil.c
OFILES = smtag smquery smmake smtest ifl2sm img2sm sm2img rename smcat smevo

CPPFLAGS += -g 

default: $(INSTALL_DIR)/bin/smquery $(INSTALL_DIR)/bin/smtag $(INSTALL_DIR)/bin/smevo  $(INSTALL_DIR)/bin/smmake $(INSTALL_DIR)/bin/smtest $(INSTALL_DIR)/bin/ifl2sm $(INSTALL_DIR)/bin/img2sm $(INSTALL_DIR)/bin/rename $(INSTALL_DIR)/bin/sm2img $(INSTALL_DIR)/bin/sminfo $(INSTALL_DIR)/bin/smcat $(INSTALL_DIR)/bin/smevo $(INSTALL_DIR)/bin/sm2qt $(INSTALL_DIR)/bin/ps2sm $(INSTALL_DIR)/bin/sm2mpg  manpages

$(INSTALL_DIR)/bin/%: %
	cp  $? $@

$(INSTALL_DIR)/bin/sminfo: sm2img
	rm $@; ln -s sm2img $@

manpages:
	mkdir -p $(INSTALL_DIR)/man/man1
	cp *.1 $(INSTALL_DIR)/man/man1

$(OFILES) : % : %.o $(SMLIB) simple_jpeg.o
	@echo executing generic build line
		$(CXX)  $^ -o $@ $(CXXFLAGS) $(LIBS) 

smtag: smtag.o debugutil.o
		$(CXX)  $^ -o $@ $(CXXFLAGS) $(LIBS) 

rename: rename.o $(SMLIB)
	$(CXX)  rename.o -o $@ $(CXXFLAGS) -lm
	cp $@ $(INSTALL_DIR)/bin

clean:
	rm -rf $(CC_CACHE) *.o core $(OFILES) sminfo

# use this carefully!  
depend: $(SFILES)
	makedepend -Y. -Y.. $(INCLUDES) $(SFILES)

# DO NOT DELETE

smtag.o: ../sm/smBase.h ../sm/smBaseP.h ../common/stringutil.h
smquery.o: ../sm/sm.h ../sm/smLZO.h ../sm/smBase.h ../sm/smBaseP.h
smquery.o: ../common/stringutil.h ../sm/smRLE.h ../sm/smGZ.h ../sm/smXZ.h
smquery.o: ../sm/smRaw.h ../sm/smJPG.h
smmake.o: ../sm/smRaw.h ../sm/smBase.h ../sm/smBaseP.h ../common/stringutil.h
smmake.o: ../sm/smRLE.h
smtest.o: ../sm/smBase.h ../sm/smBaseP.h ../common/stringutil.h
smtest.o: ../common/timer.h ../common/stringutil.h ../config/version.h
img2sm.o: ../sm/sm.h ../sm/smLZO.h ../sm/smBase.h ../sm/smBaseP.h
img2sm.o: ../common/stringutil.h ../sm/smRLE.h ../sm/smGZ.h ../sm/smXZ.h
img2sm.o: ../sm/smRaw.h ../sm/smJPG.h ../libimage/sgilib.h
img2sm.o: ../libpnmrw/libpnmrw.h simple_jpeg.h ../pt/pt.h
sm2img.o: ../sm/smRLE.h ../sm/smBase.h ../sm/smBaseP.h ../common/stringutil.h
sm2img.o: ../sm/smGZ.h ../sm/smLZO.h ../sm/smRaw.h ../pt/pt.h
sm2img.o: ../libimage/sgilib.h ../libpnmrw/libpnmrw.h simple_jpeg.h
sm2img.o: ../config/version.h
smcat.o: ../sm/smRLE.h ../sm/smBase.h ../sm/smBaseP.h ../common/stringutil.h
smcat.o: ../sm/smGZ.h ../sm/smLZO.h ../sm/smRaw.h ../sm/smJPG.h
smcat.o: ../config/version.h ../pt/pt.h
smevo.o: ../sm/smRLE.h ../sm/smBase.h ../sm/smBaseP.h ../common/stringutil.h
smevo.o: ../sm/smGZ.h ../sm/smLZO.h ../sm/smRaw.h ../sm/smJPG.h
smevo.o: ../config/version.h
debugutil.o: debugutil.h

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
SFILES = tags.C smtag.C smquery.C smmake.C smtest.C ifl2sm.C img2sm.C sm2img.C rename.C smcat.C smevo.C debugutil.c
OFILES = smtag smquery smmake smtest ifl2sm img2sm sm2img rename smcat smevo tags

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

smtag: smtag.o tags.o debugutil.o
		$(CXX)  $^ -o $@ $(CXXFLAGS) $(LIBS) -lboost_regex -lreadline

smquery: smquery.o tags.o debugutil.o
		$(CXX)  $^ -o $@ $(CXXFLAGS) $(LIBS) -lboost_regex  -lreadline

rename: rename.o $(SMLIB)
	$(CXX)  rename.o -o $@ $(CXXFLAGS) -lm
	cp $@ $(INSTALL_DIR)/bin

clean:
	rm -rf $(CC_CACHE) *.o core $(OFILES) sminfo

# use this carefully!  
depend: $(SFILES)
	makedepend -Y. -Y.. $(INCLUDES) $(SFILES)

# DO NOT DELETE

tags.o: tags.h debugutil.h ../sm/sm.h ../sm/smLZO.h ../sm/smBase.h
tags.o: ../sm/smBaseP.h ../common/stringutil.h ../common/timer.h
tags.o: ../common/stringutil.h ../sm/smRLE.h ../sm/smGZ.h ../sm/smXZ.h
tags.o: ../sm/smRaw.h ../sm/smJPG.h
smtag.o: ../sm/sm.h ../sm/smLZO.h ../sm/smBase.h ../sm/smBaseP.h
smtag.o: ../common/stringutil.h ../common/timer.h ../common/stringutil.h
smtag.o: ../sm/smRLE.h ../sm/smGZ.h ../sm/smXZ.h ../sm/smRaw.h ../sm/smJPG.h
smtag.o: debugutil.h tags.h
smquery.o: ../sm/sm.h ../sm/smLZO.h ../sm/smBase.h ../sm/smBaseP.h
smquery.o: ../common/stringutil.h ../common/timer.h ../common/stringutil.h
smquery.o: ../sm/smRLE.h ../sm/smGZ.h ../sm/smXZ.h ../sm/smRaw.h
smquery.o: ../sm/smJPG.h debugutil.h tags.h
smmake.o: ../sm/smRaw.h ../sm/smBase.h ../sm/smBaseP.h ../common/stringutil.h
smmake.o: ../common/timer.h ../common/stringutil.h ../sm/smRLE.h
smtest.o: ../sm/smBase.h ../sm/smBaseP.h ../common/stringutil.h
smtest.o: ../common/timer.h ../common/stringutil.h ../config/version.h
img2sm.o: ../sm/sm.h ../sm/smLZO.h ../sm/smBase.h ../sm/smBaseP.h
img2sm.o: ../common/stringutil.h ../common/timer.h ../common/stringutil.h
img2sm.o: ../sm/smRLE.h ../sm/smGZ.h ../sm/smXZ.h ../sm/smRaw.h ../sm/smJPG.h
img2sm.o: ../libimage/sgilib.h ../libpnmrw/libpnmrw.h simple_jpeg.h
img2sm.o: ../pt/pt.h
sm2img.o: ../sm/smRLE.h ../sm/smBase.h ../sm/smBaseP.h ../common/stringutil.h
sm2img.o: ../common/timer.h ../common/stringutil.h ../sm/smGZ.h ../sm/smLZO.h
sm2img.o: ../sm/smRaw.h ../pt/pt.h ../libimage/sgilib.h
sm2img.o: ../libpnmrw/libpnmrw.h simple_jpeg.h ../config/version.h
smcat.o: ../sm/smRLE.h ../sm/smBase.h ../sm/smBaseP.h ../common/stringutil.h
smcat.o: ../common/timer.h ../common/stringutil.h ../sm/smGZ.h ../sm/smLZO.h
smcat.o: ../sm/smRaw.h ../sm/smJPG.h ../config/version.h ../pt/pt.h
smevo.o: ../sm/smRLE.h ../sm/smBase.h ../sm/smBaseP.h ../common/stringutil.h
smevo.o: ../common/timer.h ../common/stringutil.h ../sm/smGZ.h ../sm/smLZO.h
smevo.o: ../sm/smRaw.h ../sm/smJPG.h ../config/version.h
debugutil.o: debugutil.h

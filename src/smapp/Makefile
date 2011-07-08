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

LIBS = $(LDFLAGS) -L$(INSTALL_DIR)/lib -lsmovie -lptutil $(IFL_LIBS) -lpthread -ljpeg $(PNG_LIBS) -lm -ltiff -lpnmrw -lsrwpng -lz -llzo2

SMOFILES =  smmake.o
SMTOFILES =  smtest.o
IFL2SMFILES =  ifl2sm.o
IMG2SMFILES =  img2sm.o simple_jpeg.o
SM2IMGFILES =  sm2img.o simple_jpeg.o
SMCATFILES =  smcat.o
SMEVOFILES = smevo.o

SMLIB = $(INSTALL_DIR)/lib/libsmovie.a $(INSTALL_DIR)/lib/libptutil.a
SFILES = smmake.C smtest.C ifl2sm.C img2sm.C sm2img.C rename.C smcat.C smevo.C
OFILES = smmake smtest ifl2sm img2sm sm2img rename smcat smevo

CPPFLAGS += -g 

default: smmake smtest ifl2sm img2sm rename sm2img smcat smevo sm2qt manpages

manpages:
	mkdir -p $(INSTALL_DIR)/man/man1
	cp *.1 $(INSTALL_DIR)/man/man1

sm2qt: smcat.o $(SMLIB)
	touch sm2qt
	cp ps2sm sm2mpg $@ $(INSTALL_DIR)/bin

smevo: $(SMEVOFILES) $(SMLIB)
	$(CXX)  $(SMEVOFILES) -o $@ -levomovie $(LIBS) $(CXXFLAGS) $(GZIP_LIBS)
	cp $@ $(INSTALL_DIR)/bin

smmake: $(SMOFILES) $(SMLIB)
	$(CXX)  $(SMOFILES) -o $@ $(LIBS) $(CXXFLAGS) $(GZIP_LIBS)
	cp $@ $(INSTALL_DIR)/bin

smtest: $(SMTOFILES) $(SMLIB)
	$(CXX)  $(SMTOFILES) -o $@ $(LIBS) $(CXXFLAGS) $(GZIP_LIBS)
	cp $@ $(INSTALL_DIR)/bin

ifl2sm: $(IFL2SMFILES) $(SMLIB)
	$(CXX)  $(IFL2SMFILES) -o $@ $(LIBS) $(CXXFLAGS) $(GZIP_LIBS)
	cp $@ $(INSTALL_DIR)/bin

img2sm: $(IMG2SMFILES) $(SMLIB)
	$(CXX)  $(IMG2SMFILES) -o $@ $(LIBS) $(CXXFLAGS) $(GZIP_LIBS) \
		$(TIFF_LIBS) $(PNM_LIBS) -limage -lm
	cp $@ $(INSTALL_DIR)/bin

sm2img: $(SM2IMGFILES) $(SMLIB)
	$(CXX)  $(SM2IMGFILES) -o $@ $(LIBS) $(CXXFLAGS) $(GZIP_LIBS) \
		$(TIFF_LIBS) $(PNM_LIBS) -limage -lm
	cp $@ sminfo
	cp $@ sminfo $(INSTALL_DIR)/bin

smcat: $(SMCATFILES) $(SMLIB)
	$(CXX)  $(SMCATFILES) -o $@ $(LIBS) $(CXXFLAGS) $(GZIP_LIBS) \
		$(TIFF_LIBS) $(PNM_LIBS) -limage -lm
	cp $@ $(INSTALL_DIR)/bin

rename: rename.o $(SMLIB)
	$(CXX)  rename.o -o $@ $(CXXFLAGS) -lm
	cp $@ $(INSTALL_DIR)/bin

clean:
	rm -rf $(CC_CACHE) *.o core $(OFILES) sminfo
	makedepend

# use this carefully!  
depend: $(SFILES)
	makedepend -Y. -Y.. $(INCLUDES) $(SFILES)

# DO NOT DELETE

smmake.o: ../sm/smRaw.h ../sm/smBase.h ../sm/smRLE.h
smtest.o: ../sm/smBase.h ../common/timer.h ../common/stringutil.h
img2sm.o: ../sm/smLZO.h ../sm/smBase.h ../sm/smRLE.h ../sm/smGZ.h
img2sm.o: ../sm/smRaw.h ../sm/smJPG.h ../libimage/sgilib.h
img2sm.o: ../libpnmrw/libpnmrw.h simple_jpeg.h ../pt/pt.h
sm2img.o: ../sm/smRLE.h ../sm/smBase.h ../sm/smGZ.h ../sm/smLZO.h
sm2img.o: ../sm/smRaw.h ../libimage/sgilib.h ../libpnmrw/libpnmrw.h
sm2img.o: simple_jpeg.h
smcat.o: ../sm/smRLE.h ../sm/smBase.h ../sm/smGZ.h ../sm/smLZO.h
smcat.o: ../sm/smRaw.h ../sm/smJPG.h ../pt/pt.h
smevo.o: ../sm/smRLE.h ../sm/smBase.h ../sm/smGZ.h ../sm/smLZO.h
smevo.o: ../sm/smRaw.h ../sm/smJPG.h

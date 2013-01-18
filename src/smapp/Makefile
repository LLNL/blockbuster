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

LIBS = $(LDFLAGS) -L$(INSTALL_DIR)/lib -lsmovie -lptutil $(IFL_LIBS) -lpthread -ljpeg $(PNG_LIBS) -lm -ltiff -lpnmrw -lsrwpng -lz -llzo2 -llzma -limage -lm -levomovie $(GZIP_LIBS) $(TIFF_LIBS) $(PNM_LIBS) 

#IMG2SMFILES =  img2sm.o simple_jpeg.o
SM2IMGFILES =  sm2img.o simple_jpeg.o
SMCATFILES =  smcat.o
SMEVOFILES = smevo.o

SMLIB = $(INSTALL_DIR)/lib/libsmovie.a $(INSTALL_DIR)/lib/libptutil.a
SFILES = smtag.C smquery.C smmake.C smtest.C ifl2sm.C img2sm.C sm2img.C rename.C smcat.C smevo.C
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

sm%: sm%.o $(SMLIB) 
	@echo executing generic build line
	$(CXX)  $^ -o $@ $(CXXFLAGS) $(LIBS) 

sm2qt: smcat.o $(SMLIB)
	touch sm2qt

smevo: $(SMEVOFILES) $(SMLIB)
	$(CXX)  $(SMEVOFILES) -o $@ -levomovie $(LIBS) $(CXXFLAGS) $(GZIP_LIBS)

img2sm: img2sm.o $(SMLIB) simple_jpeg.o 
	$(CXX)  -o $@ $^ $(CXXFLAGS) -limage -lm -levomovie $(LIBS)


#img2sm: $(IMG2SMFILES) $(SMLIB)  
#	$(CXX)  $(IMG2SMFILES) -o $@ $(LIBS) $(CXXFLAGS) $(GZIP_LIBS) \
#		$(TIFF_LIBS) $(PNM_LIBS) -limage -lm

sm2img: $(SM2IMGFILES) $(SMLIB) 
	$(CXX)  $(SM2IMGFILES) -o $@ $(LIBS) $(CXXFLAGS) $(GZIP_LIBS) \
		$(TIFF_LIBS) $(PNM_LIBS) -limage -lm

smcat: $(SMCATFILES) $(SMLIB)
	$(CXX)  $(SMCATFILES) -o $@ $(LIBS) $(CXXFLAGS) $(GZIP_LIBS) \
		$(TIFF_LIBS) $(PNM_LIBS) -limage -lm

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

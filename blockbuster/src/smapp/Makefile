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

LIBS = $(LDFLAGS) -L$(INSTALL_DIR)/lib -lsmovie -lptutil  -levomovie $(IFL_LIBS) -lpthread -lm -ltiff -lpnmrw  -lz -llzo2 -llzma -limage -lexiv2 -lm $(GZIP_LIBS) $(TIFF_LIBS) $(PNG_LIBS) $(PNM_LIBS) -ljpeg  -lboost_regex -lboost_filesystem -lboost_thread -lboost_system -lboost_date_time -lreadline -ltermcap

SMLIB = $(INSTALL_DIR)/lib/libsmovie.a $(INSTALL_DIR)/lib/libptutil.a $(INSTALL_DIR)/lib/libsrwpng.a 
SFILES = smtag.C smquery.C smmake.C smtest.C ifl2sm.C img2sm.C rename.C smcat.C smevo.C debugutil.c
OFILES = ifl2sm img2sm rename  smcat smevo sminfo smmake smquery smtag  smtest 
TARGETS = $(INSTALL_DIR)/bin/smquery $(INSTALL_DIR)/bin/smtag $(INSTALL_DIR)/bin/smevo  $(INSTALL_DIR)/bin/smmake $(INSTALL_DIR)/bin/smtest $(INSTALL_DIR)/bin/ifl2sm $(INSTALL_DIR)/bin/img2sm $(INSTALL_DIR)/bin/mpg2img $(INSTALL_DIR)/bin/mpg2sm $(INSTALL_DIR)/bin/rename $(INSTALL_DIR)/bin/sm2img $(INSTALL_DIR)/bin/sminfo $(INSTALL_DIR)/bin/smcat $(INSTALL_DIR)/bin/smevo $(INSTALL_DIR)/bin/sm2qt $(INSTALL_DIR)/bin/ps2sm $(INSTALL_DIR)/bin/sm2mpg $(INSTALL_DIR)/bin/img2mpg  $(INSTALL_DIR)/bin/smrecolor
CPPFLAGS += -g 

default: 
	$(MAKE) -j 20 $(TARGETS) manpages	

$(INSTALL_DIR)/bin/%: %
	@echo '***********************************************'
	@echo INSTALLING $@
	cp -p $? $@
	@echo

$(INSTALL_DIR)/bin/sminfo: smquery
	rm $@; ln -s $^ $@

$(INSTALL_DIR)/bin/sm2img: $(INSTALL_DIR)/bin/smcat
	rm $@; ln -s $^ $@

manpages:
	mkdir -p $(INSTALL_DIR)/man/man1
	cp *.1 $(INSTALL_DIR)/man/man1

$(OFILES) : % : %.o $(SMLIB) simple_jpeg.o debugutil.o 
	@echo '***********************************************'
	@echo BUILDING $@
	@echo
	$(CXX)  $^ -o $@ $(CXXFLAGS) $(LIBS) 
	if [ "$(uname -s)" == Linux ]; then \
		 $(INSTALL_DIR)/bin/patchelf --set-rpath $(INSTALL_DIR)/lib $@ || rm -f $@ ; \
	fi
	@echo
	@echo DONE BUILDING $@
	@echo

clean:
	rm -rf $(CC_CACHE) *.o core $(TARGETS) sminfo

# use this carefully!  
depend: $(SFILES)
	makedepend -Y. -Y..  $(INCLUDES) $(SFILES)

# DO NOT DELETE

smtag.o: ../sm/sm.h ../sm/smLZO.h ../sm/smBase.h ../sm/smBaseP.h
smtag.o: ../RC_cpp_lib/stringutil.h ../RC_cpp_lib/timer.h
smtag.o: ../RC_cpp_lib/stringutil.h ../sm/smRLE.h ../sm/smGZ.h ../sm/smXZ.h
smtag.o: ../sm/smRaw.h ../sm/smJPG.h debugutil.h
smquery.o: ../sm/sm.h ../sm/smLZO.h ../sm/smBase.h ../sm/smBaseP.h
smquery.o: ../RC_cpp_lib/stringutil.h ../RC_cpp_lib/timer.h
smquery.o: ../RC_cpp_lib/stringutil.h ../sm/smRLE.h ../sm/smGZ.h ../sm/smXZ.h
smquery.o: ../sm/smRaw.h ../sm/smJPG.h debugutil.h ../libpng/pngsimple.h
smmake.o: ../sm/smRaw.h ../sm/smBase.h ../sm/smBaseP.h
smmake.o: ../RC_cpp_lib/stringutil.h ../RC_cpp_lib/timer.h
smmake.o: ../RC_cpp_lib/stringutil.h ../sm/smRLE.h
smtest.o: ../sm/smBase.h ../sm/smBaseP.h ../RC_cpp_lib/stringutil.h
smtest.o: ../RC_cpp_lib/timer.h ../RC_cpp_lib/stringutil.h
smtest.o: ../config/version.h
ifl2sm.o: ../sm/sm.h ../sm/smLZO.h ../sm/smBase.h ../sm/smBaseP.h
ifl2sm.o: ../RC_cpp_lib/stringutil.h ../RC_cpp_lib/timer.h
ifl2sm.o: ../RC_cpp_lib/stringutil.h ../sm/smRLE.h ../sm/smGZ.h ../sm/smXZ.h
ifl2sm.o: ../sm/smRaw.h ../sm/smJPG.h
img2sm.o: ../sm/sm.h ../sm/smLZO.h ../sm/smBase.h ../sm/smBaseP.h
img2sm.o: ../RC_cpp_lib/stringutil.h ../RC_cpp_lib/timer.h
img2sm.o: ../RC_cpp_lib/stringutil.h ../sm/smRLE.h ../sm/smGZ.h ../sm/smXZ.h
img2sm.o: ../sm/smRaw.h ../sm/smJPG.h ../libimage/sgilib.h
img2sm.o: ../libpnmrw/libpnmrw.h ../libpng/pngsimple.h simple_jpeg.h
img2sm.o: ../pt/pt.h
rename.o: ../sm/sm.h ../sm/smLZO.h ../sm/smBase.h ../sm/smBaseP.h
rename.o: ../RC_cpp_lib/stringutil.h ../RC_cpp_lib/timer.h
rename.o: ../RC_cpp_lib/stringutil.h ../sm/smRLE.h ../sm/smGZ.h ../sm/smXZ.h
rename.o: ../sm/smRaw.h ../sm/smJPG.h
smcat.o: ../sm/smRLE.h ../sm/smBase.h ../sm/smBaseP.h
smcat.o: ../RC_cpp_lib/stringutil.h ../RC_cpp_lib/timer.h
smcat.o: ../RC_cpp_lib/stringutil.h ../sm/smGZ.h ../sm/smLZO.h ../sm/smRaw.h
smcat.o: ../sm/smJPG.h ../sm/smXZ.h ../config/version.h ../pt/pt.h
smcat.o: ../libimage/sgilib.h ../libpnmrw/libpnmrw.h simple_jpeg.h
smevo.o: ../sm/smRLE.h ../sm/smBase.h ../sm/smBaseP.h
smevo.o: ../RC_cpp_lib/stringutil.h ../RC_cpp_lib/timer.h
smevo.o: ../RC_cpp_lib/stringutil.h ../sm/smGZ.h ../sm/smLZO.h ../sm/smRaw.h
smevo.o: ../sm/smJPG.h ../config/version.h
debugutil.o: debugutil.h

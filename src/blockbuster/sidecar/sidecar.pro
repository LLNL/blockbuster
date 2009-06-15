######################################################################
# Automatically generated by qmake (2.01a) Mon Apr 6 18:30:44 2009
######################################################################

LIBS +=  -LINSTALLDIR_PLACEHOLDER/lib -fPIC -LINSTALLDIR_PLACEHOLDER/lib -fPIC -LINSTALLDIR_PLACEHOLDER/lib -fPIC -LINSTALLDIR_PLACEHOLDER/lib   -lsmovie   
INCLUDEPATH += libdmx  .. INSTALLDIR_PLACEHOLDER/include 
MAKEFILE = Makefile.qt.include
QT += network 
DEFINES += DEBUG NO_BOOST
CONFIG += debug 
QMAKE_PRE_LINK  = export DYLD_IMAGE_SUFFIX=_debug
CONFIG -= release
TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += . ../../common

# Input
HEADERS = sidecar.h  Prefs.h RemoteControl.h \
    ../common.h \
           ../events.h \
    ../errmsg.h \
           ../Lockables.h \
           ../MovieCues.h \
           ../settings.h \
           ../../common/stringutil.h \
           ../timer.h
           
FORMS = ../BlockbusterControl.ui \
         blockbusterLaunchDialog.ui \
         ../InfoWindow.ui \
         ../MovieCueWidget.ui \
         sidecar.ui
SOURCES = ../common.cpp \
           ../events.cpp \
    ../errmsg.cpp \
            ../util.cpp \
           main.cpp \
           ../MovieCues.cpp \
           Prefs.C \
           RemoteControl.cpp \
           sidecar.cpp

RESOURCES = ../images.qrc

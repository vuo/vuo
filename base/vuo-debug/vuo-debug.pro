TEMPLATE = app
CONFIG -= qt app_bundle
CONFIG += console VuoFramework

VUO_INFO_PLIST = vuo-debug-Info.plist
VUO_INFO_PLIST_GENERATED = vuo-debug-Info-generated.plist
QMAKE_LFLAGS += -Wl,-sectcreate,__TEXT,__info_plist,$$VUO_INFO_PLIST_GENERATED

include(../../vuo.pri)

SOURCES += \
	vuo-debug.cc

LIBS += -rpath @loader_path/.

QMAKE_POST_LINK += cp vuo-debug ../../framework

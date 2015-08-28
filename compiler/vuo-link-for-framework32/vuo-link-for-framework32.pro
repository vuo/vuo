TEMPLATE = app
CONFIG -= qt app_bundle
CONFIG += console VuoFramework
TARGET = vuo-link

VUO_INFO_PLIST = vuo-link-Info.plist
VUO_INFO_PLIST_GENERATED = vuo-link-Info-generated.plist
QMAKE_LFLAGS += -Wl,-sectcreate,__TEXT,__info_plist,$$VUO_INFO_PLIST_GENERATED

include(../../vuo.pri)

SOURCES += \
	../vuo-link/vuo-link.cc

LIBS += -rpath @loader_path/../../../..

QMAKE_POST_LINK += cp vuo-link $$ROOT/framework/Vuo.framework/Versions/$$VUO_VERSION/MacOS

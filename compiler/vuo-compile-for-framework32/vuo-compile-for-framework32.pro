TEMPLATE = app
CONFIG -= qt app_bundle
CONFIG += console VuoFramework
TARGET = vuo-compile

VUO_INFO_PLIST = vuo-compile-Info.plist
VUO_INFO_PLIST_GENERATED = vuo-compile-Info-generated.plist
QMAKE_LFLAGS += -Wl,-sectcreate,__TEXT,__info_plist,$$VUO_INFO_PLIST_GENERATED

include(../../vuo.pri)

SOURCES += \
	../vuo-compile/vuo-compile.cc

LIBS += -rpath @loader_path/../../../..

FRAMEWORK_HELPERS_FOLDER = $$ROOT/framework/Vuo.framework/Versions/$$VUO_VERSION/Helpers
QMAKE_POST_LINK += \
	   mkdir -p "$$FRAMEWORK_HELPERS_FOLDER" \
	&& cp vuo-compile "$$FRAMEWORK_HELPERS_FOLDER"

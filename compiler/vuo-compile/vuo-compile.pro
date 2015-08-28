TEMPLATE = app
CONFIG -= qt app_bundle
CONFIG += console graphviz VuoLLVM VuoBase VuoRuntime VuoCompiler

VUO_INFO_PLIST = vuo-compile-Info.plist
VUO_INFO_PLIST_GENERATED = vuo-compile-Info-generated.plist
QMAKE_LFLAGS += -Wl,-sectcreate,__TEXT,__info_plist,$$VUO_INFO_PLIST_GENERATED

include(../../vuo.pri)
exists($$ROOT/licensetools/licensetools.pro) {include($$ROOT/licensetools/licensetools.pro)}

SOURCES += \
	vuo-compile.cc

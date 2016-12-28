TEMPLATE = app
CONFIG -= qt app_bundle
CONFIG += console graphviz VuoLLVM VuoBase VuoRuntime VuoCompiler
LIBS += -framework AppKit

VUO_INFO_PLIST = vuo-compile-Info.plist
VUO_INFO_PLIST_GENERATED = vuo-compile-Info-generated.plist
QMAKE_LFLAGS += -Wl,-sectcreate,__TEXT,__info_plist,$$VUO_INFO_PLIST_GENERATED

include(../../vuo.pri)
exists($$ROOT/licensetools/licensetools.pro) {include($$ROOT/licensetools/licensetools.pro)}

SOURCES += \
	vuo-compile.cc

coverage {
	QMAKE_POST_LINK += && install_name_tool -change @executable_path/../lib/libprofile_rt.dylib $$LLVM_ROOT/lib/libprofile_rt.dylib vuo-compile
}

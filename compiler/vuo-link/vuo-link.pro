TEMPLATE = app
CONFIG -= qt app_bundle
CONFIG += console VuoFramework
LIBS += -framework CoreFoundation

VUO_INFO_PLIST = vuo-link-Info.plist
VUO_INFO_PLIST_GENERATED = vuo-link-Info-generated.plist
QMAKE_LFLAGS += -Wl,-sectcreate,__TEXT,__info_plist,$$VUO_INFO_PLIST_GENERATED

include(../../vuo.pri)

SOURCES += \
	vuo-link.cc

LIBS += -rpath @loader_path/.

QMAKE_POST_LINK = true
coverage {
	QMAKE_POST_LINK += && install_name_tool -change @executable_path/../lib/libprofile_rt.dylib $$LLVM_ROOT/lib/libprofile_rt.dylib vuo-link
}
QMAKE_POST_LINK += && cp vuo-link ../../framework

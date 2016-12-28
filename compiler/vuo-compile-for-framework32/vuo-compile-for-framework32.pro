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

QMAKE_POST_LINK = true
coverage {
	QMAKE_POST_LINK += && install_name_tool -change @executable_path/../lib/libprofile_rt.dylib $$LLVM_ROOT/lib/libprofile_rt.dylib vuo-compile
}
QMAKE_POST_LINK += \
	&& mkdir -p "$$FRAMEWORK_HELPERS_FOLDER" \
   	&& cp vuo-compile "$$FRAMEWORK_HELPERS_FOLDER"

EDITOR_FOLDER = $$ROOT/editor/VuoEditorApp
exists($$EDITOR_FOLDER) {
	EDITOR_FRAMEWORK_HELPERS_FOLDER = $$EDITOR_FOLDER/Vuo\ Editor.app/Contents/Frameworks/Vuo.framework/Versions/$$VUO_VERSION/Helpers
	QMAKE_POST_LINK += && cp vuo-compile "$$EDITOR_FRAMEWORK_HELPERS_FOLDER"
}

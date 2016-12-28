TEMPLATE = app
CONFIG -= qt app_bundle
CONFIG += console VuoFramework
LIBS += -framework CoreFoundation
TARGET = vuo-link

VUO_INFO_PLIST = vuo-link-Info.plist
VUO_INFO_PLIST_GENERATED = vuo-link-Info-generated.plist
QMAKE_LFLAGS += -Wl,-sectcreate,__TEXT,__info_plist,$$VUO_INFO_PLIST_GENERATED

include(../../vuo.pri)

SOURCES += \
	../vuo-link/vuo-link.cc

LIBS += -rpath @loader_path/../../../..

FRAMEWORK_HELPERS_FOLDER = $$ROOT/framework/Vuo.framework/Versions/$$VUO_VERSION/Helpers

QMAKE_POST_LINK = true
coverage {
	QMAKE_POST_LINK += && install_name_tool -change @executable_path/../lib/libprofile_rt.dylib $$LLVM_ROOT/lib/libprofile_rt.dylib vuo-link
}
QMAKE_POST_LINK += \
	&& mkdir -p "$$FRAMEWORK_HELPERS_FOLDER" \
	&& cp vuo-link "$$FRAMEWORK_HELPERS_FOLDER"

EDITOR_FOLDER = $$ROOT/editor/VuoEditorApp
exists($$EDITOR_FOLDER) {
	EDITOR_FRAMEWORK_HELPERS_FOLDER = $$EDITOR_FOLDER/Vuo\ Editor.app/Contents/Frameworks/Vuo.framework/Versions/$$VUO_VERSION/Helpers
	QMAKE_POST_LINK += && cp vuo-link "$$EDITOR_FRAMEWORK_HELPERS_FOLDER"
}

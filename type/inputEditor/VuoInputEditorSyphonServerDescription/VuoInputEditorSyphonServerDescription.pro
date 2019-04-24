TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorSyphonServerDescription

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorSyphonServerDescription.cc

HEADERS += \
		VuoInputEditorSyphonServerDescription.hh

OTHER_FILES += \
		VuoInputEditorSyphonServerDescription.json

INCLUDEPATH += \
	$$ROOT/node \
	$$ROOT/node/vuo.data \
	$$ROOT/node/vuo.syphon \
	$$ROOT/type

LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/library/libVuoGlContext.dylib \
	$$ROOT/library/libVuoGlPool.dylib \
	$$ROOT/library/libVuoApp.dylib \
	$$ROOT/library/VuoImageRenderer.o \
	$$ROOT/node/vuo.syphon/VuoSyphon.o \
	$$ROOT/node/vuo.syphon/VuoSyphonSender.o \
	$$ROOT/node/vuo.syphon/VuoSyphonServerDescription.o \
	$$ROOT/node/vuo.syphon/VuoSyphonServerNotifier.o \
	$$ROOT/node/vuo.syphon/VuoSyphonListener.o \
	$$ROOT/node/vuo.syphon/VuoSyphonListener.o \
	$$ROOT/type/VuoBoolean.o \
	$$ROOT/type/VuoColor.o \
	$$ROOT/type/VuoMesh.o \
	$$ROOT/type/VuoImage.o \
	$$ROOT/type/VuoImageColorDepth.o \
	$$ROOT/type/VuoPoint3d.o \
	$$ROOT/type/VuoPoint4d.o \
	$$ROOT/type/VuoShader.o \
	$$ROOT/type/list/VuoList_VuoBoolean.o \
	$$ROOT/type/list/VuoList_VuoColor.o \
	$$ROOT/type/list/VuoList_VuoImageColorDepth.o \
	$$ROOT/type/list/VuoList_VuoPoint2d.o \
	$$ROOT/type/list/VuoList_VuoPoint3d.o \
	$$ROOT/type/list/VuoList_VuoPoint4d.o \
	$$ROOT/type/list/VuoList_VuoSyphonServerDescription.o \
	-framework AVFoundation \
	-framework CoreMedia \
	-framework CoreVideo \
	-framework IOKit \
	-framework IOSurface \
	-framework OpenGL \
	-F$$ROOT/node/vuo.syphon/Syphon \
	-framework Syphon

QMAKE_POST_LINK += \
	install_name_tool -change @loader_path/../Frameworks/Syphon.framework/Versions/A/Syphon @rpath/Vuo.framework/Frameworks/Syphon.framework/Syphon libVuoInputEditorSyphonServerDescription.dylib

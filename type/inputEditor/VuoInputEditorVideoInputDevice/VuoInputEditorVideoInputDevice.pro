TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorVideoInputDevice

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorVideoInputDevice.cc

HEADERS += \
		VuoInputEditorVideoInputDevice.hh

OTHER_FILES += \
		VuoInputEditorVideoInputDevice.json

INCLUDEPATH += \
	$$ROOT/node \
	$$ROOT/node/vuo.video \
	$$ROOT/type

LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/library/libVuoGlContext.dylib \
	$$ROOT/library/libVuoGlPool.dylib \
	$$ROOT/library/VuoApp.o \
	$$ROOT/library/VuoImageRenderer.o \
	$$ROOT/library/VuoOsStatus.o \
	$$ROOT/node/vuo.video/VuoQTCapture.o \
	$$ROOT/node/vuo.video/VuoQtListener.o \
	$$ROOT/node/vuo.video/VuoVideoInputDevice.o \
	$$ROOT/type/VuoBoolean.o \
	$$ROOT/type/VuoColor.o \
	$$ROOT/type/VuoMesh.o \
	$$ROOT/type/VuoImage.o \
	$$ROOT/type/VuoImageColorDepth.o \
	$$ROOT/type/VuoInteger.o \
	$$ROOT/type/VuoPoint2d.o \
	$$ROOT/type/VuoPoint3d.o \
	$$ROOT/type/VuoPoint4d.o \
	$$ROOT/type/VuoReal.o \
	$$ROOT/type/VuoShader.o \
	$$ROOT/type/VuoText.o \
	$$ROOT/type/list/VuoList_VuoBoolean.o \
	$$ROOT/type/list/VuoList_VuoColor.o \
	$$ROOT/type/list/VuoList_VuoImageColorDepth.o \
	$$ROOT/type/list/VuoList_VuoPoint2d.o \
	$$ROOT/type/list/VuoList_VuoPoint3d.o \
	$$ROOT/type/list/VuoList_VuoPoint4d.o \
	$$ROOT/type/list/VuoList_VuoReal.o \
	$$ROOT/type/list/VuoList_VuoVideoInputDevice.o \
	-framework CoreMediaIO \
	-framework CoreVideo \
	-framework IOKit \
	-framework IOSurface \
	-framework OpenGL \
	-framework QTKit \
	-framework QuartzCore

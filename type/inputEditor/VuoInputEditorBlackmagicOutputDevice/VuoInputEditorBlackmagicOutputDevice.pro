TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorBlackmagicOutputDevice

include(../../../vuo.pri)

SOURCES +=\
	VuoInputEditorBlackmagicOutputDevice.cc

HEADERS += \
	VuoInputEditorBlackmagicOutputDevice.hh

OTHER_FILES += \
	VuoInputEditorBlackmagicOutputDevice.json

INCLUDEPATH += \
	$$ROOT/node \
	$$ROOT/node/vuo.blackmagic \
	$$ROOT/node/vuo.blackmagic/premium \
	$$ROOT/node/vuo.video \
	$$ROOT/type

LIBS += \
	../widget/VuoComboBox.o \
	$$ROOT/library/VuoImageRenderer.o \
	$$ROOT/library/VuoOsStatus.o \
	$$ROOT/library/libVuoApp.dylib \
	$$ROOT/library/libVuoGlContext.dylib \
	$$ROOT/library/libVuoGlPool.dylib \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/node/vuo.blackmagic/VuoBlackmagicConnection.o \
	$$ROOT/node/vuo.blackmagic/VuoBlackmagicInputDevice.o \
	$$ROOT/node/vuo.blackmagic/VuoBlackmagicOutputDevice.o \
	$$ROOT/node/vuo.blackmagic/VuoBlackmagicVideoMode.o \
	$$ROOT/node/vuo.blackmagic/VuoDeinterlacing.o \
	$$ROOT/node/vuo.blackmagic/premium/VuoBlackmagicDevices.o \
	$$ROOT/node/vuo.blackmagic/premium/VuoBlackmagicInput.o \
	$$ROOT/node/vuo.blackmagic/premium/VuoBlackmagicOutput.o \
	$$ROOT/node/vuo.blackmagic/premium/VuoDeckLinkAPIDispatch.o \
	$$ROOT/type/VuoBoolean.o \
	$$ROOT/type/VuoColor.o \
	$$ROOT/type/VuoImage.o \
	$$ROOT/type/VuoImageColorDepth.o \
	$$ROOT/type/VuoMesh.o \
	$$ROOT/type/VuoPoint3d.o \
	$$ROOT/type/VuoPoint4d.o \
	$$ROOT/type/VuoReal.o \
	$$ROOT/type/VuoShader.o \
	$$ROOT/type/list/VuoList_VuoBlackmagicConnection.o \
	$$ROOT/type/list/VuoList_VuoBlackmagicInputDevice.o \
	$$ROOT/type/list/VuoList_VuoBlackmagicOutputDevice.o \
	$$ROOT/type/list/VuoList_VuoBlackmagicVideoMode.o \
	$$ROOT/type/list/VuoList_VuoBoolean.o \
	$$ROOT/type/list/VuoList_VuoColor.o \
	$$ROOT/type/list/VuoList_VuoDeinterlacing.o \
	$$ROOT/type/list/VuoList_VuoImageColorDepth.o \
	$$ROOT/type/list/VuoList_VuoPoint2d.o \
	$$ROOT/type/list/VuoList_VuoPoint3d.o \
	-framework IOSurface \
	-framework OpenGL

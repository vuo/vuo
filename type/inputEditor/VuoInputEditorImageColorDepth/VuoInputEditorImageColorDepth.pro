TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorImageColorDepth

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorImageColorDepth.cc

HEADERS += \
		VuoInputEditorImageColorDepth.hh

OTHER_FILES += \
		VuoInputEditorImageColorDepth.json

INCLUDEPATH += $$ROOT/type
LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/library/libVuoGlPool.dylib \
	$$ROOT/type/VuoImageColorDepth.o \
	$$ROOT/type/VuoText.o \
	$$ROOT/type/list/VuoList_VuoImageColorDepth.o

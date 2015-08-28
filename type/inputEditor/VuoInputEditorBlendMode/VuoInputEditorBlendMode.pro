TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorBlendMode

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorBlendMode.cc

HEADERS += \
		VuoInputEditorBlendMode.hh

OTHER_FILES += \
		VuoInputEditorBlendMode.json

LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/type/VuoBlendMode.o \
	$$ROOT/type/list/VuoList_VuoBlendMode.o

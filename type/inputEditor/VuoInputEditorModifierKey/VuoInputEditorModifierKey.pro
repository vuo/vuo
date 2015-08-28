TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorModifierKey

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorModifierKey.cc

HEADERS += \
		VuoInputEditorModifierKey.hh

OTHER_FILES += \
		VuoInputEditorModifierKey.json

LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/type/VuoModifierKey.o \
	$$ROOT/type/list/VuoList_VuoModifierKey.o

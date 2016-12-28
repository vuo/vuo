TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorKey

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorKey.cc

HEADERS += \
		VuoKeyComboBox.hh \
		VuoInputEditorKey.hh

OTHER_FILES += \
		VuoInputEditorKey.json

INCLUDEPATH += $$ROOT/node/vuo.keyboard
LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/node/vuo.keyboard/VuoKey.o \
	$$ROOT/type/VuoInteger.o \
	$$ROOT/type/VuoText.o \
	$$ROOT/type/list/VuoList_VuoKey.o

TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorIntegerRange

include(../../../vuo.pri)

SOURCES +=\
	VuoInputEditorIntegerRange.cc \
	$$ROOT/type/inputEditor/VuoInputEditorInteger/VuoSpinBox.cc

HEADERS += \
	VuoInputEditorIntegerRange.hh \
	$$ROOT/type/inputEditor/VuoInputEditorInteger/VuoSpinBox.hh

OTHER_FILES += \
	VuoInputEditorIntegerRange.json

INCLUDEPATH += \
	$$ROOT/type/inputEditor/VuoInputEditorInteger

LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/type/VuoIntegerRange.o \
	$$ROOT/type/VuoText.o \
	$$ROOT/type/VuoInteger.o \
	$$ROOT/type/VuoReal.o \
	$$ROOT/type/list/VuoList_VuoReal.o \
	$$ROOT/type/list/VuoList_VuoInteger.o

TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorReal

include(../../../vuo.pri)

SOURCES +=\
	VuoInputEditorReal.cc \
	VuoDoubleSpinBox.cc

HEADERS += \
	VuoInputEditorReal.hh \
	VuoDoubleSpinBox.hh

OTHER_FILES += \
	VuoInputEditorReal.json

LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/type/VuoInteger.o \
	$$ROOT/type/VuoReal.o \
	$$ROOT/type/VuoBoolean.o \
	$$ROOT/type/list/VuoList_VuoInteger.o \
	$$ROOT/type/list/VuoList_VuoReal.o \
	$$ROOT/type/list/VuoList_VuoBoolean.o \
	$$ROOT/type/VuoText.o

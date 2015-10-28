TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorPoint4d

include(../../../vuo.pri)

SOURCES +=\
	VuoInputEditorPoint4d.cc \
	$$ROOT/type/inputEditor/VuoInputEditorReal/VuoDoubleSpinBox.cc

HEADERS += \
	VuoInputEditorPoint4d.hh \
	$$ROOT/type/inputEditor/VuoInputEditorReal/VuoDoubleSpinBox.hh

OTHER_FILES += \
	VuoInputEditorPoint4d.json

INCLUDEPATH += $$ROOT/type/inputEditor/VuoInputEditorReal
LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/type/VuoPoint4d.o \
	$$ROOT/type/VuoInteger.o \
	$$ROOT/type/VuoReal.o \
	$$ROOT/type/list/VuoList_VuoReal.o \
	$$ROOT/type/VuoText.o

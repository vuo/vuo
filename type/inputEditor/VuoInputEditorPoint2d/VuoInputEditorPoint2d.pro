TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorPoint2d

include(../../../vuo.pri)

SOURCES +=\
	VuoInputEditorPoint2d.cc \
	$$ROOT/type/inputEditor/VuoInputEditorReal/VuoDoubleSpinBox.cc

HEADERS += \
	VuoInputEditorPoint2d.hh \
	$$ROOT/type/inputEditor/VuoInputEditorReal/VuoDoubleSpinBox.hh

OTHER_FILES += \
	VuoInputEditorPoint2d.json

INCLUDEPATH += $$ROOT/type/inputEditor/VuoInputEditorReal
LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/type/VuoPoint2d.o \
	$$ROOT/type/VuoInteger.o \
	$$ROOT/type/VuoReal.o \
	$$ROOT/type/list/VuoList_VuoReal.o \
	$$ROOT/type/VuoText.o

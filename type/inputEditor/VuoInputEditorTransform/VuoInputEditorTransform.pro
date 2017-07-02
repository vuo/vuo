TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorTransform

include(../../../vuo.pri)

SOURCES +=\
	VuoInputEditorTransform.cc \
	$$ROOT/type/inputEditor/VuoInputEditorReal/VuoDoubleSpinBox.cc

HEADERS += \
	VuoInputEditorTransform.hh \
	$$ROOT/type/inputEditor/VuoInputEditorReal/VuoDoubleSpinBox.hh

OTHER_FILES += \
	VuoInputEditorTransform.json

INCLUDEPATH += \
	$$ROOT/type/inputEditor/VuoInputEditorTransform \
	$$ROOT/type/inputEditor/VuoInputEditorReal

LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/type/VuoTransform.o \
	$$ROOT/type/VuoPoint2d.o \
	$$ROOT/type/VuoPoint3d.o \
	$$ROOT/type/VuoInteger.o \
	$$ROOT/type/VuoReal.o \
	$$ROOT/type/list/VuoList_VuoReal.o \
	$$ROOT/type/VuoText.o

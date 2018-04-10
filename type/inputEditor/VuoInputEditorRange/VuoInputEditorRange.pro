TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorRange

include(../../../vuo.pri)

SOURCES +=\
	VuoInputEditorRange.cc \
	$$ROOT/type/inputEditor/VuoInputEditorReal/VuoDoubleSpinBox.cc

HEADERS += \
	VuoInputEditorRange.hh \
	$$ROOT/type/inputEditor/VuoInputEditorReal/VuoDoubleSpinBox.hh

OTHER_FILES += \
	VuoInputEditorRange.json

INCLUDEPATH += \
	$$ROOT/type/inputEditor/VuoInputEditorReal

LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/type/VuoRange.o \
	$$ROOT/type/VuoText.o \
	$$ROOT/type/VuoInteger.o \
	$$ROOT/type/VuoBoolean.o \
	$$ROOT/type/VuoReal.o \
	$$ROOT/type/list/VuoList_VuoBoolean.o \
	$$ROOT/type/list/VuoList_VuoInteger.o \
	$$ROOT/type/list/VuoList_VuoReal.o

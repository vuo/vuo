TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorPoint3d

include(../../../vuo.pri)

SOURCES +=\
	VuoInputEditorPoint3d.cc \
	$$ROOT/type/inputEditor/VuoInputEditorReal/VuoDoubleSpinBox.cc

HEADERS += \
	VuoInputEditorPoint3d.hh \
	$$ROOT/type/inputEditor/VuoInputEditorReal/VuoDoubleSpinBox.hh

OTHER_FILES += \
	VuoInputEditorPoint3d.json

INCLUDEPATH += $$ROOT/type/inputEditor/VuoInputEditorReal
LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/type/VuoPoint3d.o \
	$$ROOT/type/VuoReal.o \
	$$ROOT/type/VuoText.o

TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorEdgeBlend

include(../../../vuo.pri)

SOURCES +=\
	VuoInputEditorEdgeBlend.cc \
	$$ROOT/type/inputEditor/VuoInputEditorReal/VuoDoubleSpinBox.cc

HEADERS += \
	VuoInputEditorEdgeBlend.hh \
	$$ROOT/type/inputEditor/VuoInputEditorReal/VuoDoubleSpinBox.hh

OTHER_FILES += \
	VuoInputEditorEdgeBlend.json

INCLUDEPATH += \
	$$ROOT/node/vuo.layer \
	$$ROOT/type/inputEditor/VuoInputEditorReal

LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/node/vuo.layer/VuoEdgeBlend.o \
	$$ROOT/type/VuoText.o \
	$$ROOT/type/VuoInteger.o \
	$$ROOT/type/VuoReal.o \
	$$ROOT/type/list/VuoList_VuoReal.o

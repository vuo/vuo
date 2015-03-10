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

LIBS += $$ROOT/type/VuoReal.o

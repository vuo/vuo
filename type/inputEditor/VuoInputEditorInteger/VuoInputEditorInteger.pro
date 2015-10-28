TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorInteger

include(../../../vuo.pri)

SOURCES +=\
	VuoInputEditorInteger.cc \
	VuoSpinBox.cc

HEADERS += \
	VuoInputEditorInteger.hh \
	VuoSpinBox.hh

OTHER_FILES += \
	VuoInputEditorInteger.json

LIBS += \
	$$ROOT/type/VuoInteger.o

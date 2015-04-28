TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorKey

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorKey.cc

HEADERS += \
		VuoKeyComboBox.hh \
		VuoInputEditorKey.hh

OTHER_FILES += \
		VuoInputEditorKey.json

INCLUDEPATH += $$ROOT/node/vuo.keyboard
LIBS += $$ROOT/node/vuo.keyboard/VuoKey.o

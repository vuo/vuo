TEMPLATE = lib
CONFIG += plugin qtCore qtGui json VuoPCH VuoInputEditor
TARGET = VuoInputEditorDispersion

include(../../../vuo.pri)

SOURCES +=\
		VuoInputEditorDispersion.cc

HEADERS += \
		VuoInputEditorDispersion.hh

OTHER_FILES += \
		VuoInputEditorDispersion.json

INCLUDEPATH += \
	$$ROOT/type \
	$$ROOT/node/vuo.scene

LIBS += \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/node/vuo.scene/VuoDispersion.o

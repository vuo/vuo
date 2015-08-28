TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.keyboard.button.c \
	vuo.keyboard.type.c

TYPE_SOURCES += \
	VuoKey.c

HEADERS += \
	VuoKey.h

NODE_LIBRARY_SOURCES += \
	VuoKeyboard.m

HEADERS += \
	VuoKeyboard.h

include(../../module.pri)

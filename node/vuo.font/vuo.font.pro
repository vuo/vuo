TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.font.make.c

TYPE_SOURCES += \
	VuoFont.c

HEADERS += \
	VuoFont.h

include(../../module.pri)

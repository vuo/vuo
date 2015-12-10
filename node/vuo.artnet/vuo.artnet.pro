TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

TYPE_SOURCES += \
	VuoArtNetInputDevice.c \
	VuoArtNetOutputDevice.c

HEADERS += \
	VuoArtNetInputDevice.h \
	VuoArtNetOutputDevice.h

include(../../module.pri)

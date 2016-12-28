TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

GENERIC_NODE_SOURCES += \
	vuo.data.areEqual.c \
	vuo.data.fetch.c \
	vuo.data.hold.c \
	vuo.data.hold.list.c \
	vuo.data.isGreaterThan.c \
	vuo.data.isLessThan.c \
	vuo.data.share.c \
	vuo.data.share.list.c \
	vuo.data.summarize.c

TYPE_SOURCES += \
	VuoData.c

HEADERS += \
	VuoData.h

NODE_INCLUDEPATH =+ \
	../vuo.artnet

include(../../module.pri)

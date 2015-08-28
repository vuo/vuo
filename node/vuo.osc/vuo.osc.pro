TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.osc.filter.address.c \
	vuo.osc.receive.c

GENERIC_NODE_SOURCES += \
	vuo.osc.message.get.1.c \
	vuo.osc.message.get.2.c \
	vuo.osc.message.get.3.c \
	vuo.osc.message.get.4.c

NODE_LIBRARY_SOURCES += \
	VuoOsc.cc

HEADERS += \
	VuoOsc.h

NODE_LIBRARY_INCLUDEPATH = \
	$$OSCPACK_ROOT/include

TYPE_SOURCES += \
	VuoOscMessage.c

HEADERS += \
	VuoOscMessage.h

include(../../module.pri)

TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.osc.filter.address.c \
	vuo.osc.find.input.name.c \
	vuo.osc.find.output.name.c \
	vuo.osc.get.input.c \
	vuo.osc.get.output.c \
	vuo.osc.listDevices.c \
	vuo.osc.make.input.c \
	vuo.osc.make.output.c \
	vuo.osc.make.output.ip.c \
	vuo.osc.receive.c \
	vuo.osc.receive2.c \
	vuo.osc.send.c

GENERIC_NODE_SOURCES += \
	vuo.osc.message.get.1.c \
	vuo.osc.message.get.2.c \
	vuo.osc.message.get.3.c \
	vuo.osc.message.get.4.c \
	vuo.osc.message.get.11.c \
	vuo.osc.message.make.1.c \
	vuo.osc.message.make.2.c \
	vuo.osc.message.make.3.c \
	vuo.osc.message.make.4.c \
	vuo.osc.message.make.11.c

NODE_LIBRARY_SOURCES += \
	VuoOsc.cc \
	VuoOscDevices.cc

SOURCES += \
	VuoOscDevices.cc

HEADERS += \
	VuoOsc.h

NODE_LIBRARY_INCLUDEPATH = \
	../vuo.font \
	$$OSCPACK_ROOT/include

TYPE_SOURCES += \
	VuoOscInputDevice.c \
	VuoOscOutputDevice.c \
	VuoOscMessage.c

HEADERS += \
	VuoOscInputDevice.h \
	VuoOscOutputDevice.h \
	VuoOscMessage.h

include(../../module.pri)

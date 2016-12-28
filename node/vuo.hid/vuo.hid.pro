TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.hid.filter.control.c \
	vuo.hid.find.name.c \
	vuo.hid.get.c \
	vuo.hid.get.control.c \
	vuo.hid.listDevices.c \
	vuo.hid.make.name.c \
	vuo.hid.receive.c \
	vuo.hid.scale.control.c

TYPE_SOURCES += \
	VuoHidControl.c \
	VuoHidDevice.c

NODE_LIBRARY_SOURCES += \
	VuoHidDevices.cc \
	VuoHidIo.cc \
	VuoHidUsage.c \
	VuoUsbVendor.c

SOURCES += \
	VuoHidDevices.cc \
	VuoHidIo.cc \
	VuoHidUsage.c \
	VuoUsbVendor.c

NODE_LIBRARY_INCLUDEPATH += \
	../vuo.font

HEADERS += \
	VuoHid.h \
	VuoHidControl.h \
	VuoHidDevice.h \
	VuoUsbVendor.h

include(../../module.pri)

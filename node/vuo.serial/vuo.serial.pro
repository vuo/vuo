TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.serial.configure.c \
	vuo.serial.find.name.c \
	vuo.serial.get.c \
	vuo.serial.listDevices.c \
	vuo.serial.make.name.c \
	vuo.serial.make.url.c \
	vuo.serial.receive.c \
	vuo.serial.send.c

NODE_INCLUDEPATH += \
	../vuo.data

TYPE_SOURCES += \
	VuoBaudRate.c \
	VuoParity.c \
	VuoSerialDevice.c

TYPE_INCLUDEPATH += \
	../vuo.data

NODE_LIBRARY_SOURCES += \
	VuoSerialDevices.cc \
	VuoSerialIO.cc

SOURCES += \
	VuoSerialDevices.cc \
	VuoSerialIO.cc

INCLUDEPATH += \
	../../node/vuo.font

NODE_LIBRARY_INCLUDEPATH += \
	../vuo.data

HEADERS += \
	VuoBaudRate.h \
	VuoParity.h \
	VuoSerial.h \
	VuoSerialDevice.h

include(../../module.pri)

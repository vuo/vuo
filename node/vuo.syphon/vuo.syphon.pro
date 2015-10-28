TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.syphon.find.server.name.c \
	vuo.syphon.find.server.app.c \
	vuo.syphon.listServers.c \
	vuo.syphon.get.serverDescription.c \
	vuo.syphon.make.serverDescription.c \
	vuo.syphon.send.c \
	vuo.syphon.receive.c

HEADERS += \
	VuoSyphonListener.h \
	VuoSyphonSender.h \
	VuoSyphonServerNotifier.h

NODE_LIBRARY_SOURCES += \
	VuoSyphon.m \
	VuoSyphonListener.m \
	VuoSyphonSender.m \
	VuoSyphonServerNotifier.m

HEADERS += \
	VuoSyphon.h

NODE_LIBRARY_INCLUDEPATH = \
	Syphon/Syphon.framework/Headers

TYPE_SOURCES += \
	VuoSyphonServerDescription.c

HEADERS += \
	VuoSyphonServerDescription.h

include(../../module.pri)

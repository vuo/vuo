TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.syphon.filter.serverDescription.serverName.c \
	vuo.syphon.filter.serverDescription.applicationName.c \
	vuo.syphon.listServers.c \
	vuo.syphon.get.serverDescription.c \
	vuo.syphon.make.serverDescription.c \
	vuo.syphon.send.c \
	vuo.syphon.receive.c

# OTHER_FILES instead of HEADERS, to avoid including in Vuo.framework
OTHER_FILES += \
	VuoSyphonListener.h \
	VuoSyphonSender.h \
	VuoSyphonServerNotifier.h

NODE_LIBRARY_SOURCES += \
	VuoSyphon.m \
	VuoSyphonListener.m \
	VuoSyphonSender.m \
	VuoSyphonServerNotifier.m

OTHER_FILES += \
	VuoSyphon.h

NODE_LIBRARY_INCLUDEPATH = \
	Syphon/Syphon.framework/Headers

TYPE_SOURCES += \
	VuoSyphonServerDescription.c

HEADERS += \
	VuoSyphonServerDescription.h

include(../../module.pri)

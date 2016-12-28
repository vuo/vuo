TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.ui.button.action.c \
	vuo.ui.drag.file.c \
	vuo.ui.get.drag.c

NODE_INCLUDEPATH += \
	../vuo.layer \
	../vuo.mouse

TYPE_SOURCES += \
	VuoDragEvent.c \
	VuoIconPosition.c

HEADERS += \
	VuoDragEvent.h \
	VuoIconPosition.h

include(../../module.pri)

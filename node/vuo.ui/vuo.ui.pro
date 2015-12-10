TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.ui.button.action.c

NODE_INCLUDEPATH += \
	../vuo.layer \
	../vuo.mouse

TYPE_SOURCES += \
	VuoIconPosition.c

HEADERS += \
	VuoIconPosition.h

include(../../module.pri)

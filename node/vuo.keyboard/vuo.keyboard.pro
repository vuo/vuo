TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.keyboard.button.c \
	vuo.keyboard.type.c

TYPE_SOURCES += \
	VuoKey.c

NODE_LIBRARY_SOURCES += \
	VuoKeyboard.m

# OTHER_FILES instead of HEADERS, to avoid including in Vuo.framework
OTHER_FILES += \
	VuoKey.h \
	VuoKeyboard.h

include(../../module.pri)

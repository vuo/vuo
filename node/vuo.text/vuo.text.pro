TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.text.append.c \
	vuo.text.areEqual.c \
	vuo.text.countCharacters.c \
	vuo.text.cut.c \
	vuo.text.format.number.c \
	vuo.text.populated.c

GENERIC_NODE_SOURCES += \
	vuo.text.split.c \
	vuo.text.split.stream.c

TYPE_SOURCES += \
	VuoNumberFormat.c

HEADERS += \
	VuoNumberFormat.h

include(../../module.pri)

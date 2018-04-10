TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.text.append.c \
	vuo.text.areEqual.c \
	vuo.text.case.c \
	vuo.text.countCharacters.c \
	vuo.text.cut.c \
	vuo.text.find.c \
	vuo.text.insert.c \
	vuo.text.make.ascii.c \
	vuo.text.make.controlCode.c \
	vuo.text.format.number.c \
	vuo.text.populated.c \
	vuo.text.removeHtml.c \
	vuo.text.replace.c

GENERIC_NODE_SOURCES += \
	vuo.text.split.c \
	vuo.text.split.stream.c

TYPE_SOURCES += \
	VuoControlCode.c \
	VuoNumberFormat.c

HEADERS += \
	VuoControlCode.h \
	VuoNumberFormat.h

include(../../module.pri)

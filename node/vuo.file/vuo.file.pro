TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.file.create.folder.c \
	vuo.file.list.c

HEADERS += \
	VuoFileFormat.h

TYPE_SOURCES += \
	VuoFileType.c

HEADERS += \
	VuoFileType.h

include(../../module.pri)

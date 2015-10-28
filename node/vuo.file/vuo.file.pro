TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.file.list.c

HEADERS += \
	VuoFileFormat.h

TYPE_SOURCES += \
	VuoFileType.c

HEADERS += \
	VuoFileType.h

NODE_LIBRARY_SOURCES += \
	VuoFileListSort.cc

HEADERS += \
	VuoFileListSort.h

include(../../module.pri)

TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.rss.fetch.c \
	vuo.rss.get.c

HEADERS += \
	VuoRssItem.h

TYPE_SOURCES += \
	VuoRssItem.c

NODE_INCLUDEPATH = \
	$${LIBXML2_ROOT}/include/libxml2

include(../../module.pri)

TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.rss.fetch.c \
	vuo.rss.find.author.c \
	vuo.rss.find.description.c \
	vuo.rss.find.title.c \
	vuo.rss.get.c

HEADERS += \
	VuoRssItem.h

TYPE_SOURCES += \
	VuoRssItem.c

TYPE_INCLUDEPATH = \
	$$ROOT/node/vuo.math \
	$$ROOT/node/vuo.time

NODE_INCLUDEPATH = \
	$$ROOT/node/vuo.math \
	$$ROOT/node/vuo.time \
	$${LIBXML2_ROOT}/include/libxml2

include(../../module.pri)

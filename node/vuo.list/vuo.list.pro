TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

GENERIC_NODE_SOURCES += \
	vuo.list.add.c \
	vuo.list.append.c \
	vuo.list.build.c \
	vuo.list.change.c \
	vuo.list.count.c \
	vuo.list.cut.c \
	vuo.list.cycle.c \
	vuo.list.enqueue.c \
	vuo.list.get.c \
	vuo.list.get.first.c \
	vuo.list.get.last.c \
	vuo.list.get.random.c \
	vuo.list.insert.c \
	vuo.list.populated.c \
	vuo.list.process.c \
	vuo.list.reverse.c \
	vuo.list.shuffle.c \
	vuo.list.spread.c \
	vuo.list.spread.group.c \
	vuo.list.take.c

TYPE_SOURCES += \
	VuoListPosition.c

HEADERS += \
	VuoListPosition.h

include(../../module.pri)

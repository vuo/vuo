TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

GENERIC_NODE_SOURCES += \
	vuo.list.add.c \
	vuo.list.append.c \
	vuo.list.count.c \
	vuo.list.cut.c \
	vuo.list.cycle.c \
	vuo.list.enqueue.c \
	vuo.list.get.c \
	vuo.list.get.first.c \
	vuo.list.get.last.c \
	vuo.list.get.random.c \
	vuo.list.insert.c \
	vuo.list.reverse.c \
	vuo.list.shuffle.c \
	vuo.list.take.c

include(../../module.pri)

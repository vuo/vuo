TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.table.add.column.c \
	vuo.table.add.row.c \
	vuo.table.fetch.c \
	vuo.table.format.c \
	vuo.table.make.text.c \
	vuo.table.take.column.c \
	vuo.table.take.row.c \
	vuo.table.transpose.c

GENERIC_NODE_SOURCES += \
	vuo.table.change.column.c \
	vuo.table.change.item.c \
	vuo.table.change.row.c \
	vuo.table.get.column.c \
	vuo.table.get.item.c \
	vuo.table.get.row.c \
	vuo.table.sort.c

NODE_INCLUDEPATH = \
	$$ROOT/node/vuo.list

TYPE_SOURCES += \
	VuoTable.cc \
	VuoTableFormat.c

HEADERS += \
	VuoTable.h \
	VuoTableFormat.h

TYPE_INCLUDEPATH = \
	$$ROOT/node/vuo.list \
	$$ROOT/node/vuo.time \
	$${LIBCSV_ROOT}/include

include(../../module.pri)

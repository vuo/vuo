TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.tree.fetch.json.c \
	vuo.tree.fetch.xml.c \
	vuo.tree.find.attribute.c \
	vuo.tree.find.content.c \
	vuo.tree.find.name.c \
	vuo.tree.find.xpath.c \
	vuo.tree.format.json.c \
	vuo.tree.format.xml.c \
	vuo.tree.get.c \
	vuo.tree.get.attribute.c \
	vuo.tree.get.attributes.c \
	vuo.tree.get.content.c \
	vuo.tree.make.c \
	vuo.tree.make.json.c \
	vuo.tree.make.xml.c

GENERIC_NODE_SOURCES += \
	vuo.tree.find.c

include(../../module.pri)

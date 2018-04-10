TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.layer.align.window.c \
	vuo.layer.arrange.column.c \
	vuo.layer.arrange.grid.c \
	vuo.layer.arrange.row.c \
	vuo.layer.blendMode.c \
	vuo.layer.bounds.rendered.c \
	vuo.layer.combine.c \
	vuo.layer.combine.center.c \
	vuo.layer.copy.c \
	vuo.layer.copy.trs.c \
	vuo.layer.drag.c \
	vuo.layer.get.edgeBlend.c \
	vuo.layer.get.c \
	vuo.layer.get.child.c \
	vuo.layer.make.c \
	vuo.layer.make.color.c \
	vuo.layer.make.edgeBlend.c \
	vuo.layer.make.gradient.linear.c \
	vuo.layer.make.gradient.radial.c \
	vuo.layer.make.oval.c \
	vuo.layer.make.quad.c \
	vuo.layer.make.realSize.c \
	vuo.layer.make.realSize.shadow.c \
	vuo.layer.make.roundedRectangle.c \
	vuo.layer.make.shadow.c \
	vuo.layer.make.stretched.c \
	vuo.layer.make.text.c \
	vuo.layer.populated.c \
	vuo.layer.render.image.c \
	vuo.layer.render.window.c \
	vuo.layer.tile.c \
	vuo.layer.transform.c \
	vuo.layer.within.c

TYPE_SOURCES += \
	VuoLayer.c \
	VuoProjectionType.c \
	VuoEdgeBlend.c \
	VuoRenderedLayers.c

HEADERS += \
	VuoLayer.h \
	VuoProjectionType.h \
	VuoEdgeBlend.h \
	VuoRenderedLayers.h

NODE_INCLUDEPATH += \
	../vuo.mouse \
	../vuo.scene

include(../../module.pri)

TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.scene.arrange.grid.c \
	vuo.scene.back.c \
	vuo.scene.bend.c \
	vuo.scene.blendMode.c \
	vuo.scene.bounds.c \
	vuo.scene.combine.c \
	vuo.scene.copy.c \
	vuo.scene.copy.trs.c \
	vuo.scene.displace.image.c \
	vuo.scene.divide.c \
	vuo.scene.explode.c \
	vuo.scene.facet.c \
	vuo.scene.fetch.c \
	vuo.scene.fetch.list.c \
	vuo.scene.make.c \
	vuo.scene.make.camera.drag.c \
	vuo.scene.make.camera.orthographic.c \
	vuo.scene.make.camera.orthographic.target.c \
	vuo.scene.make.camera.perspective.c \
	vuo.scene.make.camera.perspective.target.c \
	vuo.scene.make.cube.c \
	vuo.scene.make.image.c \
	vuo.scene.make.image.unlit.c \
	vuo.scene.make.light.ambient.c \
	vuo.scene.make.light.point.c \
	vuo.scene.make.light.spot.c \
	vuo.scene.make.light.spot.target.c \
	vuo.scene.normalize.c \
	vuo.scene.noise.c \
	vuo.scene.pinch.c \
	vuo.scene.populated.c \
	vuo.scene.render.image.c \
	vuo.scene.render.window.c \
	vuo.scene.ripple.c \
	vuo.scene.shader.all.c \
	vuo.scene.shader.material.c \
	vuo.scene.skew.c \
	vuo.scene.spike.c \
	vuo.scene.trim.c \
	vuo.scene.twirl.c

GENERIC_NODE_SOURCES += \
	vuo.scene.make.grid.lines.c \
	vuo.scene.make.grid.points.c \
	vuo.scene.make.random.points.c \
	vuo.scene.make.sphere.c \
	vuo.scene.make.cone.c \
	vuo.scene.make.square.c \
	vuo.scene.make.cube.1.c \
	vuo.scene.make.triangle.c \
	vuo.scene.make.icosphere.c \
	vuo.scene.make.torus.c \
	vuo.scene.make.tube.c

NODE_INCLUDEPATH += \
	. \
	../vuo.noise \
	$${ASSIMP_ROOT}/include

HEADERS += \
	VuoDispersion.h \
	VuoDisplacement.h \
	VuoDistribution3d.h \
	VuoGridType.h \
	VuoMultisample.h

TYPE_SOURCES += \
	VuoDispersion.c \
	VuoDisplacement.c \
	VuoDistribution3d.c \
	VuoGridType.c \
	VuoMultisample.c

include(../../module.pri)

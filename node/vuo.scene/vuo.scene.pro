TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.scene.arrange.grid.c \
	vuo.scene.back.c \
	vuo.scene.bend.c \
	vuo.scene.bounds.c \
	vuo.scene.combine.c \
	vuo.scene.copy.c \
	vuo.scene.copy.trs.c \
	vuo.scene.divide.c \
	vuo.scene.explode.c \
	vuo.scene.facet.c \
	vuo.scene.get.c \
	vuo.scene.make.c \
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
	vuo.scene.render.image.c \
	vuo.scene.render.window.c \
	vuo.scene.ripple.c \
	vuo.scene.shader.set.all.c \
	vuo.scene.shader.set.material.c \
	vuo.scene.skew.c \
	vuo.scene.spike.c \
	vuo.scene.trim.c \
	vuo.scene.twirl.c

NODE_INCLUDEPATH += \
	$${ASSIMP_ROOT}/include

HEADERS += \
	VuoDispersion.h \
	VuoDisplacement.h

TYPE_SOURCES += \
	VuoDispersion.c \
	VuoDisplacement.c

include(../../module.pri)

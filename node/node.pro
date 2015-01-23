TEMPLATE = subdirs

include(../vuo.pri)

HEADERS += \
	module.h \
	node.h

SUBDIRS += \
	vuo.color \
	vuo.console \
	vuo.event \
	vuo.font \
	vuo.hold \
	vuo.image \
	vuo.kinect \
	vuo.layer \
	vuo.leap \
	vuo.list \
	vuo.logic \
	vuo.math \
	vuo.midi \
	vuo.mouse \
	vuo.movie \
	vuo.noise \
	vuo.osc \
	vuo.point \
	vuo.quaternion \
	vuo.scene \
	vuo.select \
	vuo.shader \
	vuo.syphon \
	vuo.text \
	vuo.time \
	vuo.transform \
	vuo.type \
	vuo.vertices

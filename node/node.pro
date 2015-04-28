TEMPLATE = subdirs

include(../vuo.pri)

HEADERS += \
	module.h \
	node.h

SUBDIRS += \
	vuo.audio \
	vuo.color \
	vuo.console \
	vuo.data \
	vuo.event \
	vuo.font \
	vuo.hold \
	vuo.image \
	vuo.keyboard \
	vuo.kinect \
	vuo.layer \
	vuo.leap \
	vuo.list \
	vuo.logic \
	vuo.math \
	vuo.midi \
	vuo.motion \
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
	vuo.vertices \
	vuo.window

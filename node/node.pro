TEMPLATE = subdirs

include(../vuo.pri)

HEADERS += \
	module.h \
	node.h

SUBDIRS += \
	vuo.app \
	vuo_artnet \
	vuo.audio \
	vuo.bcf2000 \
	vuo.color \
	vuo.console \
	vuo.data \
	vuo.dictionary \
	vuo.event \
	vuo.file \
	vuo.font \
	vuo.hid \
	vuo_image \
	vuo.keyboard \
	vuo.kinect \
	vuo.layer \
	vuo.leap \
	vuo.list \
	vuo.logic \
	vuo.math \
	vuo.mesh \
	vuo.midi \
	vuo.motion \
	vuo.mouse \
	vuo.noise \
	vuo.osc \
	vuo.point \
	vuo.quaternion \
	vuo.rss \
	vuo_scene \
	vuo.screen \
	vuo.select \
	vuo.serial \
	vuo.shader \
	vuo.syphon \
	vuo.text \
	vuo.time \
	vuo.transform \
	vuo.type \
	vuo.ui \
	vuo.url \
	vuo.video \
	vuo.window

vuo_artnet.subdir = vuo.artnet
exists(vuo.artnet/premium) {
	SUBDIRS += vuo_artnet_premium
	vuo_artnet_premium.subdir = vuo.artnet/premium
	vuo_artnet.depends = vuo_artnet_premium
}

vuo_image.subdir = vuo.image
exists(vuo.image/premium) {
	SUBDIRS += vuo_image_premium
	vuo_image_premium.subdir = vuo.image/premium
	vuo_image.depends = vuo_image_premium
}

vuo_scene.subdir = vuo.scene
exists(vuo.scene/premium) {
	SUBDIRS += vuo_scene_premium
	vuo_scene_premium.subdir = vuo.scene/premium
	vuo_scene.depends = vuo_scene_premium
}

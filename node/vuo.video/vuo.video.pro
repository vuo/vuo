TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.video.decodeImage.c \
	vuo.video.info.c \
	vuo.video.listDevices.c \
	vuo.video.make.input.c \
	vuo.video.play.c \
	vuo.video.receive.c

NODE_LIBRARY_SOURCES += \
	VuoMovie.cc \
	VuoQTCapture.mm \
	VuoQtListener.m

SOURCES += \
	VuoMovie.cc

HEADERS += \
	VuoMovie.h \
	VuoQTCapture.h \
	VuoQtListener.h

HEADERS += \
	VuoVideoFrame.h \
	VuoVideoInputDevice.h

TYPE_SOURCES += \
	VuoVideoFrame.c \
	VuoVideoInputDevice.c

NODE_LIBRARY_INCLUDEPATH = \
	../vuo.font \
	$${FFMPEG_ROOT}/include \
	$${FFMPEG_ROOT}/include/libavcodec \
	$${FFMPEG_ROOT}/include/libavformat \
	$${FFMPEG_ROOT}/include/libavutil \
	$${FFMPEG_ROOT}/include/libswscale

include(../../module.pri)

VuoCompileLibraries(
	VuoAvDecoder.cc
	VuoAvPlayerInterface.m
	VuoAvPlayerObject.m
	VuoAvWriter.mm
	VuoAvWriterObject.m
	VuoFfmpegDecoder.cc
	VuoVideo.cc
	VuoVideoCapture.mm
	VuoVideoPlayer.cc
)
target_include_directories(vuo.video.libraries
	PRIVATE
		${CONAN_FRAMEWORK_DIRS_HAP}/HapInAVFoundation.framework/Headers
)
target_link_libraries(vuo.video.libraries
	PUBLIC
		"-framework Cocoa"
		"-framework AVFoundation"
		"-framework CoreMedia"
		"-framework CoreMediaIO"
		"-framework CoreVideo"
		"-framework OpenGL"
		"-framework VideoToolbox"
		CONAN_PKG::ffmpeg
		VuoApp
		VuoGlContext
		VuoGlPool
		vuo.hid.libraries
)

VuoCompileTypes(
	VuoAudioEncoding.c
	VuoAudioFrame.c
	VuoMovieFormat.c
	VuoMovieImageEncoding.c
	VuoVideoFrame.c
	VuoVideoInputDevice.c
	VuoVideoOptimization.c
)

VuoCompileNodes(
	vuo.video.decodeImage.c
	vuo.video.get.audioframe.c
	vuo.video.get.videoframe.c
	vuo.video.info.c
	vuo.video.listDevices.c
	vuo.video.make.audioframe.c
	vuo.video.make.format.c
	vuo.video.make.input.c
	vuo.video.make.videoframe.c
	vuo.video.play.c
	vuo.video.receive.c
	vuo.video.receive2.c
	vuo.video.save.c
	vuo.video.save2.c
	vuo.video.send.rtmp.c
	vuo.video.step.c
)
target_link_libraries(vuo.video.nodes
	PUBLIC
		CONAN_PKG::ffmpeg
)

VuoNodeSet()

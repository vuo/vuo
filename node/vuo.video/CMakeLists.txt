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
target_sources(vuo.video.libraries PRIVATE
	VuoAvDecoder.hh
	VuoAvPlayerInterface.h
	VuoAvPlayerObject.h
	VuoAvWriter.h
	VuoAvWriterObject.h
	VuoFfmpegDecoder.hh
	VuoFfmpegUtility.hh
	VuoVideo.h
	VuoVideoCapture.h
	VuoVideoDecoder.hh
	VuoVideoPlayer.hh
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
target_sources(vuo.video.types PRIVATE
	VuoAudioEncoding.h
	VuoAudioFrame.h
	VuoMovieFormat.h
	VuoMovieImageEncoding.h
	VuoVideoFrame.h
	VuoVideoInputDevice.h
	VuoVideoOptimization.h
)

VuoCompileCompoundTypes(
	VuoList_VuoAudioEncoding.cc
	VuoList_VuoMovieImageEncoding.cc
	VuoList_VuoVideoInputDevice.cc
	VuoList_VuoVideoOptimization.cc
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

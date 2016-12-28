QMAKE_CXXFLAGS += \
	$(INCPATH) \
	$(DEFINES) \
	-I$$ROOT/base \
	-I$$ROOT/library \
	-I$$ROOT/node \
	-I$$ROOT/node/vuo.artnet \
	-I$$ROOT/node/vuo.audio \
	-I$$ROOT/node/vuo.data \
	-I$$ROOT/node/vuo.font \
	-I$$ROOT/node/vuo.layer \
	-I$$ROOT/node/vuo.math \
	-I$$ROOT/node/vuo.midi \
	-I$$ROOT/node/vuo.scene \
	-I$$ROOT/node/vuo.time \
	-I$$ROOT/node/vuo.video \
	-I$$ROOT/runtime \
	-I$$ROOT/type \
	-I$$ROOT/type/list \
	$$QMAKE_CXXFLAGS_WARN_ON

QMAKE_LFLAGS += \
	$$ZMQ_ROOT/lib/libzmq.a \
	$${GRAPHVIZ_ROOT}/lib/libgvc.dylib \
	$${GRAPHVIZ_ROOT}/lib/libgraph.dylib \
	$${GRAPHVIZ_ROOT}/lib/graphviz/libgvplugin_dot_layout.dylib \
	$${GRAPHVIZ_ROOT}/lib/graphviz/libgvplugin_core.dylib \
	$$MUPARSER_ROOT/lib/libmuparser.a \
	$$FFMPEG_ROOT/lib/libavcodec.dylib \
	$$FFMPEG_ROOT/lib/libavformat.dylib \
	$$FFMPEG_ROOT/lib/libavutil.dylib \
	$$FFMPEG_ROOT/lib/libswresample.dylib \
	$$FFMPEG_ROOT/lib/libswscale.dylib \
	$$FREEIMAGE_ROOT/lib/libFreeImage.a \
	$$CURL_ROOT/lib/libcurl.a \
	-lssl \
	-lcrypto \
	-lobjc \
	-framework AVFoundation \
	-framework Accelerate \
	-framework Carbon \
	-framework CoreMedia \
	-framework Foundation \
	-framework QuartzCore \
	-framework AppKit \
	$$ROOT/runtime/libVuoRuntime.bc \
	$$ROOT/runtime/libVuoRuntimeContext.bc \
	$$ROOT/runtime/libVuoRuntimeHelper.bc \
	$$ROOT/runtime/libVuoEventLoop.bc \
	$$ROOT/base/VuoCompositionStub.o \
	$$ROOT/base/VuoTelemetry.o \
	$$ROOT/library/libVuoGlContext.dylib \
	$$ROOT/library/libVuoGlPool.dylib \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/library/VuoBase64.o \
	$$ROOT/library/VuoCglPixelFormat.o \
	$$ROOT/library/VuoDisplayRefresh.o \
	$$ROOT/library/VuoImageBlur.o \
	$$ROOT/library/VuoImageGet.o \
	$$ROOT/library/VuoImageMapColors.o \
	$$ROOT/library/VuoImageRenderer.o \
	$$ROOT/library/VuoImageResize.o \
	$$ROOT/library/VuoImageText.o \
	$$ROOT/library/VuoMeshParametric.o \
	$$ROOT/library/VuoMeshUtility.o \
	$$ROOT/library/VuoMathExpressionParser.o \
	$$ROOT/library/VuoOsStatus.o \
	$$ROOT/library/VuoSceneRenderer.o \
	$$ROOT/library/VuoScreenCommon.o \
	$$ROOT/library/VuoUrlFetch.o \
	$$ROOT/library/VuoUrlParser.o \
	$$ROOT/library/VuoWindow.o \
	$$ROOT/library/VuoWindowOpenGLInternal.o \
	$$ROOT/library/VuoWindowRecorder.o \
	$$ROOT/library/VuoWindowTextInternal.o \
	$$ROOT/type/VuoAudioSamples.o \
	$$ROOT/type/VuoBoolean.o \
	$$ROOT/type/VuoBlendMode.o \
	$$ROOT/type/VuoColor.o \
	$$ROOT/type/VuoCoordinateUnit.o \
	$$ROOT/type/VuoImage.o \
	$$ROOT/type/VuoImageColorDepth.o \
	$$ROOT/type/VuoInteger.o \
	$$ROOT/node/vuo.artnet/VuoArtNetInputDevice.o \
	$$ROOT/node/vuo.artnet/VuoArtNetOutputDevice.o \
	$$ROOT/node/vuo.data/VuoData.o \
	$$ROOT/node/vuo.font/VuoFont.o \
	$$ROOT/node/vuo.layer/VuoLayer.o \
	$$ROOT/node/vuo.midi/VuoMidiNote.o \
	$$ROOT/node/vuo.noise/VuoGradientNoiseCommon.bc \
	$$ROOT/node/vuo.time/VuoRelativeTime.o \
	$$ROOT/node/vuo.time/VuoTime.o \
	$$ROOT/node/vuo.time/VuoTimeUnit.o \
	$$ROOT/node/vuo.video/VuoAvDecoder.o \
	$$ROOT/node/vuo.video/VuoAvPlayerInterface.o \
	$$ROOT/node/vuo.video/VuoAvPlayerObject.o \
	$$ROOT/node/vuo.video/VuoFfmpegDecoder.o \
	$$ROOT/node/vuo.video/VuoVideo.o \
	$$ROOT/node/vuo.video/VuoVideoFrame.o \
	$$ROOT/node/vuo.video/VuoVideoPlayer.o \
	$$ROOT/type/VuoCursor.o \
	$$ROOT/type/VuoDictionary_VuoText_VuoReal.o \
	$$ROOT/type/VuoMesh.o \
	$$ROOT/type/VuoMathExpressionList.o \
	$$ROOT/type/VuoPoint2d.o \
	$$ROOT/type/VuoPoint3d.o \
	$$ROOT/type/VuoPoint4d.o \
	$$ROOT/type/VuoReal.o \
	$$ROOT/type/VuoSceneObject.o \
	$$ROOT/type/VuoScreen.o \
	$$ROOT/type/VuoShader.o \
	$$ROOT/type/VuoText.o \
	$$ROOT/type/VuoTransform.o \
	$$ROOT/type/VuoTransform2d.o \
	$$ROOT/type/VuoUrl.o \
	$$ROOT/type/VuoWindowProperty.o \
	$$ROOT/type/VuoWindowReference.o \
	$$ROOT/type/list/VuoList_VuoAudioSamples.o \
	$$ROOT/type/list/VuoList_VuoBoolean.o \
	$$ROOT/type/list/VuoList_VuoBlendMode.o \
	$$ROOT/type/list/VuoList_VuoColor.o \
	$$ROOT/type/list/VuoList_VuoCoordinateUnit.o \
	$$ROOT/type/list/VuoList_VuoCursor.o \
	$$ROOT/type/list/VuoList_VuoImage.o \
	$$ROOT/type/list/VuoList_VuoImageColorDepth.o \
	$$ROOT/type/list/VuoList_VuoInteger.o \
	$$ROOT/type/list/VuoList_VuoLayer.o \
	$$ROOT/type/list/VuoList_VuoPoint2d.o \
	$$ROOT/type/list/VuoList_VuoPoint3d.o \
	$$ROOT/type/list/VuoList_VuoPoint4d.o \
	$$ROOT/type/list/VuoList_VuoReal.o \
	$$ROOT/type/list/VuoList_VuoScreen.o \
	$$ROOT/type/list/VuoList_VuoSceneObject.o \
	$$ROOT/type/list/VuoList_VuoText.o \
	$$ROOT/type/list/VuoList_VuoTime.o \
	$$ROOT/type/list/VuoList_VuoTimeUnit.o \
	$$ROOT/type/list/VuoList_VuoWindowProperty.o \
	$$QMAKE_LFLAGS $$LIBS $$QMAKE_LIBS \
	-Wl,-rpath,$$ROOT/framework \
	"-framework QtTest" \
	"-framework QtCore" \
	"-framework IOSurface" \
	"-framework OpenGL"

QMAKE_CXXFLAGS += \
	$(INCPATH) \
	$(DEFINES) \
	-I$$ROOT/base \
	-I$$ROOT/library \
	-I$$ROOT/node \
	-I$$ROOT/node/vuo.midi \
	-I$$ROOT/node/vuo.movie \
	-I$$ROOT/runtime \
	-I$$ROOT/type \
	-I$$ROOT/type/list \
	-I$$ICU_ROOT/include \
	$$QMAKE_CXXFLAGS_WARN_ON

QMAKE_LFLAGS += \
	$$ICU_ROOT/lib/libicuuc.a \
	$$ICU_ROOT/lib/libicudata.a \
	$$ZMQ_ROOT/lib/libzmq.a \
	$${GRAPHVIZ_ROOT}/lib/libgvc.dylib \
	$${GRAPHVIZ_ROOT}/lib/libgraph.dylib \
	$${GRAPHVIZ_ROOT}/lib/graphviz/libgvplugin_dot_layout.dylib \
	$${GRAPHVIZ_ROOT}/lib/graphviz/libgvplugin_core.dylib \
	$$MUPARSER_ROOT/lib/libmuparser.a \
	$$FFMPEG_ROOT/lib/libavcodec.dylib \
	$$FFMPEG_ROOT/lib/libavformat.dylib \
	$$FFMPEG_ROOT/lib/libavutil.dylib \
	$$FFMPEG_ROOT/lib/libswscale.dylib \
	-lobjc \
	-framework Foundation \
	$$ROOT/runtime/VuoHeap.cc \
	$$ROOT/base/VuoRuntime.o \
	$$ROOT/base/VuoCompositionStub.o \
	$$ROOT/base/VuoTelemetry.o \
	$$ROOT/library/libVuoGlContext.dylib \
	$$ROOT/library/libVuoGlPool.dylib \
	$$ROOT/library/VuoImageRenderer.o \
	$$ROOT/library/VuoSceneRenderer.o \
	$$ROOT/library/VuoVerticesParametric.o \
	$$ROOT/type/VuoBoolean.o \
	$$ROOT/type/VuoColor.o \
	$$ROOT/type/VuoImage.o \
	$$ROOT/type/VuoInteger.o \
	$$ROOT/node/vuo.midi/VuoMidiNote.o \
	$$ROOT/node/vuo.movie/VuoMovie.o \
	$$ROOT/type/VuoPoint2d.o \
	$$ROOT/type/VuoPoint3d.o \
	$$ROOT/type/VuoPoint4d.o \
	$$ROOT/type/VuoReal.o \
	$$ROOT/type/VuoSceneObject.o \
	$$ROOT/type/VuoShader.o \
	$$ROOT/type/VuoText.o \
	$$ROOT/type/VuoTransform.o \
	$$ROOT/type/VuoTransform2d.o \
	$$ROOT/type/VuoVertices.o \
	$$ROOT/type/list/VuoList_VuoImage.o \
	$$ROOT/type/list/VuoList_VuoInteger.o \
	$$ROOT/type/list/VuoList_VuoPoint2d.o \
	$$ROOT/type/list/VuoList_VuoPoint3d.o \
	$$ROOT/type/list/VuoList_VuoSceneObject.o \
	$$ROOT/type/list/VuoList_VuoText.o \
	$$ROOT/type/list/VuoList_VuoVertices.o \
	$$QMAKE_LFLAGS $$LIBS $$QMAKE_LIBS \
	-Wl,-rpath,$$ROOT/framework \
	"-framework QtTest" \
	"-framework QtCore" \
	"-framework IOSurface" \
	"-framework OpenGL"

TEMPLATE = aux
CONFIG += VuoPCH

include(../vuo.pri)

NODE_LIBRARY_SOURCES += \
	VuoBase64.cc \
	VuoCglPixelFormat.c \
	VuoDisplayRefresh.c \
	VuoIoReturn.c \
	VuoImageResize.c \
	VuoImageBlend.c \
	VuoImageBlur.c \
	VuoImageGet.cc \
	VuoImageMapColors.c \
	VuoImageRenderer.cc \
	VuoImageText.cc \
	VuoSceneObjectGet.c \
	VuoSceneObjectRenderer.cc \
	VuoSceneRenderer.cc \
	VuoUrlFetch.c \
	VuoMathExpressionParser.cc \
	VuoMeshParametric.cc \
	VuoMeshUtility.cc \
	VuoOsStatus.c \
	VuoPointsParametric.cc \
	VuoScreenCapture.m \
	VuoScreenCommon.m \
	VuoWindow.m \
	VuoWindowDrag.mm \
	VuoWindowOpenGLInternal.m \
	VuoWindowRecorder.m \
	VuoWindowTextInternal.m \
	VuoUrlParser.c

SOURCES += \
	VuoBase64.cc \
	VuoCglPixelFormat.c \
	VuoDisplayRefresh.c \
	VuoIoReturn.c \
	VuoImageResize.c \
	VuoImageBlend.c \
	VuoImageBlur.c \
	VuoImageGet.cc \
	VuoImageMapColors.c \
	VuoImageRenderer.cc \
	VuoImageText.cc \
	VuoMathExpressionParser.cc \
	VuoMeshParametric.cc \
	VuoMeshUtility.cc \
	VuoOsStatus.c \
	VuoPointsParametric.cc \
	VuoSceneObjectRenderer.cc \
	VuoSceneRenderer.cc \
	VuoUrlFetch.c \
	VuoUrlParser.c

OBJECTIVE_SOURCES += \
	VuoScreenCommon.m \
	VuoWindow.m \
	VuoWindowOpenGLInternal.m \
	VuoWindowRecorder.m \
	VuoWindowTextInternal.m

HEADERS += \
	VuoBase64.h \
	VuoCglPixelFormat.h \
	VuoDisplayRefresh.h \
	VuoGlContext.h \
	VuoGlPool.h \
	VuoHeap.h \
	VuoIoReturn.h \
	VuoImageGet.h \
	VuoImageBlend.h \
	VuoImageBlur.h \
	VuoImageRenderer.h \
	VuoLog.h \
	VuoMathExpressionParser.h \
	VuoMeshParametric.h \
	VuoMeshUtility.h \
	VuoOsStatus.h \
	VuoPointsParametric.h \
	VuoPool.hh \
	VuoSceneObjectGet.h \
	VuoSceneObjectRenderer.h \
	VuoSceneRenderer.h \
	VuoScreenCapture.h \
	VuoScreenCommon.h \
	VuoSmooth.h \
	VuoUrlFetch.h \
	VuoWindow.h \
	VuoUrlParser.h

OTHER_FILES += \
	VuoImageResize.h \
	VuoImageText.h \
	VuoImageWatermark.h \
	VuoTriggerSet.hh \
	VuoWindowOpenGLInternal.h \
	VuoWindowRecorder.h \
	VuoWindowTextInternal.h

exists($$ROOT/library/premium) {
	DEFINES += LIBRARY_PREMIUM_AVAILABLE
	OTHER_FILES += premium/VuoSceneRendererPremium.h
}

INCLUDEPATH += \
	../node \
	../runtime \
	../type \
	../type/list

NODE_LIBRARY_INCLUDEPATH = \
	shader \
	../node/vuo.mouse \
	../node/vuo.noise \
	../node/vuo.scene \
	$${FREEIMAGE_ROOT}/include \
	$${MUPARSER_ROOT}/include \
	$${CURL_ROOT}/include \
	$${ASSIMP_ROOT}/include

include(../module.pri)

NODE_LIBRARY_SHARED_NONGL_SOURCES += \
	VuoHeap.cc

OTHER_FILES += $$NODE_LIBRARY_SHARED_NONGL_SOURCES

CLANG_NODE_LIBRARY_SHARED_NONGL_FLAGS = \
	$$CLANG_NODE_LIBRARY_FLAGS \
	$$QMAKE_LFLAGS \
	../runtime/libVuoLog.bc \
	-framework CoreFoundation \
	-dynamiclib \
	-Wl,-no_function_starts \
	-Wl,-no_version_load_command \
	-headerpad_max_install_names
CLANG_NODE_LIBRARY_SHARED_NONGL_FLAGS -= -emit-llvm
node_library_shared_nongl.input = NODE_LIBRARY_SHARED_NONGL_SOURCES
node_library_shared_nongl.depend_command = $$QMAKE_CXX -nostdinc -MM -MF - -MG $$CLANG_NODE_LIBRARY_FLAGS ${QMAKE_FILE_NAME} | sed \"s,^.*: ,,\"
node_library_shared_nongl.output = lib${QMAKE_FILE_IN_BASE}.dylib
node_library_shared_nongl.commands = $$QMAKE_CXX $$CLANG_NODE_LIBRARY_SHARED_NONGL_FLAGS ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT} \
	&& install_name_tool -id @rpath/Vuo.framework/Versions/$$VUO_VERSION/Modules/${QMAKE_FILE_OUT} ${QMAKE_FILE_OUT}
coverage {
	node_library_shared_nongl.commands += && install_name_tool -change @executable_path/../lib/libprofile_rt.dylib $$LLVM_ROOT/lib/libprofile_rt.dylib ${QMAKE_FILE_OUT}
}
QMAKE_EXTRA_COMPILERS += node_library_shared_nongl

NODE_LIBRARY_SHARED_GL_SOURCES += \
	VuoGlContext.cc



OTHER_FILES += $$NODE_LIBRARY_SHARED_GL_SOURCES

CLANG_NODE_LIBRARY_SHARED_GL_FLAGS = \
	$$CLANG_NODE_LIBRARY_SHARED_NONGL_FLAGS \
	-framework OpenGL
node_library_shared_gl.input = NODE_LIBRARY_SHARED_GL_SOURCES
node_library_shared_gl.depend_command = $$QMAKE_CXX -nostdinc -MM -MF - -MG $$CLANG_NODE_LIBRARY_FLAGS ${QMAKE_FILE_NAME} | sed \"s,^.*: ,,\"
node_library_shared_gl.output = lib${QMAKE_FILE_IN_BASE}.dylib
node_library_shared_gl.commands = $$QMAKE_CXX $$CLANG_NODE_LIBRARY_SHARED_GL_FLAGS ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT} \
	&& install_name_tool -id @rpath/Vuo.framework/Versions/$$VUO_VERSION/Modules/${QMAKE_FILE_OUT} ${QMAKE_FILE_OUT}
coverage {
	node_library_shared_gl.commands += && install_name_tool -change @executable_path/../lib/libprofile_rt.dylib $$LLVM_ROOT/lib/libprofile_rt.dylib ${QMAKE_FILE_OUT}
}
QMAKE_EXTRA_COMPILERS += node_library_shared_gl



NODE_LIBRARY_SHARED_SOURCES_DEPENDENT_ON_CONTEXT += \
	VuoGlPool.cc

OTHER_FILES += $$NODE_LIBRARY_SHARED_SOURCES_DEPENDENT_ON_CONTEXT

CLANG_NODE_LIBRARY_SHARED_DEPENDENT_ON_CONTEXT_FLAGS = \
	$$CLANG_NODE_LIBRARY_SHARED_GL_FLAGS \
	-framework ApplicationServices \
	-framework CoreFoundation \
	-framework IOSurface \
	../type/VuoText.o \
	$$JSONC_ROOT/lib/libjson-c.a \
	-L . \
	-lVuoGlContext \
	-lVuoHeap
node_library_shared_dependent_on_context.input = NODE_LIBRARY_SHARED_SOURCES_DEPENDENT_ON_CONTEXT
node_library_shared_dependent_on_context.depends = libVuoGlContext.dylib
node_library_shared_dependent_on_context.depend_command = $$QMAKE_CXX -nostdinc -MM -MF - -MG $$CLANG_NODE_LIBRARY_FLAGS ${QMAKE_FILE_NAME} | sed \"s,^.*: ,,\"
node_library_shared_dependent_on_context.output = lib${QMAKE_FILE_IN_BASE}.dylib
node_library_shared_dependent_on_context.commands = $$QMAKE_CXX $$CLANG_NODE_LIBRARY_SHARED_DEPENDENT_ON_CONTEXT_FLAGS ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT} \
	&& install_name_tool -id @rpath/Vuo.framework/Versions/$$VUO_VERSION/Modules/${QMAKE_FILE_OUT} ${QMAKE_FILE_OUT}
coverage {
	node_library_shared_dependent_on_context.commands += && install_name_tool -change @executable_path/../lib/libprofile_rt.dylib $$LLVM_ROOT/lib/libprofile_rt.dylib ${QMAKE_FILE_OUT}
}
QMAKE_EXTRA_COMPILERS += node_library_shared_dependent_on_context

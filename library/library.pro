TEMPLATE = aux
CONFIG += VuoPCH

include(../vuo.pri)

NODE_LIBRARY_SOURCES += \
	VuoApp.m \
	VuoBase64.cc \
	VuoCglPixelFormat.c \
	VuoDisplayRefresh.c \
	VuoGraphicsView.m \
	VuoGraphicsWindow.m \
	VuoGraphicsWindowDelegate.m \
	VuoGraphicsWindowDrag.mm \
	VuoIoReturn.c \
	VuoImageResize.c \
	VuoImageBlend.c \
	VuoImageBlur.c \
	VuoImageConvolve.c \
	VuoImageGet.cc \
	VuoImageMapColors.c \
	VuoImageRenderer.cc \
	VuoSceneText.c \
	VuoImageText.cc \
	VuoSceneObjectGet.c \
	VuoSceneObjectRenderer.cc \
	VuoSceneRenderer.cc \
	VuoUrlFetch.c \
	VuoMathExpressionParser.cc \
	VuoMeshParametric.cc \
	VuoMeshUtility.cc \
	VuoOsStatus.c \
	VuoPnpId.c \
	VuoPointsParametric.cc \
	VuoScreenCapture.m \
	VuoScreenCommon.m \
	VuoSort.c \
	VuoTextHtml.c \
	VuoWindow.m \
	VuoWindowRecorder.m \
	VuoWindowTextInternal.m \
	VuoUrlParser.c \
	libmodule.c

SOURCES += \
	VuoBase64.cc \
	VuoCglPixelFormat.c \
	VuoDisplayRefresh.c \
	VuoIoReturn.c \
	VuoImageResize.c \
	VuoImageBlend.c \
	VuoImageBlur.c \
	VuoImageConvolve.c \
	VuoImageGet.cc \
	VuoImageMapColors.c \
	VuoImageRenderer.cc \
	VuoSceneText.c \
	VuoImageText.cc \
	VuoMathExpressionParser.cc \
	VuoMeshParametric.cc \
	VuoMeshUtility.cc \
	VuoOsStatus.c \
	VuoPnpId.c \
	VuoPointsParametric.cc \
	VuoSceneObjectRenderer.cc \
	VuoSceneRenderer.cc \
	VuoTextHtml.c \
	VuoUrlFetch.c \
	VuoUrlParser.c \
	libmodule.c

OBJECTIVE_SOURCES += \
	VuoApp.m \
	VuoGraphicsView.m \
	VuoGraphicsWindow.m \
	VuoGraphicsWindowDelegate.m \
	VuoGraphicsWindowDrag.mm \
	VuoScreenCommon.m \
	VuoWindow.m \
	VuoWindowRecorder.m \
	VuoWindowTextInternal.m

HEADERS += \
	VuoApp.h \
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
	VuoImageConvolve.h \
	VuoImageRenderer.h \
	VuoImageText.h \
	VuoLog.h \
	VuoMathExpressionParser.h \
	VuoMeshParametric.h \
	VuoMeshUtility.h \
	VuoOsStatus.h \
	VuoPnpId.h \
	VuoPointsParametric.h \
	VuoPool.hh \
	VuoSceneObjectGet.h \
	VuoSceneObjectRenderer.h \
	VuoSceneRenderer.h \
	VuoScreenCapture.h \
	VuoScreenCommon.h \
	VuoSmooth.h \
	VuoSort.h \
	VuoTextHtml.h \
	VuoUrlFetch.h \
	VuoWindow.h \
	VuoUrlParser.h

OTHER_FILES += \
	../node/module.h \
	../node/node.h \
	VuoGraphicsView.h \
	VuoGraphicsWindow.h \
	VuoGraphicsWindowDelegate.h \
	VuoGraphicsWindowDrag.h \
	VuoImageResize.h \
	VuoSceneText.h \
	VuoImageWatermark.h \
	VuoTriggerSet.hh \
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
	../node/vuo.image \
	../node/vuo.mouse \
	../node/vuo.noise \
	../node/vuo.scene \
	$${FREEIMAGE_ROOT}/include \
	$${LIBXML2_ROOT}/include/libxml2 \
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
	VuoCglPixelFormat.o \
	VuoPnpId.o \
	VuoScreenCommon.o \
	$$CLANG_NODE_LIBRARY_SHARED_NONGL_FLAGS \
	$$JSONC_ROOT/lib/libjson-c.a \
	../type/VuoInteger.o \
	../type/VuoPoint2d.o \
	../type/VuoReal.o \
	../type/VuoScreen.o \
	../type/VuoText.o \
	../type/list/VuoList_VuoInteger.o \
	../type/list/VuoList_VuoReal.o \
	../type/list/VuoList_VuoScreen.o \
	-L . \
	-lVuoHeap \
	-framework ApplicationServices \
	-framework Cocoa \
	-framework CoreFoundation \
	-framework IOKit \
	-framework IOSurface \
	-framework OpenGL
node_library_shared_gl.input = NODE_LIBRARY_SHARED_GL_SOURCES
node_library_shared_gl.depends = \
	VuoCglPixelFormat.o \
	VuoPnpId.o \
	VuoScreenCommon.o
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

CLANG_NODE_LIBRARY_SHARED_DEPENDENT_ON_CONTEXT_LIBS = \
	../runtime/VuoEventLoop.o
CLANG_NODE_LIBRARY_SHARED_DEPENDENT_ON_CONTEXT_FLAGS = \
	$$CLANG_NODE_LIBRARY_SHARED_GL_FLAGS \
	$$CLANG_NODE_LIBRARY_SHARED_DEPENDENT_ON_CONTEXT_LIBS \
	-lVuoGlContext
node_library_shared_dependent_on_context.input = NODE_LIBRARY_SHARED_SOURCES_DEPENDENT_ON_CONTEXT
node_library_shared_dependent_on_context.depends = libVuoGlContext.dylib $$CLANG_NODE_LIBRARY_SHARED_DEPENDENT_ON_CONTEXT_LIBS
node_library_shared_dependent_on_context.depend_command = $$QMAKE_CXX -nostdinc -MM -MF - -MG $$CLANG_NODE_LIBRARY_FLAGS ${QMAKE_FILE_NAME} | sed \"s,^.*: ,,\"
node_library_shared_dependent_on_context.output = lib${QMAKE_FILE_IN_BASE}.dylib
node_library_shared_dependent_on_context.commands = $$QMAKE_CXX $$CLANG_NODE_LIBRARY_SHARED_DEPENDENT_ON_CONTEXT_FLAGS ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT} \
	&& install_name_tool -id @rpath/Vuo.framework/Versions/$$VUO_VERSION/Modules/${QMAKE_FILE_OUT} ${QMAKE_FILE_OUT}
coverage {
	node_library_shared_dependent_on_context.commands += && install_name_tool -change @executable_path/../lib/libprofile_rt.dylib $$LLVM_ROOT/lib/libprofile_rt.dylib ${QMAKE_FILE_OUT}
}
QMAKE_EXTRA_COMPILERS += node_library_shared_dependent_on_context



CSGJS_SOURCES = csgjs.cc
OTHER_FILES += $$CSGJS_SOURCES
csgjs.input = CSGJS_SOURCES
csgjs.output = lib${QMAKE_FILE_IN_BASE}.a
csgjs.commands = $$QMAKE_CXX -Oz -c ${QMAKE_FILE_IN} -o ${QMAKE_FILE_IN_BASE}.o \
	&& ar -r lib${QMAKE_FILE_IN_BASE}.a ${QMAKE_FILE_IN_BASE}.o
QMAKE_EXTRA_COMPILERS += csgjs

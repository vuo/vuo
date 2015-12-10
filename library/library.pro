TEMPLATE = aux
CONFIG += VuoPCH

include(../vuo.pri)

NODE_LIBRARY_SOURCES += \
	VuoDisplayRefresh.c \
	VuoImageBlur.c \
	VuoImageGet.cc \
	VuoImageMapColors.c \
	VuoImageRenderer.cc \
	VuoImageText.c \
	VuoSceneObjectGet.c \
	VuoSceneObjectRenderer.cc \
	VuoSceneRenderer.cc \
	VuoUrlFetch.c \
	VuoMathExpressionParser.cc \
	VuoMeshParametric.cc \
	VuoPointsParametric.cc \
	VuoScreenCommon.m \
	VuoWindow.m \
	VuoWindowApplication.m \
	VuoWindowOpenGLInternal.m \
	VuoWindowTextInternal.m \
	VuoNSRunLoop.m

SOURCES += \
	VuoImageBlur.c \
	VuoImageMapColors.c \
	VuoImageRenderer.cc \
	VuoImageText.c \
	VuoMathExpressionParser.cc \
	VuoMeshParametric.cc \
	VuoPointsParametric.cc \
	VuoSceneObjectRenderer.cc \
	VuoSceneRenderer.cc \
	VuoUrlFetch.c

HEADERS += \
	VuoDisplayRefresh.h \
	VuoGlContext.h \
	VuoGlPool.h \
	VuoHeap.h \
	VuoImageGet.h \
	VuoImageRenderer.h \
	VuoLog.h \
	VuoMathExpressionParser.h \
	VuoMeshParametric.h \
	VuoPointsParametric.h \
	VuoPool.hh \
	VuoSceneObjectGet.h \
	VuoSceneObjectRenderer.h \
	VuoSceneRenderer.h \
	VuoScreenCommon.h \
	VuoSmooth.h \
	VuoUrlFetch.h \
	VuoWindow.h \
	VuoNSRunLoop.h

OTHER_FILES += \
	VuoImageText.h \
	VuoTriggerSet.hh \
	VuoWindowApplication.h \
	VuoWindowOpenGLInternal.h \
	VuoWindowTextInternal.h

INCLUDEPATH += \
	../node \
	../runtime \
	../type \
	../type/list

NODE_LIBRARY_INCLUDEPATH = \
	shader \
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
QMAKE_EXTRA_COMPILERS += node_library_shared_gl



NODE_LIBRARY_SHARED_SOURCES_DEPENDENT_ON_CONTEXT += \
	VuoGlPool.cc

OTHER_FILES += $$NODE_LIBRARY_SHARED_SOURCES_DEPENDENT_ON_CONTEXT

CLANG_NODE_LIBRARY_SHARED_DEPENDENT_ON_CONTEXT_FLAGS = \
	$$CLANG_NODE_LIBRARY_SHARED_GL_FLAGS \
	-framework CoreFoundation \
	-framework IOSurface \
	../type/VuoText.o \
	$$JSONC_ROOT/lib/libjson.a \
	-L . \
	-lVuoGlContext \
	-lVuoHeap
node_library_shared_dependent_on_context.input = NODE_LIBRARY_SHARED_SOURCES_DEPENDENT_ON_CONTEXT
node_library_shared_dependent_on_context.depends = libVuoGlContext.dylib
node_library_shared_dependent_on_context.depend_command = $$QMAKE_CXX -nostdinc -MM -MF - -MG $$CLANG_NODE_LIBRARY_FLAGS ${QMAKE_FILE_NAME} | sed \"s,^.*: ,,\"
node_library_shared_dependent_on_context.output = lib${QMAKE_FILE_IN_BASE}.dylib
node_library_shared_dependent_on_context.commands = $$QMAKE_CXX $$CLANG_NODE_LIBRARY_SHARED_DEPENDENT_ON_CONTEXT_FLAGS ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT} \
	&& install_name_tool -id @rpath/Vuo.framework/Versions/$$VUO_VERSION/Modules/${QMAKE_FILE_OUT} ${QMAKE_FILE_OUT}
QMAKE_EXTRA_COMPILERS += node_library_shared_dependent_on_context

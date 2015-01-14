TEMPLATE = aux
CONFIG += VuoPCH

include(../vuo.pri)

NODE_LIBRARY_SOURCES += \
	VuoDisplayRefresh.c \
	VuoImageGet.cc \
	VuoImageRenderer.cc \
	VuoSceneRenderer.cc \
	VuoUrl.c \
	VuoVerticesParametric.cc \
	VuoWindow.m \
	VuoWindowApplication.m \
	VuoWindowOpenGLInternal.m \
	VuoWindowTextInternal.m \
	VuoNSRunLoop.m

SOURCES += \
	VuoImageRenderer.cc \
	VuoSceneRenderer.cc \
	VuoVerticesParametric.cc

HEADERS += \
	VuoDisplayRefresh.h \
	VuoGlContext.h \
	VuoGlPool.h \
	VuoImageGet.h \
	VuoImageRenderer.h \
	VuoSceneRenderer.h \
	VuoUrl.h \
	VuoVerticesParametric.h \
	VuoWindow.h \
	VuoNSRunLoop.h

OTHER_FILES += \
	VuoWindowApplication.h \
	VuoWindowOpenGLInternal.h \
	VuoWindowTextInternal.h

INCLUDEPATH += \
	../node \
	../runtime \
	../type \
	../type/list

QMAKE_CLEAN += *.vuonode *.bc

NODE_LIBRARY_INCLUDEPATH = \
	$${FREEIMAGE_ROOT}/include \
	$${ICU_ROOT}/include \
	$${MUPARSER_ROOT}/include \
	$${CURL_ROOT}/include \
	$${ASSIMP_ROOT}/include

include(../module.pri)


NODE_LIBRARY_SHARED_SOURCES += \
	VuoGlContext.cc

OTHER_FILES += $$NODE_LIBRARY_SHARED_SOURCES

CLANG_NODE_LIBRARY_SHARED_FLAGS = \
	$$CLANG_NODE_LIBRARY_FLAGS \
	-dynamiclib \
	-framework OpenGL \
	-Wl,-no_function_starts \
	-Wl,-no_version_load_command
CLANG_NODE_LIBRARY_SHARED_FLAGS -= -emit-llvm
node_library_shared.input = NODE_LIBRARY_SHARED_SOURCES
node_library_shared.depend_command = $$QMAKE_CXX -o /dev/null -E -MD -MF - $${CLANG_NODE_LIBRARY_SHARED_FLAGS} ${QMAKE_FILE_NAME} 2>&1 | sed \"s,^.*: ,,\"
node_library_shared.output = lib${QMAKE_FILE_IN_BASE}.dylib
node_library_shared.commands = $$QMAKE_CXX $$CLANG_NODE_LIBRARY_SHARED_FLAGS ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT} \
	&& install_name_tool -id @rpath/Vuo.framework/Versions/$$VUO_VERSION/Modules/${QMAKE_FILE_OUT} ${QMAKE_FILE_OUT}
QMAKE_EXTRA_COMPILERS += node_library_shared



NODE_LIBRARY_SHARED_SOURCES_DEPENDENT_ON_CONTEXT += \
	VuoGlPool.cc

OTHER_FILES += $$NODE_LIBRARY_SHARED_SOURCES_DEPENDENT_ON_CONTEXT

CLANG_NODE_LIBRARY_SHARED_DEPENDENT_ON_CONTEXT_FLAGS = \
	$$CLANG_NODE_LIBRARY_SHARED_FLAGS \
	-headerpad_max_install_names \
	-L . \
	-lVuoGlContext
node_library_shared_dependent_on_context.input = NODE_LIBRARY_SHARED_SOURCES_DEPENDENT_ON_CONTEXT
node_library_shared_dependent_on_context.depends = libVuoGlContext.dylib
node_library_shared_dependent_on_context.depend_command = $$QMAKE_CXX -o /dev/null -E -MD -MF - $${CLANG_NODE_LIBRARY_SHARED_DEPENDENT_ON_CONTEXT_FLAGS} ${QMAKE_FILE_NAME} 2>&1 | sed \"s,^.*: ,,\"
node_library_shared_dependent_on_context.output = lib${QMAKE_FILE_IN_BASE}.dylib
node_library_shared_dependent_on_context.commands = $$QMAKE_CXX $$CLANG_NODE_LIBRARY_SHARED_DEPENDENT_ON_CONTEXT_FLAGS ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT} \
	&& install_name_tool -id @rpath/Vuo.framework/Versions/$$VUO_VERSION/Modules/${QMAKE_FILE_OUT} ${QMAKE_FILE_OUT}
QMAKE_EXTRA_COMPILERS += node_library_shared_dependent_on_context

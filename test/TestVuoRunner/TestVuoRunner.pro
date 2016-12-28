TEMPLATE = app
CONFIG -= app_bundle
CONFIG += console testcase graphviz VuoLLVM qtTest VuoBase VuoRuntime
TARGET = TestVuoRunner

include(../../vuo.pri)

SOURCES += \
	TestVuoRunner.cc

TEST_RUNNER_SOURCES += \
	compositionForListening.c \
	compositionForControlling.c \
	compositionForLosingContact.c

OTHER_FILES += composition.h \
	$$TEST_RUNNER_SOURCES

QMAKE_LFLAGS += \
	-rpath ../../framework \
	-framework AppKit \
	-framework OpenGL \
	$$ROOT/library/libVuoGlContext.dylib

LLVM_LINK = $${LLVM_ROOT}/bin/llvm-link
TEST_RUNNER_NATIVE_LIBS = \
	"$${ZMQ_ROOT}/lib/libzmq.a \
	-lstdc++ \
	-framework CoreFoundation \
	$${GRAPHVIZ_ROOT}/lib/graphviz/libgvplugin_dot_layout.dylib \
	$${GRAPHVIZ_ROOT}/lib/graphviz/libgvplugin_core.dylib \
	$${GRAPHVIZ_ROOT}/lib/libgvc.dylib \
	$${GRAPHVIZ_ROOT}/lib/libgraph.dylib"

test_runner.depends = $${ROOT}/runtime/*.bc $${ROOT}/library/*.dylib
test_runner.input = TEST_RUNNER_SOURCES
test_runner.output = ${QMAKE_FILE_IN_BASE}
test_runner.commands = \
	cp $$ROOT/library/libVuoHeap.dylib . && \
	install_name_tool -id libVuoHeap.dylib libVuoHeap.dylib && \
	$$QMAKE_CC \
		-g \
		-I$$ROOT/runtime \
		-I$$ROOT/base \
		-emit-llvm \
		-Xclang -include-pch -Xclang pch/TestVuoRunner/c.pch \
		$(CFLAGS) \	# Use $() here to get the variable at make-time because QMAKE_CFLAGS doesn't have platform-specific flags yet at this point in qmake-time.
		-c ${QMAKE_FILE_IN} \
		-o ${QMAKE_FILE_IN_BASE}.bc && \
	$$LLVM_LINK \
		$$ROOT/runtime/libVuoEventLoop.bc \
		$$ROOT/runtime/libVuoRuntime.bc \
		$$ROOT/runtime/libVuoTelemetry.bc \
		${QMAKE_FILE_IN_BASE}.bc \
		-o ${QMAKE_FILE_IN_BASE}-linked.bc && \
	$$QMAKE_CC \
		$$QMAKE_LFLAGS \
		$${TEST_RUNNER_NATIVE_LIBS} \
		${QMAKE_FILE_IN_BASE}-linked.bc \
		libVuoHeap.dylib \
		$${JSONC_ROOT}/lib/libjson-c.a \
		-lobjc \
		-framework AppKit \
		-o ${QMAKE_FILE_OUT}
test_runner.CONFIG += no_link target_predeps
QMAKE_EXTRA_COMPILERS += test_runner

buildComposition.commands = \
	   ../../framework/vuo-compile PublishedPorts.vuo \
	&& ../../framework/vuo-link PublishedPorts.bc
buildComposition.depends += PublishedPorts.vuo
buildComposition.target = PublishedPorts
POST_TARGETDEPS += PublishedPorts
QMAKE_EXTRA_TARGETS += buildComposition
QMAKE_CLEAN += PublishedPorts PublishedPorts.bc

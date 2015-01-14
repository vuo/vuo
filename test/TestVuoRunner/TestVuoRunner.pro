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

LLVM_LINK = $${LLVM_ROOT}/bin/llvm-link
TEST_RUNNER_NATIVE_LIBS = \
	"$${ZMQ_ROOT}/lib/libzmq.a \
	-lstdc++ \
	-framework CoreFoundation \
	$${GRAPHVIZ_ROOT}/lib/graphviz/libgvplugin_dot_layout.dylib \
	$${GRAPHVIZ_ROOT}/lib/graphviz/libgvplugin_core.dylib \
	$${GRAPHVIZ_ROOT}/lib/libgvc.dylib \
	$${GRAPHVIZ_ROOT}/lib/libgraph.dylib"

test_runner.depends = $${ROOT}/runtime/*.bc
test_runner.input = TEST_RUNNER_SOURCES
test_runner.output = ${QMAKE_FILE_IN_BASE}
test_runner.commands = \
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
		$$ROOT/runtime/libVuoRuntime.bc \
		$$ROOT/runtime/libVuoTelemetry.bc \
		${QMAKE_FILE_IN_BASE}.bc \
		-o ${QMAKE_FILE_IN_BASE}-linked.bc && \
	$$QMAKE_CC \
		$${TEST_RUNNER_NATIVE_LIBS} \
		${QMAKE_FILE_IN_BASE}-linked.bc \
		$${JSONC_ROOT}/lib/libjson.a \
		-lobjc \
		-framework Foundation \
		-o ${QMAKE_FILE_OUT}
test_runner.CONFIG += no_link target_predeps
QMAKE_EXTRA_COMPILERS += test_runner

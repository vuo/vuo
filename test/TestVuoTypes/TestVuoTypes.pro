TEMPLATE = lib
CONFIG -= app_bundle
CONFIG += no_link target_predeps staticlib VuoPCH json qtCore qtTest testcase VuoNoLibrary

include(../../vuo.pri)

HEADERS += TestVuoTypes.h

TEST_VUO_TYPES_SOURCES += \
	TestVuoColor.cc \
	TestVuoInteger.cc \
	TestVuoImage.cc \
	TestVuoList.cc \
	TestVuoMidiNote.cc \
	TestVuoPoint2d.cc \
	TestVuoPoint3d.cc \
	TestVuoPoint4d.cc \
	TestVuoReal.cc \
	TestVuoVertices.cc \
	TestVuoSceneObject.cc \
	TestVuoShader.cc \
	TestVuoText.cc \
	TestVuoTransform.cc

OTHER_FILES += $$TEST_VUO_TYPES_SOURCES

TEST_FLAGS += \
	$(CXXFLAGS) \	# Use $() here to get the variable at make-time because QMAKE_CFLAGS doesn't have platform-specific flags yet at this point in qmake-time.
	-I$$ROOT/base \
	-I$$ROOT/node \
	-I$$ROOT/runtime \
	-I$$ROOT/type \
	-I$$ROOT/type/list \
	-I$$ICU_ROOT/include \
	$$ICU_ROOT/lib/libicuuc.a \
	$$ICU_ROOT/lib/libicudata.a \
	$$ZMQ_ROOT/lib/libzmq.a \
	$${GRAPHVIZ_ROOT}/lib/libgvc.dylib \
	$${GRAPHVIZ_ROOT}/lib/libgraph.dylib \
	$${GRAPHVIZ_ROOT}/lib/graphviz/libgvplugin_dot_layout.dylib \
	$${GRAPHVIZ_ROOT}/lib/graphviz/libgvplugin_core.dylib \
	$(INCPATH) \
	$(DEFINES) \
	$$ROOT/runtime/VuoHeap.cc \
	$$ROOT/base/VuoRuntime.o \
	$$ROOT/base/VuoCompositionStub.o \
	$$ROOT/base/VuoTelemetry.o \
	$$ROOT/node/libVuoGlContext.dylib \
	$$ROOT/node/libVuoGlTexturePool.dylib \
	$$ROOT/node/VuoImageRenderer.o \
	$$ROOT/type/VuoBoolean.o \
	$$ROOT/type/VuoColor.o \
	$$ROOT/type/VuoImage.o \
	$$ROOT/type/VuoInteger.o \
	$$ROOT/type/VuoMidiNote.o \
	$$ROOT/type/VuoPoint2d.o \
	$$ROOT/type/VuoPoint3d.o \
	$$ROOT/type/VuoPoint4d.o \
	$$ROOT/type/VuoReal.o \
	$$ROOT/type/VuoSceneObject.o \
	$$ROOT/type/VuoShader.o \
	$$ROOT/type/VuoText.o \
	$$ROOT/type/VuoTransform.o \
	$$ROOT/type/VuoVertices.o \
	$$ROOT/type/list/VuoList_VuoImage.o \
	$$ROOT/type/list/VuoList_VuoInteger.o \
	$$ROOT/type/list/VuoList_VuoSceneObject.o \
	$$ROOT/type/list/VuoList_VuoText.o \
	$$ROOT/type/list/VuoList_VuoVertices.o \
	$$QMAKE_LFLAGS $$LIBS $$QMAKE_LIBS \
	-Xclang -include-pch -Xclang pch/TestVuoTypes/c++.pch \
	-Wl,-rpath,$$ROOT/framework \
	$$QMAKE_CFLAGS_WARN_ON \
	"-framework QtTest" \
	"-framework QtCore" \
	"-framework IOSurface" \
	"-framework OpenGL"

# Independently compile multiple tests in this folder
testVuoTypes.input = TEST_VUO_TYPES_SOURCES
testVuoTypes.output = ${QMAKE_FILE_IN_BASE}
testVuoTypes.depends = $$ROOT/type/*.c
testVuoTypes.commands  = $$QT_ROOT/bin/moc $(DEFINES) $(INCPATH) -o ${QMAKE_FILE_IN_BASE}.moc ${QMAKE_FILE_IN}
testVuoTypes.commands += && $$QMAKE_CXX $$TEST_FLAGS -o ${QMAKE_FILE_IN_BASE} ${QMAKE_FILE_IN}
QMAKE_EXTRA_COMPILERS += testVuoTypes

QMAKE_CLEAN += -R *.moc *.dSYM

# Execute each of the independently-compiled tests in this folder
check.commands = (find * -perm +1 -type f -exec ./{} `echo '$(TESTARGS)' | perl -pe 's/test.xml/test-{}.xml/g'` \\;)
# Discard the default check command which gets unconditionally appended
check.commands += && true

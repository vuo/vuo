TEMPLATE = aux
CONFIG += VuoPCH json qtCore qtTest testcase

include(../../vuo.pri)
include(../test.pri)

HEADERS += TestVuoTypes.h

TEST_VUO_TYPES_SOURCES += \
	TestVuoArtNet.cc \
	TestVuoAudioSamples.cc \
	TestVuoBoolean.cc \
	TestVuoColor.cc \
	TestVuoFont.cc \
	TestVuoInteger.cc \
	TestVuoImage.cc \
	TestVuoLayer.cc \
	TestVuoList.cc \
	TestVuoMathExpression.cc \
	TestVuoMesh.cc \
	TestVuoMidiNote.cc \
	TestVuoPoint2d.cc \
	TestVuoPoint3d.cc \
	TestVuoPoint4d.cc \
	TestVuoReal.cc \
	TestVuoSceneObject.cc \
	TestVuoShader.cc \
	TestVuoText.cc \
	TestVuoTime.cc \
	TestVuoTransform.cc \
	TestVuoTransform2d.cc \
	TestVuoUrl.cc

OTHER_FILES += $$TEST_VUO_TYPES_SOURCES

# Independently compile multiple tests in this folder
testVuoTypes.input = TEST_VUO_TYPES_SOURCES
testVuoTypes.output = ${QMAKE_FILE_IN_BASE}
testVuoTypes.depends = $$ROOT/type/*.c
testVuoTypes.commands  = $$QT_ROOT/bin/moc $(DEFINES) $(INCPATH) -o ${QMAKE_FILE_IN_BASE}.moc ${QMAKE_FILE_IN}
testVuoTypes.commands += && $$QMAKE_CXX \
	-Xclang -include-pch -Xclang pch/TestVuoTypes/c++.pch \
	$(CXXFLAGS) \	# Use $() here to get the variable at make-time because QMAKE_CFLAGS doesn't have platform-specific flags yet at this point in qmake-time.
	$$QMAKE_LFLAGS \
	-o ${QMAKE_FILE_IN_BASE} \
	${QMAKE_FILE_IN}
QMAKE_EXTRA_COMPILERS += testVuoTypes

# Execute each of the independently-compiled tests in this folder
check.commands = (find * -perm +1 -type f -exec ./{} `echo '$(TESTARGS)' | perl -pe 's/test.xml/test-{}.xml/g'` \\;)
# Discard the default check command which gets unconditionally appended
check.commands += && true

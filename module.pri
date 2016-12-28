MODULE_INCLUDEPATH = \
	$$JSONC_ROOT/include \
	$${ROOT}/library \
	$${ROOT}/node \
	$${ROOT}/node/vuo.font \
	$${ROOT}/node/vuo.ui \
	$${ROOT}/type \
	$${ROOT}/type/list \
	$${ROOT}/runtime

NODE_SOURCES += $$GENERIC_NODE_SOURCES
VUOCOMPILE_NODE_INCLUDEPATH = \
	$$MODULE_INCLUDEPATH \
	$$NODE_INCLUDEPATH
VUOCOMPILE_NODE_DEPEND_FLAGS = \
	$$join(VUOCOMPILE_NODE_INCLUDEPATH, " -I", "-I")
VUOCOMPILE_NODE_FLAGS = \
	$$join(VUOCOMPILE_NODE_INCLUDEPATH, " --header-search-path ", "--header-search-path ")
FAKE_DEFINES_FOR_GAMMA = -Duint8_t -Dint8_t -Duint16_t -Dint16_t -Duint32_t -Dint32_t
node.input = NODE_SOURCES
node.depend_command = $$QMAKE_CC -nostdinc -MM -MF - -MG $$FAKE_DEFINES_FOR_GAMMA $$VUOCOMPILE_NODE_DEPEND_FLAGS ${QMAKE_FILE_NAME} | sed \"s,^.*: ,,\"
node.output = ${QMAKE_FILE_IN_BASE}.vuonode
node.commands = $$VUOCOMPILE $$VUOCOMPILE_NODE_FLAGS --output ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
QMAKE_EXTRA_COMPILERS += node


VUOCOMPILE_TYPE_INCLUDEPATH = \
	$$MODULE_INCLUDEPATH \
	$$JSONC_ROOT/include \
	$$TYPE_INCLUDEPATH
VUOCOMPILE_TYPE_DEPEND_FLAGS = \
	$$join(VUOCOMPILE_TYPE_INCLUDEPATH, " -I", "-I")
VUOCOMPILE_TYPE_FLAGS = \
	$$join(VUOCOMPILE_TYPE_INCLUDEPATH, " --header-search-path ", "--header-search-path ")
type.input = TYPE_SOURCES
type.depend_command = $$QMAKE_CC -nostdinc -MM -MF - -MG $$VUOCOMPILE_TYPE_DEPEND_FLAGS ${QMAKE_FILE_NAME} | sed \"s,^.*: ,,\"
type.output = ${QMAKE_FILE_IN_BASE}.bc
type.commands = $$VUOCOMPILE $$VUOCOMPILE_TYPE_FLAGS --output ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
type.variable_out = TYPE_BITCODE
type.CONFIG = target_predeps
QMAKE_EXTRA_COMPILERS += type

typeObjects.input = TYPE_BITCODE
typeObjects.output = ${QMAKE_FILE_IN_BASE}.o
typeObjects.commands = $$QMAKE_CC $$FLAGS -Oz -c -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
typeObjects.variable_out = OBJECTS
typeObjects.CONFIG = target_predeps
QMAKE_EXTRA_COMPILERS += typeObjects


CLANG_NODE_LIBRARY_INCLUDEPATH = \
	$$MODULE_INCLUDEPATH \
	$$JSONC_ROOT/include \
	$$NODE_LIBRARY_INCLUDEPATH
CLANG_NODE_LIBRARY_FLAGS = \
	-target x86_64-apple-macosx10.7.0 \
	-fblocks \
	-fexceptions \
	-emit-llvm \
	-Oz \
	$$QMAKE_CXXFLAGS_WARN_ON \
	$$join(CLANG_NODE_LIBRARY_INCLUDEPATH, " -I", "-I") \
	$$join(DEFINES, " -D", "-D") \
	$$VUO_VERSION_DEFINES \
	-DVUO_COMPILER=1

CLANG_NODE_LIBRARY_FLAGS += -isysroot $$QMAKE_MAC_SDK.macosx.path

# Skip --coverage on Objective-C(++) files, since clang crashes.
CLANG_NODE_LIBRARY_FLAGS_NON_OBJC = $$FLAGS
# When using --coverage, skip debug symbols, since Vuo Compiler crashes.
CLANG_NODE_LIBRARY_FLAGS_NON_OBJC -= -g

node_library.input = NODE_LIBRARY_SOURCES
node_library.depend_command = $$QMAKE_CC -nostdinc -MM -MF - -MG $$QMAKE_CFLAGS_X86_64 $${CLANG_NODE_LIBRARY_FLAGS} ${QMAKE_FILE_NAME} | sed \"s,^.*: ,,\"
node_library.output = ${QMAKE_FILE_IN_BASE}.bc
node_library.commands = \
	@if [ "${QMAKE_FILE_EXT}" == ".c" ]; then \
		echo $$QMAKE_CC $$CLANG_NODE_LIBRARY_FLAGS $$CLANG_NODE_LIBRARY_FLAGS_NON_OBJC -c ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT} ; \
		$$QMAKE_CC $$CLANG_NODE_LIBRARY_FLAGS $$CLANG_NODE_LIBRARY_FLAGS_NON_OBJC -c ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT} ; \
	elif [ "${QMAKE_FILE_EXT}" == ".m" ]; then \
		echo $$QMAKE_CC $$CLANG_NODE_LIBRARY_FLAGS -c ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT} ; \
		$$QMAKE_CC $$CLANG_NODE_LIBRARY_FLAGS -c ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT} ; \
	elif [ "${QMAKE_FILE_EXT}" == ".cc" ]; then \
		echo $$QMAKE_CXX $$CLANG_NODE_LIBRARY_FLAGS $$CLANG_NODE_LIBRARY_FLAGS_NON_OBJC -c ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT} ; \
		$$QMAKE_CXX $$CLANG_NODE_LIBRARY_FLAGS $$CLANG_NODE_LIBRARY_FLAGS_NON_OBJC -c ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT} ; \
	elif [ "${QMAKE_FILE_EXT}" == ".mm" ]; then \
		echo $$QMAKE_CXX $$CLANG_NODE_LIBRARY_FLAGS -c ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT} ; \
		$$QMAKE_CXX $$CLANG_NODE_LIBRARY_FLAGS -c ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT} ; \
	else \
		echo "Error: Unknown file type: ${QMAKE_FILE_IN}" ; \
	fi
QMAKE_EXTRA_COMPILERS += node_library


VuoNodeSet {
	NODE_SET = $$basename(_PRO_FILE_)
	NODE_SET = $$replace(NODE_SET,.pro,)
	NODE_SET_ZIP = $$ROOT/node/$${NODE_SET}.vuonode
	NODE_SET_DIR = $$ROOT/node/$${NODE_SET}

	NODE_OBJECTS = $$NODE_SOURCES
	NODE_OBJECTS ~= s/\\.cc?$/.vuonode/g

	TYPE_OBJECTS = $$TYPE_SOURCES
	TYPE_OBJECTS ~= s/\\.cc?$/.bc/g

	NODE_LIBRARY_OBJECTS = $$NODE_LIBRARY_SOURCES
	NODE_LIBRARY_OBJECTS ~= s/\\.cc?$/.bc/g
	NODE_LIBRARY_OBJECTS ~= s/\\.mm?$/.bc/g

	NODE_SET_ZIP_CONTENTS = \
		$$NODE_OBJECTS \
		$$TYPE_OBJECTS \
		$$NODE_LIBRARY_OBJECTS \
		$$GENERIC_NODE_SOURCES \
		$$HEADERS \
		$$OTHER_FILES \
		descriptions/*

	exists($$NODE_SET_DIR/examples/*.vuo) { NODE_SET_ZIP_CONTENTS += examples/*.vuo }
	exists($$NODE_SET_DIR/examples/*.png) { NODE_SET_ZIP_CONTENTS += examples/*.png }
	exists($$NODE_SET_DIR/examples/*.jpg) { NODE_SET_ZIP_CONTENTS += examples/*.jpg }
	exists($$NODE_SET_DIR/examples/*.mov) { NODE_SET_ZIP_CONTENTS += examples/*.mov }
	exists($$NODE_SET_DIR/examples/*.mp3) { NODE_SET_ZIP_CONTENTS += examples/*.mp3 }
	exists($$NODE_SET_DIR/examples/*.3ds) { NODE_SET_ZIP_CONTENTS += examples/*.3ds }
	exists($$NODE_SET_DIR/examples/*.dae) { NODE_SET_ZIP_CONTENTS += examples/*.dae }
	exists($$NODE_SET_DIR/examples/*.data){ NODE_SET_ZIP_CONTENTS += examples/*.data}
	exists($$NODE_SET_DIR/examples/*.csv) { NODE_SET_ZIP_CONTENTS += examples/*.csv }

	# Check for impending existence of premium nodes.
	exists($$NODE_SET_DIR/premium) { NODE_SET_ZIP_CONTENTS += *.vuonode+ }

	# Ugly hack to check for impending existence of premium node libraries.
	exists($$NODE_SET_DIR/premium/Vuo[^\.]*) { NODE_SET_ZIP_CONTENTS += *.bc+ }

	createNodeSetZip.commands = \
		( [ -f $$NODE_SET_ZIP ] && rm $$NODE_SET_ZIP ) ; \
		zip --quiet $$NODE_SET_ZIP $$NODE_SET_ZIP_CONTENTS
	createNodeSetZip.depends = $$NODE_SET_ZIP_CONTENTS
	createNodeSetZip.target = $$NODE_SET_ZIP
	POST_TARGETDEPS += $$NODE_SET_ZIP
	QMAKE_EXTRA_TARGETS += createNodeSetZip

	QMAKE_CLEAN += $$NODE_SET_ZIP
}

# Enable Qt Creator to open and autocomplete 3rd-party headers
INCLUDEPATH += \
	$$MODULE_INCLUDEPATH \
	$$NODE_INCLUDEPATH \
	$$TYPE_INCLUDEPATH \
	$$NODE_LIBRARY_INCLUDEPATH

# Enable building libraries as normal machine code (by adding them to SOURCES)
INCLUDEPATH += \
	$$JSONC_ROOT/include \
	$$ROOT/library \
	$$ROOT/node \
	$$ROOT/runtime \
	$$ROOT/type \
	$$ROOT/type/list

# Enable Qt Creator to see these other source files.
OTHER_FILES += \
	$$NODE_SOURCES \
	$$TYPE_SOURCES \
	$$NODE_LIBRARY_SOURCES

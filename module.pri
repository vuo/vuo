MODULE_INCLUDEPATH = \
	$${ROOT}/library \
	$${ROOT}/node \
	$${ROOT}/type \
	$${ROOT}/type/list \
	$${ROOT}/runtime

VUOCOMPILE_NODE_INCLUDEPATH = \
	$$MODULE_INCLUDEPATH \
	$$NODE_INCLUDEPATH
VUOCOMPILE_NODE_DEPEND_FLAGS = \
	$$join(VUOCOMPILE_NODE_INCLUDEPATH, " -I", "-I")
VUOCOMPILE_NODE_FLAGS = \
	$$join(VUOCOMPILE_NODE_INCLUDEPATH, " --header-search-path ", "--header-search-path ")
node.input = NODE_SOURCES
node.depend_command = $$QMAKE_CC -o /dev/null -E -MD -MF - $$VUOCOMPILE_NODE_DEPEND_FLAGS ${QMAKE_FILE_NAME} 2>&1 | sed \"s,^.*: ,,\"
node.output = ${QMAKE_FILE_IN_BASE}.vuonode
node.commands = $$VUOCOMPILE $$VUOCOMPILE_NODE_FLAGS --output ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
QMAKE_EXTRA_COMPILERS += node
OTHER_FILES += $$NODE_SOURCES


VUOCOMPILE_TYPE_INCLUDEPATH = \
	$$MODULE_INCLUDEPATH \
	$$ICU_ROOT/include \
	$$JSONC_ROOT/include \
	$$TYPE_INCLUDEPATH
VUOCOMPILE_TYPE_DEPEND_FLAGS = \
	$$join(VUOCOMPILE_TYPE_INCLUDEPATH, " -I", "-I")
VUOCOMPILE_TYPE_FLAGS = \
	$$join(VUOCOMPILE_TYPE_INCLUDEPATH, " --header-search-path ", "--header-search-path ")
type.input = TYPE_SOURCES
type.depend_command = $$QMAKE_CC -o /dev/null -E -MD -MF - $$VUOCOMPILE_TYPE_DEPEND_FLAGS ${QMAKE_FILE_NAME} | sed \"s,^.*: ,,\"
type.output = ${QMAKE_FILE_IN_BASE}.bc
type.commands = $$VUOCOMPILE $$VUOCOMPILE_TYPE_FLAGS --output ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
type.variable_out = TYPE_BITCODE
type.CONFIG = target_predeps
QMAKE_EXTRA_COMPILERS += type
OTHER_FILES += $$TYPE_SOURCES

typeObjects.input = TYPE_BITCODE
typeObjects.output = ${QMAKE_FILE_IN_BASE}.o
typeObjects.commands = $$QMAKE_CC -c -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
typeObjects.variable_out = OBJECTS
typeObjects.CONFIG = target_predeps
QMAKE_EXTRA_COMPILERS += typeObjects


CLANG_NODE_LIBRARY_INCLUDEPATH = \
	$$MODULE_INCLUDEPATH \
	$$NODE_LIBRARY_INCLUDEPATH
CLANG_NODE_LIBRARY_FLAGS = \
	$$QMAKE_CXXFLAGS_WARN_ON \
	$$join(CLANG_NODE_LIBRARY_INCLUDEPATH, " -I", "-I") \
	$$join(DEFINES, " -D", "-D") \
	$$VUO_VERSION_DEFINES \
	-DVUO_COMPILER=1
node_library.input = NODE_LIBRARY_SOURCES
node_library.depend_command = $$QMAKE_CC -o /dev/null -E -MD -MF - -emit-llvm $$QMAKE_CFLAGS_X86_64 $${CLANG_NODE_LIBRARY_FLAGS} ${QMAKE_FILE_NAME} 2>&1 | sed \"s,^.*: ,,\"
node_library.output = ${QMAKE_FILE_IN_BASE}.bc
node_library.commands = $$QMAKE_CC -cc1 -triple x86_64-apple-macosx10.6.0 -fblocks -fcxx-exceptions -emit-llvm-bc $$CLANG_NODE_LIBRARY_FLAGS ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT}
QMAKE_EXTRA_COMPILERS += node_library
OTHER_FILES += $$NODE_LIBRARY_SOURCES


VuoNodeSet {
	NODE_SET = $$basename(_PRO_FILE_)
	NODE_SET = $$replace(NODE_SET,.pro,)
	NODE_SET_ZIP = $$ROOT/node/$${NODE_SET}.vuonode
	NODE_SET_DIR = $$ROOT/node/$${NODE_SET}

	NODE_OBJECTS = $$NODE_SOURCES
	NODE_OBJECTS ~= s/\\.c$/.vuonode/g

	TYPE_OBJECTS = $$TYPE_SOURCES
	TYPE_OBJECTS ~= s/\\.cc?$/.bc/g

	NODE_LIBRARY_OBJECTS = $$NODE_LIBRARY_SOURCES
	NODE_LIBRARY_OBJECTS ~= s/\\.cc?$/.bc/g
	NODE_LIBRARY_OBJECTS ~= s/\\.mm?$/.bc/g

	NODE_SET_ZIP_CONTENTS = $$NODE_OBJECTS $$TYPE_OBJECTS $$NODE_LIBRARY_OBJECTS

	createNodeSetZip.commands = \
		( [ -f $$NODE_SET_ZIP ] && rm $$NODE_SET_ZIP ) ; \
		zip --quiet $$NODE_SET_ZIP $$NODE_SET_ZIP_CONTENTS && \
		( ( [ -d $$NODE_SET_DIR/examples ] && zip --quiet -r $$NODE_SET_ZIP examples \
			--include *.vuo \
			--include *.png \
			--include *.jpg \
			--include *.mov \
			) || true ) && \
		( ( [ -d $$NODE_SET_DIR/descriptions ] && zip --quiet -r $$NODE_SET_ZIP descriptions) || true )
	createNodeSetZip.depends = $$NODE_SET_ZIP_CONTENTS
	createNodeSetZip.target = $$NODE_SET_ZIP
	POST_TARGETDEPS += $$NODE_SET_ZIP
	QMAKE_EXTRA_TARGETS += createNodeSetZip

	QMAKE_CLEAN += $$NODE_SET_ZIP
}

# Enable Qt Creator to open and autocomplete 3rd-party headers
INCLUDEPATH += \
	$$NODE_INCLUDEPATH \
	$$TYPE_INCLUDEPATH \
	$$NODE_LIBRARY_INCLUDEPATH

# Enable building libraries as normal machine code (by adding them to SOURCES)
INCLUDEPATH += \
	$$ROOT/library \
	$$ROOT/node \
	$$ROOT/runtime \
	$$ROOT/type \
	$$ROOT/type/list

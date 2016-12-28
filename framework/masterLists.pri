# Produces several variables:
#
# FRAMEWORK_VUO_HEADERS		— headers to be copied to the framework's Headers folder
# FRAMEWORK_VUO_STUB_HEADER	— filename of the Vuo.h template into which actual header names are substituted
# MASTER_VUO_HEADER_LIST	— headers to be automatically included by the framework's Vuo.h
# MODULE_OBJECTS
# QMAKE_LFLAGS
# TYPE_OBJECTS
# TYPE_HEADERS
# VUO_PRI_LIBS

CONFIG_OLD = $$CONFIG
QMAKE_CFLAGS_PRECOMPILE_OLD = $$QMAKE_CFLAGS_PRECOMPILE
QMAKE_CFLAGS_USE_PRECOMPILE_OLD = $$QMAKE_CFLAGS_USE_PRECOMPILE
QMAKE_CXXFLAGS_PRECOMPILE_OLD = $$QMAKE_CXXFLAGS_PRECOMPILE
QMAKE_CXXFLAGS_USE_PRECOMPILE_OLD = $$QMAKE_CXXFLAGS_USE_PRECOMPILE
QMAKE_EXT_CPP_OLD = $$QMAKE_EXT_CPP
QMAKE_OBJCFLAGS_PRECOMPILE_OLD = $$QMAKE_OBJCFLAGS_PRECOMPILE
QMAKE_OBJCFLAGS_USE_PRECOMPILE_OLD = $$QMAKE_OBJCFLAGS_USE_PRECOMPILE
QMAKE_OBJCXXFLAGS_PRECOMPILE_OLD = $$QMAKE_OBJCXXFLAGS_PRECOMPILE
QMAKE_OBJCXXFLAGS_USE_PRECOMPILE_OLD = $$QMAKE_OBJCXXFLAGS_USE_PRECOMPILE

# .pro files that specify HEADERS to be included in the framework:
API_HEADER_LISTS = \
	../base/base.pri \
	../compiler/compiler.pro \
	../library/library.pro \
	../node/node.pro \
	../runtime/runtime.pro \
	../type/type.pro \
	../type/list/list.pro

for(header_list, API_HEADER_LISTS) {
	include($${header_list})
	EXPLICIT_HEADERS = $$find(HEADERS, "^[^\\*]+$")  # Exclude header paths containing wildcards
	for(header, EXPLICIT_HEADERS) {
		FRAMEWORK_VUO_HEADERS.files += $$join(header, , $${dirname(header_list)}/, )
		MASTER_VUO_HEADER_LIST += $${header}
	}
	HEADERS = ""
	POST_TARGETDEPS -= $$NODE_SET_ZIP
	QMAKE_EXTRA_TARGETS -= createNodeSetZip
}

# Add the list headers (which were specified with a wildcard, excluded above).
FRAMEWORK_VUO_HEADERS.files += ../type/list/VuoList_*.h
# The list headers get included via node.h, so there's no need to add them to MASTER_VUO_HEADER_LIST.

FRAMEWORK_VUO_STUB_HEADER = "Vuo.stub.h"
VUO_PRI_LIBS = $$LIBS


# Build the TYPE_HEADERS list, containing headers for Vuo Types to be copied to the Framework.
HEADERS = ""
include(../type/type.pro)
TYPE_HEADERS = $$HEADERS
HEADERS = ""
POST_TARGETDEPS -= $$NODE_SET_ZIP
QMAKE_EXTRA_TARGETS -= createNodeSetZip


# Build the MODULE_OBJECTS list, containing compiled Vuo Modules to be copied to the Framework.
MODULE_LISTS = \
	../library/library.pro \
	../node/node.pro \
	../type/type.pro \
	../type/list/list.pro

for(module_list, MODULE_LISTS) {
	NODE_SOURCES = ""
	NODE_LIBRARY_SOURCES = ""
	NODE_LIBRARY_SHARED_NONGL_SOURCES = ""
	NODE_LIBRARY_SHARED_GL_SOURCES = ""
	NODE_LIBRARY_SHARED_SOURCES_DEPENDENT_ON_CONTEXT = ""
	TYPE_SOURCES = ""
	TYPE_LIST_SOURCES = ""

	include($${module_list})

	module_list_sources = \
		$$TYPE_SOURCES \
		$$TYPE_LIST_SOURCES \
		$$NODE_LIBRARY_SOURCES
	for(sourcefile, module_list_sources) {
		objectfile = $${dirname(module_list)}/$${basename(sourcefile)}
		objectfile ~= s/\\.cc?$/.bc
		objectfile ~= s/\\.mm?$/.bc
		MODULE_OBJECTS += $$objectfile
	}

	SHARED = $$NODE_LIBRARY_SHARED_NONGL_SOURCES $$NODE_LIBRARY_SHARED_GL_SOURCES $$NODE_LIBRARY_SHARED_SOURCES_DEPENDENT_ON_CONTEXT
	for(sourcefile, SHARED) {
		objectfile = $${dirname(module_list)}/lib$${basename(sourcefile)}
		objectfile ~= s/\\.cc?$/.dylib
		MODULE_OBJECTS += $$objectfile
		QMAKE_LFLAGS += $$objectfile
		headerfile = $${basename(sourcefile)}
		headerfile ~= s/\\.cc?$/.h
		FRAMEWORK_VUO_HEADERS.files += $${dirname(module_list)}/$$headerfile
		MASTER_VUO_HEADER_LIST += $$headerfile
	}
}

# Add node set zips to the list of files to be copied to Modules
NODE_SETS = $$system(ls -1 ../node/*/*.pro)
NODE_SETS = $$dirname(NODE_SETS)
NODE_SETS = $$basename(NODE_SETS)
NODE_SET_ZIPS = $$join(NODE_SETS,.vuonode ../node/,../node/,.vuonode)
NODE_SET_ZIPS = $$split(NODE_SET_ZIPS," ")
MODULE_OBJECTS += $$NODE_SET_ZIPS

# Add each type's objects to the list of files to be linked in to the framework dylib
for(node_set, NODE_SETS) {
	TYPE_SOURCES = ""
	include(../node/$${node_set}/$${node_set}.pro)
	POST_TARGETDEPS -= $$NODE_SET_ZIP
	QMAKE_EXTRA_TARGETS -= createNodeSetZip

	TYPE_OBJECTS = $$TYPE_SOURCES
	TYPE_OBJECTS ~= s/\\.cc?$/.o/g
	TYPE_OBJECTS = $$join(TYPE_OBJECTS," ../node/$${node_set}/",../node/$${node_set}/,)
	VUO_PRI_LIBS += $$TYPE_OBJECTS

	EXPLICIT_HEADERS = $$find(HEADERS, "^[^\\*]+$")  # Exclude header paths containing wildcards
	for(header, EXPLICIT_HEADERS) {
		FRAMEWORK_VUO_HEADERS.files += $$join(header, , ../node/$${node_set}/, )
	}
	HEADERS = ""
}

# Now reset variables as appropriate for building Vuo.framework
API_HEADER_LISTS = ""
BASE_STUB_SOURCES = ""
DEFINES = ""
DEPENDPATH = ""
HEADERS = ""
INCLUDEPATH = ""
LIBS = ""
NODE_LIBRARY_SHARED_GL_SOURCES = ""
NODE_LIBRARY_SHARED_NONGL_SOURCES = ""
NODE_LIBRARY_SHARED_SOURCES_DEPENDENT_ON_CONTEXT = ""
NODE_LIBRARY_SOURCES = ""
NODE_SOURCES = ""
OBJECTIVE_SOURCES = ""
OTHER_FILES = ""
PCH_INCLUDE_PATHS = ""
PRE_TARGETDEPS = ""
QMAKE_AR_CMD = ""
QMAKE_CFLAGS = ""
QMAKE_CFLAGS_DEBUG = ""
QMAKE_CFLAGS_I386 = ""
QMAKE_CFLAGS_RELEASE = ""
QMAKE_CFLAGS_WARN_ON = ""
QMAKE_CFLAGS_X86_64 = ""
QMAKE_CLEAN = ""
QMAKE_CXXFLAGS = ""
QMAKE_CXXFLAGS_DEBUG = ""
QMAKE_CXXFLAGS_I386 = ""
QMAKE_CXXFLAGS_RELEASE = ""
QMAKE_CXXFLAGS_WARN_ON = ""
QMAKE_CXXFLAGS_X86_64 = ""
QMAKE_LFLAGS = ""
QMAKE_LFLAGS_I386 = ""
QMAKE_LFLAGS_X86_64 = ""
QMAKE_MAC_SDK.$$basename(QMAKESPEC).$${QMAKE_MAC_SDK}.QMAKE_RANLIB = ""
QMAKE_OBJECTIVE_CFLAGS = ""
QMAKE_OBJECTIVE_CFLAGS_DEBUG = ""
QMAKE_OBJECTIVE_CFLAGS_RELEASE = ""
QMAKE_OBJECTIVE_CFLAGS_WARN_ON = ""
QMAKE_POST_LINK = ""
QMAKE_PRE_LINK = ""
RUNTIME_CXX_SOURCES = ""
RUNTIME_C_SOURCES = ""
RUNTIME_OBJC_SOURCES = ""
RUNTIME_LOADER_SOURCES = ""
SOURCES = ""
TEMPLATE = ""
TYPE_LIST_SOURCES = ""
TYPE_SOURCES = ""
VUO_INFO_PLIST = ""
VUO_INFO_PLIST_GENERATED = ""
VUO_VERSION_DEFINES = ""

CONFIG = $$CONFIG_OLD
QMAKE_CFLAGS_PRECOMPILE = $$QMAKE_CFLAGS_PRECOMPILE
QMAKE_CFLAGS_USE_PRECOMPILE = $$QMAKE_CFLAGS_USE_PRECOMPILE
QMAKE_CXXFLAGS_PRECOMPILE = $$QMAKE_CXXFLAGS_PRECOMPILE
QMAKE_CXXFLAGS_USE_PRECOMPILE = $$QMAKE_CXXFLAGS_USE_PRECOMPILE
QMAKE_EXT_CPP = $$QMAKE_EXT_CPP_OLD
QMAKE_OBJCFLAGS_PRECOMPILE = $$QMAKE_OBJCFLAGS_PRECOMPILE
QMAKE_OBJCFLAGS_USE_PRECOMPILE = $$QMAKE_OBJCFLAGS_USE_PRECOMPILE
QMAKE_OBJCXXFLAGS_PRECOMPILE = $$QMAKE_OBJCXXFLAGS_PRECOMPILE
QMAKE_OBJCXXFLAGS_USE_PRECOMPILE = $$QMAKE_OBJCXXFLAGS_USE_PRECOMPILE

CONFIG_OLD = $$CONFIG

# .pro files that specify HEADERS to be included in the framework:
API_HEADER_LISTS = \
	../base/base.pro \
	../compiler/compiler.pro \
	../library/library.pro \
	../node/node.pro \
	../node/vuo.image/vuo.image.pro \
	../node/vuo.leap/vuo.leap.pro \
	../node/vuo.math/vuo.math.pro \
	../node/vuo.midi/vuo.midi.pro \
	../node/vuo.noise/vuo.noise.pro \
	../node/vuo.syphon/vuo.syphon.pro \
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


# Build the MODULE_OBJECTS list, containing compiled Vuo Modules to be copied to the Framework.
MODULE_LISTS = \
	../library/library.pro \
	../node/node.pro \
	../type/type.pro \
	../type/list/list.pro

for(module_list, MODULE_LISTS) {
	NODE_SOURCES = ""
	NODE_LIBRARY_SOURCES = ""
	NODE_LIBRARY_SHARED_SOURCES = ""
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
		objectfile ~= s/\\.m$/.bc
		MODULE_OBJECTS += $$objectfile
	}

	SHARED = $$NODE_LIBRARY_SHARED_SOURCES $$NODE_LIBRARY_SHARED_SOURCES_DEPENDENT_ON_CONTEXT
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
}


# Now reset variables as appropriate for building Vuo.framework
SOURCES = ""
BASE_STUB_SOURCES = ""
INCLUDEPATH = ""
NODE_SOURCES = ""
NODE_LIBRARY_SOURCES = ""
NODE_LIBRARY_SHARED_SOURCES = ""
NODE_LIBRARY_SHARED_SOURCES_DEPENDENT_ON_CONTEXT = ""
OTHER_FILES = ""
QMAKE_AR_CMD = ""
QMAKE_CLEAN = ""
QMAKE_MAC_SDK.$$basename(QMAKESPEC).$${QMAKE_MAC_SDK}.QMAKE_RANLIB = ""
RUNTIME_C_SOURCES = ""
RUNTIME_CXX_SOURCES = ""
RUNTIME_LOADER_SOURCES = ""
TYPE_SOURCES = ""
TYPE_LIST_SOURCES = ""
TEMPLATE = aux
QMAKE_PRE_LINK = ""
QMAKE_POST_LINK = ""
LIBS = ""


CONFIG = $$CONFIG_OLD VuoBase VuoCompiler VuoRuntime json lib_bundle
LIBS += \
	$${ICU_ROOT}/lib/libicuuc.a \
	$${ICU_ROOT}/lib/libicudata.a
TARGET = Vuo

include(../vuo.pri)
CONFIG -= moc

VERSION = $${VUO_VERSION} # used for clang "-current_version" and "-compatibility_version" arguments
QMAKE_FRAMEWORK_VERSION = $${VUO_VERSION} # used for Vuo.framework/Versions/ subdirectory name

QMAKE_LFLAGS += $${QMAKE_LFLAGS_X86_64}
QMAKE_LFLAGS += -Wl,-force_load,$${ROOT}/base/libVuoBase.a
QMAKE_LFLAGS += -Wl,-force_load,$${ROOT}/compiler/libVuoCompiler.a
QMAKE_LFLAGS += -Wl,-force_load,$${ROOT}/type/libVuoType.a
QMAKE_LFLAGS += -Wl,-force_load,$${ROOT}/type/list/libVuoTypeList.a


# Add the third-party libraries that Vuo nodes/types depend on
MODULE_OBJECTS += \
	$$ICU_ROOT/lib/libicuuc.a \
	$$ICU_ROOT/lib/libicudata.a \
	$$JSONC_ROOT/lib/libjson.a \
	$$MUPARSER_ROOT/lib/libmuparser.a \
	$$FREEIMAGE_ROOT/lib/libfreeimage.a \
	$$CURL_ROOT/lib/libcurl.a \
	$$RTMIDI_ROOT/lib/librtmidi.a \
	$$FFMPEG_ROOT/lib/libavcodec.dylib \
	$$FFMPEG_ROOT/lib/libavdevice.dylib \
	$$FFMPEG_ROOT/lib/libavfilter.dylib \
	$$FFMPEG_ROOT/lib/libavformat.dylib \
	$$FFMPEG_ROOT/lib/libavutil.dylib \
	$$FFMPEG_ROOT/lib/libswresample.dylib \
	$$FFMPEG_ROOT/lib/libswscale.dylib \
	$$LIBUSB_ROOT/lib/libusb.dylib \
	$$LIBFREENECT_ROOT/lib/libfreenect.dylib \
	$$OSCPACK_ROOT/lib/liboscpack.a \
	$$ASSIMP_ROOT/lib/libassimp.a
!equals(MAC_VERSION, "10.6") {
	MODULE_OBJECTS += $$ROOT/node/vuo.leap/Leap/libLeap.dylib
}


# Create and populate Headers directory
HEADERS_DEST_DIR = "Vuo.framework/Versions/$${QMAKE_FRAMEWORK_VERSION}/Headers"
createHeadersDir.commands += rm -rf $${HEADERS_DEST_DIR} &&
createHeadersDir.commands += mkdir -p $${HEADERS_DEST_DIR} &&
createHeadersDir.commands += ln -s . $${HEADERS_DEST_DIR}/Vuo &&
createHeadersDir.commands += cp $$FRAMEWORK_VUO_HEADERS.files $${HEADERS_DEST_DIR} &&
createHeadersDir.commands += ./generateFrameworkHeader.pl $${FRAMEWORK_VUO_STUB_HEADER} $${MASTER_VUO_HEADER_LIST} > $${HEADERS_DEST_DIR}/Vuo.h
createHeadersDir.depends += $$FRAMEWORK_VUO_HEADERS.files $$FRAMEWORK_VUO_STUB_HEADER
createHeadersDir.target = $${HEADERS_DEST_DIR}
POST_TARGETDEPS += $${HEADERS_DEST_DIR}
QMAKE_EXTRA_TARGETS += createHeadersDir


# Create Resources directory; populate it with Info.plist
RESOURCES_DEST_DIR = "Vuo.framework/Versions/$${QMAKE_FRAMEWORK_VERSION}/Resources"
createResourcesDir.commands += rm -rf $${RESOURCES_DEST_DIR}
createResourcesDir.commands += && mkdir -p $${RESOURCES_DEST_DIR}
createResourcesDir.commands += && cat Info.plist
createResourcesDir.commands += | sed '"s/@SHORT_VERSION@/$$VUO_VERSION \\(r`svnversion -n` on `date +%Y.%m.%d`\\)/"'
createResourcesDir.commands += > $${RESOURCES_DEST_DIR}/Info.plist
createResourcesDir.depends += Info.plist
createResourcesDir.target = $${RESOURCES_DEST_DIR}
POST_TARGETDEPS += $${RESOURCES_DEST_DIR}
QMAKE_EXTRA_TARGETS += createResourcesDir


# Create and populate Licenses directory
LICENSES_DEST_DIR = "Vuo.framework/Versions/$${QMAKE_FRAMEWORK_VERSION}/Licenses"
createLicensesDir.commands += rm -rf $${LICENSES_DEST_DIR} &&
createLicensesDir.commands += mkdir -p $${LICENSES_DEST_DIR} &&
createLicensesDir.commands += cp ../license/* $${LICENSES_DEST_DIR}
createLicensesDir.depends += ../license/*
createLicensesDir.target = $${LICENSES_DEST_DIR}
POST_TARGETDEPS += $${LICENSES_DEST_DIR}
QMAKE_EXTRA_TARGETS += createLicensesDir

# Create Modules directory.
# Populate it with Vuo Node Classes, Types, and Libraries.
# Update linker ids for dynamic libraries.
MODULES_DEST_DIR = "Vuo.framework/Versions/$${QMAKE_FRAMEWORK_VERSION}/Modules"
copyModules.commands = \
	   rm -rf $$MODULES_DEST_DIR \
	&& mkdir -p $$MODULES_DEST_DIR \
	&& cp $$MODULE_OBJECTS $$MODULES_DEST_DIR \
	&& install_name_tool -id "@rpath/$$MODULES_DEST_DIR/libfreenect.dylib" "$$MODULES_DEST_DIR/libfreenect.dylib" \
	&& install_name_tool -change "$$LIBUSB_ROOT/lib/libusb-1.0.0.dylib" "@rpath/$$MODULES_DEST_DIR/libusb.dylib" "$$MODULES_DEST_DIR/libfreenect.dylib" \
	&& install_name_tool -id "@rpath/$$MODULES_DEST_DIR/libusb.dylib" "$$MODULES_DEST_DIR/libusb.dylib" \
	&& install_name_tool -id "@rpath/$$MODULES_DEST_DIR/libavcodec.dylib" "$$MODULES_DEST_DIR/libavcodec.dylib" \
	&& install_name_tool -change "$$FFMPEG_ROOT/lib/libavutil.52.dylib" "@rpath/$$MODULES_DEST_DIR/libavutil.dylib" "$$MODULES_DEST_DIR/libavcodec.dylib" \
	&& install_name_tool -id "@rpath/$$MODULES_DEST_DIR/libavdevice.dylib" "$$MODULES_DEST_DIR/libavdevice.dylib" \
	&& install_name_tool -change "$$FFMPEG_ROOT/lib/libavfilter.3.dylib" "@rpath/$$MODULES_DEST_DIR/libavfilter.dylib" "$$MODULES_DEST_DIR/libavdevice.dylib" \
	&& install_name_tool -change "$$FFMPEG_ROOT/lib/libavformat.55.dylib" "@rpath/$$MODULES_DEST_DIR/libavformat.dylib" "$$MODULES_DEST_DIR/libavdevice.dylib" \
	&& install_name_tool -change "$$FFMPEG_ROOT/lib/libavcodec.55.dylib" "@rpath/$$MODULES_DEST_DIR/libavcodec.dylib" "$$MODULES_DEST_DIR/libavdevice.dylib" \
	&& install_name_tool -change "$$FFMPEG_ROOT/lib/libavutil.52.dylib" "@rpath/$$MODULES_DEST_DIR/libavutil.dylib" "$$MODULES_DEST_DIR/libavdevice.dylib" \
	&& install_name_tool -id "@rpath/$$MODULES_DEST_DIR/libavfilter.dylib" "$$MODULES_DEST_DIR/libavfilter.dylib" \
	&& install_name_tool -change "$$FFMPEG_ROOT/lib/libavformat.55.dylib" "@rpath/$$MODULES_DEST_DIR/libavformat.dylib" "$$MODULES_DEST_DIR/libavfilter.dylib" \
	&& install_name_tool -change "$$FFMPEG_ROOT/lib/libavcodec.55.dylib" "@rpath/$$MODULES_DEST_DIR/libavcodec.dylib" "$$MODULES_DEST_DIR/libavfilter.dylib" \
	&& install_name_tool -change "$$FFMPEG_ROOT/lib/libavutil.52.dylib" "@rpath/$$MODULES_DEST_DIR/libavutil.dylib" "$$MODULES_DEST_DIR/libavfilter.dylib" \
	&& install_name_tool -change "$$FFMPEG_ROOT/lib/libswresample.0.dylib" "@rpath/$$MODULES_DEST_DIR/libswresample.dylib" "$$MODULES_DEST_DIR/libavfilter.dylib" \
	&& install_name_tool -change "$$FFMPEG_ROOT/lib/libswscale.2.dylib" "@rpath/$$MODULES_DEST_DIR/libswscale.dylib" "$$MODULES_DEST_DIR/libavfilter.dylib" \
	&& install_name_tool -id "@rpath/$$MODULES_DEST_DIR/libavformat.dylib" "$$MODULES_DEST_DIR/libavformat.dylib" \
	&& install_name_tool -change "$$FFMPEG_ROOT/lib/libavcodec.55.dylib" "@rpath/$$MODULES_DEST_DIR/libavcodec.dylib" "$$MODULES_DEST_DIR/libavformat.dylib" \
	&& install_name_tool -change "$$FFMPEG_ROOT/lib/libavutil.52.dylib" "@rpath/$$MODULES_DEST_DIR/libavutil.dylib" "$$MODULES_DEST_DIR/libavformat.dylib" \
	&& install_name_tool -id "@rpath/$$MODULES_DEST_DIR/libswresample.dylib" "$$MODULES_DEST_DIR/libswresample.dylib" \
	&& install_name_tool -change "$$FFMPEG_ROOT/lib/libavutil.52.dylib" "@rpath/$$MODULES_DEST_DIR/libavutil.dylib" "$$MODULES_DEST_DIR/libswresample.dylib" \
	&& install_name_tool -id "@rpath/$$MODULES_DEST_DIR/libswscale.dylib" "$$MODULES_DEST_DIR/libswscale.dylib" \
	&& install_name_tool -change "$$FFMPEG_ROOT/lib/libavutil.52.dylib" "@rpath/$$MODULES_DEST_DIR/libavutil.dylib" "$$MODULES_DEST_DIR/libswscale.dylib" \
	&& install_name_tool -id "@rpath/$$MODULES_DEST_DIR/libavutil.dylib" "$$MODULES_DEST_DIR/libavutil.dylib"
!equals(MAC_VERSION, "10.6") {
	copyModules.commands += && install_name_tool -id "@rpath/$$MODULES_DEST_DIR/libLeap.dylib" "$$MODULES_DEST_DIR/libLeap.dylib"
}
copyModules.depends += $$MODULE_OBJECTS
copyModules.target = $${MODULES_DEST_DIR}
POST_TARGETDEPS += $${MODULES_DEST_DIR}
QMAKE_EXTRA_TARGETS += copyModules


FRAMEWORKS_DEST_DIR = "Vuo.framework/Versions/$${QMAKE_FRAMEWORK_VERSION}/Frameworks"

# Copy VuoRuntime files to the Frameworks directory
VUORUNTIME_DEST_DIR = "$$FRAMEWORKS_DEST_DIR/VuoRuntime.framework"
copyVuoRuntime.commands += rm -Rf $${VUORUNTIME_DEST_DIR} &&
copyVuoRuntime.commands += mkdir -p $${VUORUNTIME_DEST_DIR} &&
copyVuoRuntime.commands += cp $$ROOT/runtime/*\\.bc $$ROOT/runtime/libVuoHeap.dylib $$ROOT/runtime/VuoCompositionLoader $${VUORUNTIME_DEST_DIR} &&
copyVuoRuntime.commands += cp $$ROOT/base/VuoCompositionStub.dylib $${VUORUNTIME_DEST_DIR}
copyVuoRuntime.depends += $$ROOT/runtime/*.bc
copyVuoRuntime.depends += $$ROOT/runtime/libVuoHeap.dylib
copyVuoRuntime.depends += $$ROOT/runtime/VuoCompositionLoader
copyVuoRuntime.depends += $$ROOT/base/VuoCompositionStub.dylib
copyVuoRuntime.target = $${VUORUNTIME_DEST_DIR}
POST_TARGETDEPS += $${VUORUNTIME_DEST_DIR}
QMAKE_EXTRA_TARGETS += copyVuoRuntime

# Copy ZeroMQ library to the Frameworks directory
ZEROMQ_LIBS_DEST_DIR = "$$FRAMEWORKS_DEST_DIR/zmq.framework"
copyZeroMQLibs.commands += rm -rf $${ZEROMQ_LIBS_DEST_DIR} &&
copyZeroMQLibs.commands += mkdir -p $${ZEROMQ_LIBS_DEST_DIR} &&
copyZeroMQLibs.commands += cp $${ZMQ_ROOT}/lib/libzmq.a $${ZEROMQ_LIBS_DEST_DIR}
copyZeroMQLibs.target = $${ZEROMQ_LIBS_DEST_DIR}
POST_TARGETDEPS += $${ZEROMQ_LIBS_DEST_DIR}
QMAKE_EXTRA_TARGETS += copyZeroMQLibs


# Copy ZeroMQ headers to the Frameworks directory
ZEROMQ_HEADERS_DEST_DIR = "$$FRAMEWORKS_DEST_DIR/zmq.framework/Headers"
copyZeroMQHeaders.commands += rm -rf $${ZEROMQ_HEADERS_DEST_DIR} &&
copyZeroMQHeaders.commands += mkdir -p $${ZEROMQ_HEADERS_DEST_DIR} &&
copyZeroMQHeaders.commands += cp -r $${ZMQ_ROOT}/include/* $${ZEROMQ_HEADERS_DEST_DIR}
copyZeroMQHeaders.target = $${ZEROMQ_HEADERS_DEST_DIR}
POST_TARGETDEPS += $${ZEROMQ_HEADERS_DEST_DIR}
QMAKE_EXTRA_TARGETS += copyZeroMQHeaders


# Copy JSON-C library to the Frameworks directory
JSON_LIBS_DEST_DIR = "$$FRAMEWORKS_DEST_DIR/json.framework/"
copyJsonLibs.commands += rm -rf $${JSON_LIBS_DEST_DIR} &&
copyJsonLibs.commands += mkdir -p $${JSON_LIBS_DEST_DIR} &&
copyJsonLibs.commands += cp $${JSONC_ROOT}/lib/libjson.dylib $${JSON_LIBS_DEST_DIR} &&
copyJsonLibs.commands += chmod +wx "$$JSON_LIBS_DEST_DIR/libjson.dylib" &&
copyJsonLibs.commands += install_name_tool -id "@rpath/$$JSON_LIBS_DEST_DIR/libjson.dylib" "$$JSON_LIBS_DEST_DIR/libjson.dylib"
copyJsonLibs.target = $${JSON_LIBS_DEST_DIR}
POST_TARGETDEPS += $${JSON_LIBS_DEST_DIR}
QMAKE_EXTRA_TARGETS += copyJsonLibs


# Copy JSON-C headers to the frameworks directory
JSON_HEADERS_DEST_DIR = "$$FRAMEWORKS_DEST_DIR/json.framework/Headers"
copyJsonHeaders.commands += rm -rf $${JSON_HEADERS_DEST_DIR} &&
copyJsonHeaders.commands += mkdir -p $${JSON_HEADERS_DEST_DIR} &&
copyJsonHeaders.commands += cp -r $${JSONC_ROOT}/include/json/* $${JSON_HEADERS_DEST_DIR}
copyJsonHeaders.target = $${JSON_HEADERS_DEST_DIR}
POST_TARGETDEPS += $${JSON_HEADERS_DEST_DIR}
QMAKE_EXTRA_TARGETS += copyJsonHeaders


# Copy CRuntime files to the Frameworks directory
CRUNTIME_LIBS_DEST_DIR = "$$FRAMEWORKS_DEST_DIR/CRuntime.framework"
copyCRuntime.commands += rm -rf $${CRUNTIME_LIBS_DEST_DIR} &&
copyCRuntime.commands += mkdir -p $${CRUNTIME_LIBS_DEST_DIR} &&
copyCRuntime.commands += cp $${ROOT}/compiler/binary/crt1.o $${CRUNTIME_LIBS_DEST_DIR}/
copyCRuntime.target = $${CRUNTIME_LIBS_DEST_DIR}
POST_TARGETDEPS += $${CRUNTIME_LIBS_DEST_DIR}
QMAKE_EXTRA_TARGETS += copyCRuntime


# Copy system headers installed by XCode Command Line Tools to the Frameworks directory
MACOS_HEADERS_DEST_DIR = "$$FRAMEWORKS_DEST_DIR/MacOS.framework/Headers"
copyMacOsHeaders.commands += rm -rf $${MACOS_HEADERS_DEST_DIR} &&
copyMacOsHeaders.commands += mkdir -p $${MACOS_HEADERS_DEST_DIR}/OpenGL &&
copyMacOsHeaders.commands += cp /usr/include/*.h $${MACOS_HEADERS_DEST_DIR} &&
#copyMacOsHeaders.commands += cp -r /usr/include/_types $${MACOS_HEADERS_DEST_DIR}/ ;
copyMacOsHeaders.commands += cp -r /usr/include/architecture $${MACOS_HEADERS_DEST_DIR}/ ;
copyMacOsHeaders.commands += cp -r /usr/include/dispatch $${MACOS_HEADERS_DEST_DIR}/ ;
copyMacOsHeaders.commands += cp -r /usr/include/i386 $${MACOS_HEADERS_DEST_DIR}/ ;
copyMacOsHeaders.commands += cp -r /usr/include/libkern $${MACOS_HEADERS_DEST_DIR}/ ;
copyMacOsHeaders.commands += cp -r /usr/include/mach $${MACOS_HEADERS_DEST_DIR}/ ;
copyMacOsHeaders.commands += cp -r /usr/include/machine $${MACOS_HEADERS_DEST_DIR}/ ;
copyMacOsHeaders.commands += cp -r /usr/include/os $${MACOS_HEADERS_DEST_DIR}/ ;
copyMacOsHeaders.commands += cp -r /usr/include/secure $${MACOS_HEADERS_DEST_DIR}/ ;
copyMacOsHeaders.commands += cp -r /usr/include/sys $${MACOS_HEADERS_DEST_DIR}/ ;
copyMacOsHeaders.commands += cp /System/Library/Frameworks/OpenGL.framework/Versions/A/Headers/*.h $${MACOS_HEADERS_DEST_DIR}/OpenGL
copyMacOsHeaders.target = $${MACOS_HEADERS_DEST_DIR}
POST_TARGETDEPS += $${MACOS_HEADERS_DEST_DIR}
QMAKE_EXTRA_TARGETS += copyMacOsHeaders


# Copy Syphon library to the Frameworks directory
SYPHON_LIBS_DEST_DIR = "$$FRAMEWORKS_DEST_DIR/Syphon.framework"
copySyphonLibs.commands = \
	   rm -rf $${SYPHON_LIBS_DEST_DIR} \
	&& cp -r $$ROOT/node/vuo.syphon/Syphon/Syphon.framework $${FRAMEWORKS_DEST_DIR} \
	&& install_name_tool -id "@rpath/$$SYPHON_LIBS_DEST_DIR/Syphon" "$$SYPHON_LIBS_DEST_DIR/Syphon"
copySyphonLibs.target = $${SYPHON_LIBS_DEST_DIR}
POST_TARGETDEPS += $${SYPHON_LIBS_DEST_DIR}
QMAKE_EXTRA_TARGETS += copySyphonLibs


# Copy input editor SDK, fonts, Qt frameworks, and Qt plugins to the resources folder (to be bundled as part of the SDK, but not Vuo.framework)
RESOURCES_SUBDIR = resources

INPUT_EDITOR_WIDGETS_SRC_DIR = $$ROOT/type/inputEditor/widget
INPUT_EDITOR_WIDGETS_DEST_DIR = $$RESOURCES_SUBDIR/inputEditorWidgets
copyInputEditorWidgets.commands = \
	mkdir -p $$INPUT_EDITOR_WIDGETS_DEST_DIR \
	&& cp -a \
		$$INPUT_EDITOR_WIDGETS_SRC_DIR/*.hh \
		$$INPUT_EDITOR_WIDGETS_SRC_DIR/*.a \
		$$INPUT_EDITOR_WIDGETS_SRC_DIR/*.pch \
		$$INPUT_EDITOR_WIDGETS_DEST_DIR
copyInputEditorWidgets.target = $$INPUT_EDITOR_WIDGETS_DEST_DIR/libwidget.a
POST_TARGETDEPS += $$INPUT_EDITOR_WIDGETS_DEST_DIR/libwidget.a
QMAKE_EXTRA_TARGETS += copyInputEditorWidgets

copyFonts.commands = \
	mkdir -p $$RESOURCES_SUBDIR \
	&& cp -a \
		../renderer/font/*.otf \
		$$RESOURCES_SUBDIR
copyFonts.target = $$RESOURCES_SUBDIR/Signika-Light.otf
POST_TARGETDEPS += $$RESOURCES_SUBDIR/Signika-Light.otf
QMAKE_EXTRA_TARGETS += copyFonts

QTPLUGINS_DEST_DIR = "$$RESOURCES_SUBDIR/QtPlugins"
copyAndCleanQtFrameworks.commands = \
	mkdir -p $$RESOURCES_SUBDIR \
	&& cp -a \
		$${QT_ROOT}/lib/QtCore.framework \
		$${QT_ROOT}/lib/QtGui.framework \
		$${QT_ROOT}/lib/QtWidgets.framework \
		$${QT_ROOT}/lib/QtMacExtras.framework \
		$${QT_ROOT}/lib/QtPrintSupport.framework \
		$${QT_ROOT}/lib/QtOpenGL.framework \
		$${QT_ROOT}/lib/QtXml.framework \
		$$RESOURCES_SUBDIR \
	&& rm -Rf "$$RESOURCES_SUBDIR/Qt*.framework/Contents" \
	&& rm -f  "$$RESOURCES_SUBDIR/Qt*.framework/Qt*.prl" \
	&& rm -f  "$$RESOURCES_SUBDIR/Qt*.framework/Qt*_debug" \
	&& rm -f  "$$RESOURCES_SUBDIR/Qt*.framework/Versions/$$QT_MAJOR_VERSION/Qt*_debug" \
	&& rm -f  "$$RESOURCES_SUBDIR/Qt*.framework/Headers" \
	&& rm -Rf "$$RESOURCES_SUBDIR/Qt*.framework/Versions/$$QT_MAJOR_VERSION/Headers" \
	&& chmod +wx "$$RESOURCES_SUBDIR/QtCore.framework/Versions/$$QT_MAJOR_VERSION/QtCore" \
	&& install_name_tool -id "@rpath/QtCore.framework/QtCore" "$$RESOURCES_SUBDIR/QtCore.framework/QtCore" \
	&& chmod +wx "$$RESOURCES_SUBDIR/QtGui.framework/Versions/$$QT_MAJOR_VERSION/QtGui" \
	&& install_name_tool -id "@rpath/QtGui.framework/QtGui" "$$RESOURCES_SUBDIR/QtGui.framework/QtGui" \
	&& chmod +wx "$$RESOURCES_SUBDIR/QtWidgets.framework/Versions/$$QT_MAJOR_VERSION/QtWidgets" \
	&& install_name_tool -id "@rpath/QtWidgets.framework/QtWidgets" "$$RESOURCES_SUBDIR/QtWidgets.framework/QtWidgets" \
	&& chmod +wx "$$RESOURCES_SUBDIR/QtMacExtras.framework/Versions/$$QT_MAJOR_VERSION/QtMacExtras" \
	&& install_name_tool -id "@rpath/QtMacExtras.framework/QtMacExtras" "$$RESOURCES_SUBDIR/QtMacExtras.framework/QtMacExtras" \
	&& chmod +wx "$$RESOURCES_SUBDIR/QtPrintSupport.framework/Versions/$$QT_MAJOR_VERSION/QtPrintSupport" \
	&& install_name_tool -id "@rpath/QtPrintSupport.framework/QtPrintSupport" "$$RESOURCES_SUBDIR/QtPrintSupport.framework/QtPrintSupport" \
	&& chmod +wx "$$RESOURCES_SUBDIR/QtOpenGL.framework/Versions/$$QT_MAJOR_VERSION/QtOpenGL" \
	&& install_name_tool -id "@rpath/QtOpenGL.framework/QtOpenGL" "$$RESOURCES_SUBDIR/QtOpenGL.framework/QtOpenGL" \
	&& chmod +wx "$$RESOURCES_SUBDIR/QtXml.framework/Versions/$$QT_MAJOR_VERSION/QtXml" \
	&& install_name_tool -id "@rpath/QtXml.framework/QtXml" "$$RESOURCES_SUBDIR/QtXml.framework/QtXml" \
	&& rm -rf $$QTPLUGINS_DEST_DIR \
	&& mkdir -p $$QTPLUGINS_DEST_DIR \
	&& cp -R $$QT_ROOT/plugins/accessible $$QTPLUGINS_DEST_DIR/accessible \
	&& mkdir -p $$QTPLUGINS_DEST_DIR/platforms \
	&& cp -R $$QT_ROOT/plugins/platforms/libqcocoa.dylib $$QTPLUGINS_DEST_DIR/platforms \
	&& rm -f "$$QTPLUGINS_DEST_DIR/*/*_debug.dylib" \
	&& ./fixRpaths.sh $$QT_ROOT $$RESOURCES_SUBDIR $$QT_MAJOR_VERSION \
	&& ./fixRpaths.sh /usr/local $$RESOURCES_SUBDIR $$QT_MAJOR_VERSION
copyAndCleanQtFrameworks.target = $$RESOURCES_SUBDIR/QtCore.framework
POST_TARGETDEPS += $$RESOURCES_SUBDIR/QtCore.framework
QMAKE_EXTRA_TARGETS += copyAndCleanQtFrameworks

# Link the LIBS from vuo.pri to produce Vuo.framework/Vuo
LIBS += $$VUO_PRI_LIBS
LIBS += \
	-framework CoreFoundation \
	-framework OpenGL \
	-framework IOSurface \
	$$ROOT/library/VuoImageRenderer.o \
	$$ROOT/runtime/libVuoHeap.dylib


# Copy Clang headers to the Frameworks directory
CLANG_HEADERS_DEST_DIR = "$$FRAMEWORKS_DEST_DIR/clang.framework/Headers"
copyClangHeaders.commands += rm -rf $${CLANG_HEADERS_DEST_DIR} &&
copyClangHeaders.commands += mkdir -p $${CLANG_HEADERS_DEST_DIR} &&
copyClangHeaders.commands += cp -r $${LLVM_ROOT}/include/clang/* $${CLANG_HEADERS_DEST_DIR}
copyClangHeaders.target = $${CLANG_HEADERS_DEST_DIR}
POST_TARGETDEPS += $${CLANG_HEADERS_DEST_DIR}
QMAKE_EXTRA_TARGETS += copyClangHeaders


# Copy LLVM headers to the Frameworks directory
LLVM_HEADERS_DEST_DIR = "$$FRAMEWORKS_DEST_DIR/llvm.framework/Headers"
copyLLVMHeaders.commands += rm -rf $${LLVM_HEADERS_DEST_DIR} &&
copyLLVMHeaders.commands += mkdir -p $${LLVM_HEADERS_DEST_DIR} &&
copyLLVMHeaders.commands += cp -r $${LLVM_ROOT}/include/llvm/* $${LLVM_HEADERS_DEST_DIR}
copyLLVMHeaders.target = $${LLVM_HEADERS_DEST_DIR}
POST_TARGETDEPS += $${LLVM_HEADERS_DEST_DIR}
QMAKE_EXTRA_TARGETS += copyLLVMHeaders


# Copy Graphviz libraries to the Frameworks directory
GRAPHVIZ_LIBS_DEST_DIR = "$$FRAMEWORKS_DEST_DIR/graphviz.framework"
copyGraphvizLibs.commands = \
	   rm -rf "$${GRAPHVIZ_LIBS_DEST_DIR}" \
	&& mkdir -p "$${GRAPHVIZ_LIBS_DEST_DIR}" \
	&& mkdir -p "$${GRAPHVIZ_LIBS_DEST_DIR}/graphviz" \
	&& cp "$${GRAPHVIZ_ROOT}/lib/graphviz/libgvplugin_dot_layout.6.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}" \
	&& ln -s "libgvplugin_dot_layout.6.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}/libgvplugin_dot_layout.dylib" \
	&& cp "$${GRAPHVIZ_ROOT}/lib/graphviz/libgvplugin_core.6.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}" \
	&& ln -s "libgvplugin_core.6.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}/libgvplugin_core.dylib" \
	&& cp "$${GRAPHVIZ_ROOT}/lib/libcdt.5.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}" \
	&& ln -s "libcdt.5.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}/libcdt.dylib" \
	&& cp "$${GRAPHVIZ_ROOT}/lib/libgraph.5.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}" \
	&& ln -s "libgraph.5.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}/libgraph.dylib" \
	&& cp "$${GRAPHVIZ_ROOT}/lib/libgvc.6.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}" \
	&& ln -s "libgvc.6.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}/libgvc.dylib" \
	&& cp "$${GRAPHVIZ_ROOT}/lib/libpathplan.4.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}" \
	&& ln -s "libpathplan.4.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}/libpathplan.dylib" \
	&& cp "$${GRAPHVIZ_ROOT}/lib/libxdot.4.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}" \
	&& ln -s "libxdot.4.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}/libxdot.dylib" \
	&& chmod +w "$${GRAPHVIZ_LIBS_DEST_DIR}/*.dylib" \
	&& install_name_tool -id "@rpath/$${GRAPHVIZ_LIBS_DEST_DIR}/libgvplugin_core.6.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}/libgvplugin_core.6.dylib" \
	&& install_name_tool -change "$${GRAPHVIZ_ROOT}/lib/libcdt.5.dylib" "@rpath/$${GRAPHVIZ_LIBS_DEST_DIR}/libcdt.5.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}/libgvplugin_core.6.dylib" \
	&& install_name_tool -change "$${GRAPHVIZ_ROOT}/lib/libgraph.5.dylib" "@rpath/$${GRAPHVIZ_LIBS_DEST_DIR}/libgraph.5.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}/libgvplugin_core.6.dylib" \
	&& install_name_tool -change "$${GRAPHVIZ_ROOT}/lib/libgvc.6.dylib" "@rpath/$${GRAPHVIZ_LIBS_DEST_DIR}/libgvc.6.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}/libgvplugin_core.6.dylib" \
	&& install_name_tool -change "$${GRAPHVIZ_ROOT}/lib/libpathplan.4.dylib" "@rpath/$${GRAPHVIZ_LIBS_DEST_DIR}/libpathplan.4.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}/libgvplugin_core.6.dylib" \
	&& install_name_tool -change "$${GRAPHVIZ_ROOT}/lib/libxdot.4.dylib" "@rpath/$${GRAPHVIZ_LIBS_DEST_DIR}/libxdot.4.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}/libgvplugin_core.6.dylib" \
	&& install_name_tool -id "@rpath/$${GRAPHVIZ_LIBS_DEST_DIR}/libgvplugin_dot_layout.6.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}/libgvplugin_dot_layout.6.dylib" \
	&& install_name_tool -change "$${GRAPHVIZ_ROOT}/lib/libcdt.5.dylib" "@rpath/$${GRAPHVIZ_LIBS_DEST_DIR}/libcdt.5.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}/libgvplugin_dot_layout.6.dylib" \
	&& install_name_tool -change "$${GRAPHVIZ_ROOT}/lib/libgraph.5.dylib" "@rpath/$${GRAPHVIZ_LIBS_DEST_DIR}/libgraph.5.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}/libgvplugin_dot_layout.6.dylib" \
	&& install_name_tool -change "$${GRAPHVIZ_ROOT}/lib/libgvc.6.dylib" "@rpath/$${GRAPHVIZ_LIBS_DEST_DIR}/libgvc.6.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}/libgvplugin_dot_layout.6.dylib" \
	&& install_name_tool -change "$${GRAPHVIZ_ROOT}/lib/libpathplan.4.dylib" "@rpath/$${GRAPHVIZ_LIBS_DEST_DIR}/libpathplan.4.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}/libgvplugin_dot_layout.6.dylib" \
	&& install_name_tool -change "$${GRAPHVIZ_ROOT}/lib/libxdot.4.dylib" "@rpath/$${GRAPHVIZ_LIBS_DEST_DIR}/libxdot.4.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}/libgvplugin_dot_layout.6.dylib" \
	&& install_name_tool -id "@rpath/$${GRAPHVIZ_LIBS_DEST_DIR}/libcdt.5.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}/libcdt.5.dylib" \
	&& install_name_tool -id "@rpath/$${GRAPHVIZ_LIBS_DEST_DIR}/libgraph.5.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}/libgraph.5.dylib" \
	&& install_name_tool -change "$${GRAPHVIZ_ROOT}/lib/libcdt.5.dylib" "@rpath/$${GRAPHVIZ_LIBS_DEST_DIR}/libcdt.5.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}/libgraph.5.dylib" \
	&& install_name_tool -id "@rpath/$${GRAPHVIZ_LIBS_DEST_DIR}/libgvc.6.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}/libgvc.6.dylib" \
	&& install_name_tool -change "$${GRAPHVIZ_ROOT}/lib/libcdt.5.dylib" "@rpath/$${GRAPHVIZ_LIBS_DEST_DIR}/libcdt.5.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}/libgvc.6.dylib" \
	&& install_name_tool -change "$${GRAPHVIZ_ROOT}/lib/libgraph.5.dylib" "@rpath/$${GRAPHVIZ_LIBS_DEST_DIR}/libgraph.5.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}/libgvc.6.dylib" \
	&& install_name_tool -change "$${GRAPHVIZ_ROOT}/lib/libpathplan.4.dylib" "@rpath/$${GRAPHVIZ_LIBS_DEST_DIR}/libpathplan.4.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}/libgvc.6.dylib" \
	&& install_name_tool -change "$${GRAPHVIZ_ROOT}/lib/libxdot.4.dylib" "@rpath/$${GRAPHVIZ_LIBS_DEST_DIR}/libxdot.4.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}/libgvc.6.dylib" \
	&& install_name_tool -id "@rpath/$${GRAPHVIZ_LIBS_DEST_DIR}/libpathplan.4.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}/libpathplan.4.dylib" \
	&& install_name_tool -id "@rpath/$${GRAPHVIZ_LIBS_DEST_DIR}/libxdot.4.dylib" "$${GRAPHVIZ_LIBS_DEST_DIR}/libxdot.4.dylib" \
	&& chmod -w "$${GRAPHVIZ_LIBS_DEST_DIR}/*.dylib"
copyGraphvizLibs.target = $${GRAPHVIZ_LIBS_DEST_DIR}
POST_TARGETDEPS += $${GRAPHVIZ_LIBS_DEST_DIR}
QMAKE_EXTRA_TARGETS += copyGraphvizLibs



# Link Vuo.framework's main shared library
# (Link via an extra target instead of using qmake's TEMPLATE=lib, so that we have finer control over its dependencies.)
VUO_FRAMEWORK_BINARY = Vuo.framework/Versions/$$VUO_VERSION/Vuo
CURRENT_LINK = Vuo.framework/Versions/Current
linkVuoFramework.commands = \
	$$QMAKE_LINK \
		$$QMAKE_LFLAGS \
		$$QMAKE_LFLAGS_SHLIB \
		-current_version $$VUO_VERSION \
		-install_name @rpath/$$VUO_FRAMEWORK_BINARY \
		$$LIBS \
		-o $$VUO_FRAMEWORK_BINARY \
	&& install_name_tool -change "/usr/local/lib/graphviz/libgvplugin_core.6.dylib" "@rpath/$${GRAPHVIZ_LIBS_DEST_DIR}/libgvplugin_core.6.dylib" "$$VUO_FRAMEWORK_BINARY" \
	&& install_name_tool -change "/usr/local/lib/graphviz/libgvplugin_dot_layout.6.dylib" "@rpath/$${GRAPHVIZ_LIBS_DEST_DIR}/libgvplugin_dot_layout.6.dylib" "$$VUO_FRAMEWORK_BINARY" \
	&& install_name_tool -change "/usr/local/lib/libcdt.5.dylib" "@rpath/$${GRAPHVIZ_LIBS_DEST_DIR}/libcdt.5.dylib" "$$VUO_FRAMEWORK_BINARY" \
	&& install_name_tool -change "/usr/local/lib/libgraph.5.dylib" "@rpath/$${GRAPHVIZ_LIBS_DEST_DIR}/libgraph.5.dylib" "$$VUO_FRAMEWORK_BINARY" \
	&& install_name_tool -change "/usr/local/lib/libgvc.6.dylib" "@rpath/$${GRAPHVIZ_LIBS_DEST_DIR}/libgvc.6.dylib" "$$VUO_FRAMEWORK_BINARY" \
	&& install_name_tool -change "/usr/local/lib/libpathplan.4.dylib" "@rpath/$${GRAPHVIZ_LIBS_DEST_DIR}/libpathplan.4.dylib" "$$VUO_FRAMEWORK_BINARY" \
	&& install_name_tool -change "/usr/local/lib/libxdot.4.dylib" "@rpath/$${GRAPHVIZ_LIBS_DEST_DIR}/libxdot.4.dylib" "$$VUO_FRAMEWORK_BINARY" \
	&& rm -f $$CURRENT_LINK
linkVuoFramework.target = $$VUO_FRAMEWORK_BINARY
linkVuoFramework.depends = \
	../library/libVuoGlContext.dylib \
	../library/libVuoGlPool.dylib \
	../base/libVuoBase.a \
	../compiler/libVuoCompiler.a \
	../type/libVuoType.a \
	../type/list/libVuoTypeList.a \
	../library/VuoImageRenderer.o \
	../runtime/libVuoHeap.dylib
POST_TARGETDEPS += $$VUO_FRAMEWORK_BINARY
QMAKE_EXTRA_TARGETS += linkVuoFramework


# Copy Clang and ld64 binaries to the MacOS directory
CLANG_BINS_DEST_DIR = "Vuo.framework/Versions/$${QMAKE_FRAMEWORK_VERSION}/MacOS/Clang/bin"
copyClangBins.commands += rm -rf $${CLANG_BINS_DEST_DIR} &&
copyClangBins.commands += mkdir -p $${CLANG_BINS_DEST_DIR} &&
copyClangBins.commands += cp -r $${LLVM_ROOT}/bin/clang $${CLANG_BINS_DEST_DIR} &&
copyClangBins.commands += cp -r $${LLVM_ROOT}/bin/llvm-link $${CLANG_BINS_DEST_DIR} &&
copyClangBins.commands += ln -s clang $${CLANG_BINS_DEST_DIR}/clang++ &&
copyClangBins.commands += cp -r $${ROOT}/compiler/binary/ld $${CLANG_BINS_DEST_DIR}
copyClangBins.target = $${CLANG_BINS_DEST_DIR}
POST_TARGETDEPS += $${CLANG_BINS_DEST_DIR}
QMAKE_EXTRA_TARGETS += copyClangBins


# Copy Clang system headers to the MacOS directory
CLANG_SYS_HEADERS_DEST_DIR = "Vuo.framework/Versions/$${QMAKE_FRAMEWORK_VERSION}/MacOS/Clang/lib/clang/3.2/include"
copyClangSysHeaders.commands += rm -rf $${CLANG_SYS_HEADERS_DEST_DIR} &&
copyClangSysHeaders.commands += mkdir -p $${CLANG_SYS_HEADERS_DEST_DIR} &&
copyClangSysHeaders.commands += cp -r $${LLVM_ROOT}/lib/clang/3.2/include/* $${CLANG_SYS_HEADERS_DEST_DIR}
copyClangSysHeaders.target = $${CLANG_SYS_HEADERS_DEST_DIR}
POST_TARGETDEPS += $${CLANG_SYS_HEADERS_DEST_DIR}
QMAKE_EXTRA_TARGETS += copyClangSysHeaders

createCurrentLink.commands = ln -sf $$VUO_VERSION $$CURRENT_LINK
createCurrentLink.target = $$CURRENT_LINK
createCurrentLink.depends = $$VUO_FRAMEWORK_BINARY
POST_TARGETDEPS += $$CURRENT_LINK
QMAKE_EXTRA_TARGETS += createCurrentLink

createLinks.commands  =    ln -sf Versions/Current/Vuo Vuo.framework/
createLinks.commands += && ln -sf Versions/Current/Headers Vuo.framework/
createLinks.commands += && ln -sf Versions/Current/Resources Vuo.framework/
createLinks.commands += && ln -sf Versions/Current/Licenses Vuo.framework/
createLinks.commands += && ln -sf Versions/Current/Modules Vuo.framework/
createLinks.commands += && ln -sf Versions/Current/Frameworks Vuo.framework/
createLinks.commands += && ln -sf Versions/Current/MacOS Vuo.framework/
createLinks.target = Vuo.framework/Headers
POST_TARGETDEPS += Vuo.framework/Headers
QMAKE_EXTRA_TARGETS += createLinks


# Some other project files depend on Vuo.framework.
# If qmake was run before Vuo.framework was created, the other projects don't calculate dependencies properly.
# As a litmus test, we know that vuo-link should depend on Vuo.framework's Vuo.h, so re-run qmake if it doesn't.
fixOtherProjects.commands = \
	( \
		grep -q framework/Vuo.framework/Versions/$${VUO_VERSION}/Headers/Vuo.h ../compiler/vuo-link/Makefile \
		|| (cd .. && /usr/local/bin/qmake -spec macx-clang CONFIG+=x86_64 -r) \
	) \
	&& rm -Rf $$ROOT/editor/VuoEditorApp/pch \
	&& touch fixedOtherProjects
fixOtherProjects.depends = $$VUO_FRAMEWORK_BINARY
fixOtherProjects.target = fixedOtherProjects
POST_TARGETDEPS += fixedOtherProjects
QMAKE_EXTRA_TARGETS += fixOtherProjects


# Tell qmake not to mess with Info.plist
QMAKE_INFO_PLIST_OUT = /dev/null

QMAKE_CLEAN += \
	"Vuo\\.framework" \
	$$RESOURCES_SUBDIR \
	fixedOtherProjects \
	vuo-*

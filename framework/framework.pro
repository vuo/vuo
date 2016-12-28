include(masterLists.pri)

TEMPLATE = aux
CONFIG += VuoBase VuoCompiler VuoRuntime json lib_bundle
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

# Make sure there's enough room to install_name_tool after linking.
QMAKE_LFLAGS += -headerpad_max_install_names

# Make sure it can run in older OS X versions.
QMAKE_LFLAGS += -mmacosx-version-min=$$QMAKE_MACOSX_DEPLOYMENT_TARGET

# Add the third-party libraries that Vuo nodes/types depend on
MODULE_LIBRARY_OBJECTS += \
	$$ROOT/runtime/*\\.bc \
	$$ROOT/base/VuoCompositionStub.dylib \
	$$ROOT/compiler/binary/crt1.o \
	$$ZMQ_ROOT/lib/libzmq.a \
	$$JSONC_ROOT/lib/libjson-c.a \
	$$MUPARSER_ROOT/lib/libmuparser.a \
	$$FREEIMAGE_ROOT/lib/libfreeimage.a \
	$$OPENSSL_ROOT/lib/libssl.a \
	$$OPENSSL_ROOT/lib/libcrypto.a \
	$$CURL_ROOT/lib/libcurl.a \
	$$RTMIDI_ROOT/lib/librtmidi.a \
	$$RTAUDIO_ROOT/lib/librtaudio.a \
	$$GAMMA_ROOT/lib/libGamma.a \
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
	$$ZXING_ROOT/lib/libzxing.a \
	$$LIBXML2_ROOT/lib/libxml2.a \
	$$ASSIMP_ROOT/lib/libassimp.a
	MODULE_LIBRARY_OBJECTS += $$ROOT/node/vuo.leap/Leap/libLeap.dylib


# Create and populate Headers directory
FRAMEWORK_VUO_HEADERS.files += \
	../type/coreTypes.h \
	../type/coreTypesStringify.h \
	../type/coreTypesStringify.hh
HEADERS_DEST_DIR = "Vuo.framework/Versions/$${QMAKE_FRAMEWORK_VERSION}/Headers"
createHeadersDir.commands += rm -rf $${HEADERS_DEST_DIR} &&
createHeadersDir.commands += mkdir -p $${HEADERS_DEST_DIR} &&
createHeadersDir.commands += ln -s . $${HEADERS_DEST_DIR}/Vuo &&
createHeadersDir.commands += cp $$FRAMEWORK_VUO_HEADERS.files $${HEADERS_DEST_DIR} &&
createHeadersDir.commands += cp Vuo.h $${HEADERS_DEST_DIR}/Vuo.h
createHeadersDir.depends += $$FRAMEWORK_VUO_HEADERS.files $$FRAMEWORK_VUO_STUB_HEADER Vuo.h
createHeadersDir.target = $${HEADERS_DEST_DIR}/Vuo.h
POST_TARGETDEPS += $${HEADERS_DEST_DIR}/Vuo.h
QMAKE_EXTRA_TARGETS += createHeadersDir


# Create Resources directory; populate it with Info.plist and drivers
RESOURCES_DEST_DIR = "Vuo.framework/Versions/$${QMAKE_FRAMEWORK_VERSION}/Resources"
createResourcesDir.commands += rm -rf $${RESOURCES_DEST_DIR}
createResourcesDir.commands += && mkdir -p $${RESOURCES_DEST_DIR}
createResourcesDir.commands += && cat Info.plist
createResourcesDir.commands += | sed '"s/@SHORT_VERSION@/$$VUO_VERSION \\(r`svnversion -n` on `date +%Y.%m.%d`\\)/"'
createResourcesDir.commands += > $${RESOURCES_DEST_DIR}/Info.plist &&
createResourcesDir.commands += cp drivers/* $${RESOURCES_DEST_DIR}
createResourcesDir.depends += Info.plist
createResourcesDir.depends += drivers/*
createResourcesDir.target = $${RESOURCES_DEST_DIR}
POST_TARGETDEPS += $${RESOURCES_DEST_DIR}
QMAKE_EXTRA_TARGETS += createResourcesDir


# Create and populate Licenses directory
LICENSES_DEST_DIR = "Vuo.framework/Versions/$${QMAKE_FRAMEWORK_VERSION}/Documentation/Licenses"
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
	   mkdir -p $$MODULES_DEST_DIR \
	&& ( [ -f $$MODULES_DEST_DIR/libVuoHeap.dylib ] && chmod +w $$MODULES_DEST_DIR/libVuo*.dylib ; true ) \
	&& rsync --archive --delete $$MODULE_OBJECTS $$MODULES_DEST_DIR
copyModules.depends += $$MODULE_OBJECTS
copyModules.target = $$MODULES_DEST_DIR
POST_TARGETDEPS += $$MODULES_DEST_DIR
QMAKE_EXTRA_TARGETS += copyModules

# Separately copy the libraries that change less frequently, to make the above MODULE_OBJECTS copying go faster.
copyLibraryModules.commands = \
	   mkdir -p $$MODULES_DEST_DIR \
	&& ( [ -f $$MODULES_DEST_DIR/libgvplugin_dot_layout.dylib ] && chmod +w $$MODULES_DEST_DIR/*.dylib $$MODULES_DEST_DIR/lib*.a ; true ) \
	&& cp $$MODULE_LIBRARY_OBJECTS $$MODULES_DEST_DIR \
	&& cp "$$GRAPHVIZ_ROOT/lib/graphviz/libgvplugin_dot_layout.6.dylib" "$$MODULES_DEST_DIR/libgvplugin_dot_layout.dylib" \
	&& cp "$$GRAPHVIZ_ROOT/lib/graphviz/libgvplugin_core.6.dylib"       "$$MODULES_DEST_DIR/libgvplugin_core.dylib"       \
	&& cp "$$GRAPHVIZ_ROOT/lib/libcdt.5.dylib"                          "$$MODULES_DEST_DIR/libcdt.dylib"                 \
	&& cp "$$GRAPHVIZ_ROOT/lib/libgraph.5.dylib"                        "$$MODULES_DEST_DIR/libgraph.dylib"               \
	&& cp "$$GRAPHVIZ_ROOT/lib/libgvc.6.dylib"                          "$$MODULES_DEST_DIR/libgvc.dylib"                 \
	&& cp "$$GRAPHVIZ_ROOT/lib/libpathplan.4.dylib"                     "$$MODULES_DEST_DIR/libpathplan.dylib"            \
	&& cp "$$GRAPHVIZ_ROOT/lib/libxdot.4.dylib"                         "$$MODULES_DEST_DIR/libxdot.dylib"                \
	&& install_name_tool -id "@rpath/$$MODULES_DEST_DIR/libgvplugin_core.dylib"                                          "$$MODULES_DEST_DIR/libgvplugin_core.dylib"       \
	&& install_name_tool -change "$$GRAPHVIZ_ROOT/lib/libcdt.5.dylib"      "@rpath/$$MODULES_DEST_DIR/libcdt.dylib"      "$$MODULES_DEST_DIR/libgvplugin_core.dylib"       \
	&& install_name_tool -change "$$GRAPHVIZ_ROOT/lib/libgraph.5.dylib"    "@rpath/$$MODULES_DEST_DIR/libgraph.dylib"    "$$MODULES_DEST_DIR/libgvplugin_core.dylib"       \
	&& install_name_tool -change "$$GRAPHVIZ_ROOT/lib/libgvc.6.dylib"      "@rpath/$$MODULES_DEST_DIR/libgvc.dylib"      "$$MODULES_DEST_DIR/libgvplugin_core.dylib"       \
	&& install_name_tool -change "$$GRAPHVIZ_ROOT/lib/libpathplan.4.dylib" "@rpath/$$MODULES_DEST_DIR/libpathplan.dylib" "$$MODULES_DEST_DIR/libgvplugin_core.dylib"       \
	&& install_name_tool -change "$$GRAPHVIZ_ROOT/lib/libxdot.4.dylib"     "@rpath/$$MODULES_DEST_DIR/libxdot.dylib"     "$$MODULES_DEST_DIR/libgvplugin_core.dylib"       \
	&& install_name_tool -id "@rpath/$$MODULES_DEST_DIR/libgvplugin_dot_layout.dylib"                                    "$$MODULES_DEST_DIR/libgvplugin_dot_layout.dylib" \
	&& install_name_tool -change "$$GRAPHVIZ_ROOT/lib/libcdt.5.dylib"      "@rpath/$$MODULES_DEST_DIR/libcdt.dylib"      "$$MODULES_DEST_DIR/libgvplugin_dot_layout.dylib" \
	&& install_name_tool -change "$$GRAPHVIZ_ROOT/lib/libgraph.5.dylib"    "@rpath/$$MODULES_DEST_DIR/libgraph.dylib"    "$$MODULES_DEST_DIR/libgvplugin_dot_layout.dylib" \
	&& install_name_tool -change "$$GRAPHVIZ_ROOT/lib/libgvc.6.dylib"      "@rpath/$$MODULES_DEST_DIR/libgvc.dylib"      "$$MODULES_DEST_DIR/libgvplugin_dot_layout.dylib" \
	&& install_name_tool -change "$$GRAPHVIZ_ROOT/lib/libpathplan.4.dylib" "@rpath/$$MODULES_DEST_DIR/libpathplan.dylib" "$$MODULES_DEST_DIR/libgvplugin_dot_layout.dylib" \
	&& install_name_tool -change "$$GRAPHVIZ_ROOT/lib/libxdot.4.dylib"     "@rpath/$$MODULES_DEST_DIR/libxdot.dylib"     "$$MODULES_DEST_DIR/libgvplugin_dot_layout.dylib" \
	&& install_name_tool -id "@rpath/$$MODULES_DEST_DIR/libcdt.dylib"                                                    "$$MODULES_DEST_DIR/libcdt.dylib"                 \
	&& install_name_tool -id "@rpath/$$MODULES_DEST_DIR/libgraph.dylib"                                                  "$$MODULES_DEST_DIR/libgraph.dylib"               \
	&& install_name_tool -change "$$GRAPHVIZ_ROOT/lib/libcdt.5.dylib"      "@rpath/$$MODULES_DEST_DIR/libcdt.dylib"      "$$MODULES_DEST_DIR/libgraph.dylib"               \
	&& install_name_tool -id "@rpath/$$MODULES_DEST_DIR/libgvc.dylib"                                                    "$$MODULES_DEST_DIR/libgvc.dylib"                 \
	&& install_name_tool -change "$$GRAPHVIZ_ROOT/lib/libcdt.5.dylib"      "@rpath/$$MODULES_DEST_DIR/libcdt.dylib"      "$$MODULES_DEST_DIR/libgvc.dylib"                 \
	&& install_name_tool -change "$$GRAPHVIZ_ROOT/lib/libgraph.5.dylib"    "@rpath/$$MODULES_DEST_DIR/libgraph.dylib"    "$$MODULES_DEST_DIR/libgvc.dylib"                 \
	&& install_name_tool -change "$$GRAPHVIZ_ROOT/lib/libpathplan.4.dylib" "@rpath/$$MODULES_DEST_DIR/libpathplan.dylib" "$$MODULES_DEST_DIR/libgvc.dylib"                 \
	&& install_name_tool -change "$$GRAPHVIZ_ROOT/lib/libxdot.4.dylib"     "@rpath/$$MODULES_DEST_DIR/libxdot.dylib"     "$$MODULES_DEST_DIR/libgvc.dylib"                 \
	&& install_name_tool -id "@rpath/$$MODULES_DEST_DIR/libpathplan.dylib"                                               "$$MODULES_DEST_DIR/libpathplan.dylib"            \
	&& install_name_tool -id "@rpath/$$MODULES_DEST_DIR/libxdot.dylib"                                                   "$$MODULES_DEST_DIR/libxdot.dylib"                \
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
	&& install_name_tool -id "@rpath/$$MODULES_DEST_DIR/libavutil.dylib" "$$MODULES_DEST_DIR/libavutil.dylib" \
	&& install_name_tool -id "@rpath/$$MODULES_DEST_DIR/libLeap.dylib" "$$MODULES_DEST_DIR/libLeap.dylib" \
	&& chmod -w "$$MODULES_DEST_DIR/*.dylib"
copyLibraryModules.depends += $$MODULE_LIBRARY_OBJECTS
copyLibraryModules.target = $$MODULES_DEST_DIR/libLeap.dylib
POST_TARGETDEPS += $$MODULES_DEST_DIR/libLeap.dylib
QMAKE_EXTRA_TARGETS += copyLibraryModules


# Copy VuoCompositionLoader.app to the Helpers directory.
HELPERS_DEST_DIR = "Vuo.framework/Versions/$${QMAKE_FRAMEWORK_VERSION}/Helpers"
VUOCOMPOSITIONLOADER_DEST_DIR = "$$HELPERS_DEST_DIR/VuoCompositionLoader.app/Contents/MacOS"
copyHelpers.commands = \
	   rm -rf $$HELPERS_DEST_DIR \
	&& mkdir -p $$VUOCOMPOSITIONLOADER_DEST_DIR \
	&& cp VuoCompositionLoader.plist $$VUOCOMPOSITIONLOADER_DEST_DIR/../Info.plist \
	&& cp $$ROOT/runtime/VuoCompositionLoader $$VUOCOMPOSITIONLOADER_DEST_DIR
copyHelpers.depends += $$ROOT/runtime/VuoCompositionLoader
copyHelpers.target = $$HELPERS_DEST_DIR
POST_TARGETDEPS += $$HELPERS_DEST_DIR
QMAKE_EXTRA_TARGETS += copyHelpers


FRAMEWORKS_DEST_DIR = "Vuo.framework/Versions/$${QMAKE_FRAMEWORK_VERSION}/Frameworks"


# Copy ZeroMQ headers to the Headers directory.
ZEROMQ_HEADERS_DEST_DIR = "$$HEADERS_DEST_DIR/zmq"
copyZeroMQHeaders.commands += rm -rf $${ZEROMQ_HEADERS_DEST_DIR} &&
copyZeroMQHeaders.commands += mkdir -p $${ZEROMQ_HEADERS_DEST_DIR} &&
copyZeroMQHeaders.commands += cp -a $${ZMQ_ROOT}/include/* $${ZEROMQ_HEADERS_DEST_DIR}
copyZeroMQHeaders.target = $${ZEROMQ_HEADERS_DEST_DIR}/zmq.h
POST_TARGETDEPS += $${ZEROMQ_HEADERS_DEST_DIR}/zmq.h
QMAKE_EXTRA_TARGETS += copyZeroMQHeaders


# Copy JSON-C headers to the Headers directory.
JSON_HEADERS_DEST_DIR = "$$HEADERS_DEST_DIR/json-c"
copyJsonHeaders.commands += rm -rf $${JSON_HEADERS_DEST_DIR} &&
copyJsonHeaders.commands += mkdir -p $${JSON_HEADERS_DEST_DIR} &&
copyJsonHeaders.commands += cp -r $${JSONC_ROOT}/include/json-c/* $${JSON_HEADERS_DEST_DIR}
copyJsonHeaders.target = $${JSON_HEADERS_DEST_DIR}/json.h
POST_TARGETDEPS += $${JSON_HEADERS_DEST_DIR}/json.h
QMAKE_EXTRA_TARGETS += copyJsonHeaders


# Copy system headers installed by XCode Command Line Tools to the Headers directory
MACOS_HEADERS_DEST_DIR = "$$HEADERS_DEST_DIR/macos"
copyMacOsHeaders.commands += rm -rf $${MACOS_HEADERS_DEST_DIR} &&
copyMacOsHeaders.commands += mkdir -p $${MACOS_HEADERS_DEST_DIR}/OpenGL &&
copyMacOsHeaders.commands += cp /usr/include/*.h $${MACOS_HEADERS_DEST_DIR} &&
copyMacOsHeaders.commands += ( [ -d /usr/include/_types ] && cp -r /usr/include/_types $${MACOS_HEADERS_DEST_DIR}/ ) ;
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
	&& mkdir -p $$SYPHON_LIBS_DEST_DIR \
	&& cp -a $$ROOT/node/vuo.syphon/Syphon/Syphon.framework $${FRAMEWORKS_DEST_DIR} \
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
		$${QT_ROOT}/lib/QtNetwork.framework \
		$$RESOURCES_SUBDIR \
	&& mv "$$RESOURCES_SUBDIR/QtCore.framework/Contents" "$$RESOURCES_SUBDIR/QtCore.framework/Versions/5/Resources" \
	&& ln -s Versions/Current/Resources "$$RESOURCES_SUBDIR/QtCore.framework/Resources" \
	&& mv "$$RESOURCES_SUBDIR/QtGui.framework/Contents" "$$RESOURCES_SUBDIR/QtGui.framework/Versions/5/Resources" \
	&& ln -s Versions/Current/Resources "$$RESOURCES_SUBDIR/QtGui.framework/Resources" \
	&& mv "$$RESOURCES_SUBDIR/QtWidgets.framework/Contents" "$$RESOURCES_SUBDIR/QtWidgets.framework/Versions/5/Resources" \
	&& ln -s Versions/Current/Resources "$$RESOURCES_SUBDIR/QtWidgets.framework/Resources" \
	&& mv "$$RESOURCES_SUBDIR/QtMacExtras.framework/Contents" "$$RESOURCES_SUBDIR/QtMacExtras.framework/Versions/5/Resources" \
	&& ln -s Versions/Current/Resources "$$RESOURCES_SUBDIR/QtMacExtras.framework/Resources" \
	&& mv "$$RESOURCES_SUBDIR/QtPrintSupport.framework/Contents" "$$RESOURCES_SUBDIR/QtPrintSupport.framework/Versions/5/Resources" \
	&& ln -s Versions/Current/Resources "$$RESOURCES_SUBDIR/QtPrintSupport.framework/Resources" \
	&& mv "$$RESOURCES_SUBDIR/QtOpenGL.framework/Contents" "$$RESOURCES_SUBDIR/QtOpenGL.framework/Versions/5/Resources" \
	&& ln -s Versions/Current/Resources "$$RESOURCES_SUBDIR/QtOpenGL.framework/Resources" \
	&& mv "$$RESOURCES_SUBDIR/QtXml.framework/Contents" "$$RESOURCES_SUBDIR/QtXml.framework/Versions/5/Resources" \
	&& ln -s Versions/Current/Resources "$$RESOURCES_SUBDIR/QtXml.framework/Resources" \
	&& mv "$$RESOURCES_SUBDIR/QtNetwork.framework/Contents" "$$RESOURCES_SUBDIR/QtNetwork.framework/Versions/5/Resources" \
	&& ln -s Versions/Current/Resources "$$RESOURCES_SUBDIR/QtNetwork.framework/Resources" \
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
	&& chmod +wx "$$RESOURCES_SUBDIR/QtNetwork.framework/Versions/$$QT_MAJOR_VERSION/QtNetwork" \
	&& install_name_tool -id "@rpath/QtNetwork.framework/QtNetwork" "$$RESOURCES_SUBDIR/QtNetwork.framework/QtNetwork" \
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
	-framework AppKit \
	-framework AVFoundation \
	-framework CoreFoundation \
	-framework CoreMedia \
	-framework QuartzCore \
	-framework OpenGL \
	-framework IOKit \
	-framework IOSurface \
	-F$$FRAMEWORKS_DEST_DIR \
	-Wl,-reexport_framework,llvm \
	$$MUPARSER_ROOT/lib/libmuparser.a \
	$$FREEIMAGE_ROOT/lib/libfreeimage.a \
	$$CURL_ROOT/lib/libcurl.a \
	$$ROOT/library/VuoBase64.o \
	$$ROOT/library/VuoIOReturn.o \
	$$ROOT/library/VuoImageRenderer.o \
	$$ROOT/library/VuoImageResize.o \
	$$ROOT/library/VuoImageBlend.o \
	$$ROOT/library/VuoImageBlur.o \
	$$ROOT/library/VuoImageGet.o \
	$$ROOT/library/VuoImageMapColors.o \
	$$ROOT/library/VuoImageText.o \
	$$ROOT/library/VuoMathExpressionParser.o \
	$$ROOT/library/VuoOsStatus.o \
	$$ROOT/library/VuoScreenCommon.o \
	$$ROOT/library/VuoUrlFetch.o \
	$$ROOT/library/VuoUrlParser.o \
	$$ROOT/library/libVuoGlContext.dylib \
	$$ROOT/library/libVuoGlPool.dylib \
	$$ROOT/library/libVuoHeap.dylib \
	$$ROOT/node/vuo.noise/VuoGradientNoiseCommon.bc \
	$$ROOT/node/vuo.hid/VuoHidDevices.o \
	$$ROOT/node/vuo.hid/VuoHidIo.o \
	$$ROOT/node/vuo.hid/VuoHidUsage.o \
	$$ROOT/node/vuo.hid/VuoUsbVendor.o \
	$$ROOT/node/vuo.serial/VuoSerialDevices.o \
	$$ROOT/node/vuo.serial/VuoSerialIO.o


# Turn Clang headers into a Framework.
CLANG_FRAMEWORK_DEST_DIR = "$$FRAMEWORKS_DEST_DIR/clang.framework"
CLANG_DYLIB_DEST = "$$CLANG_FRAMEWORK_DEST_DIR/Versions/A/clang"
CLANG_HEADERS_DIR = "$$CLANG_FRAMEWORK_DEST_DIR/Versions/A/Headers"
CLANG_RESOURCES_DIR = "$$CLANG_FRAMEWORK_DEST_DIR/Versions/A/Resources"
makeClangFramework.commands = \
	   mkdir -p "$$CLANG_HEADERS_DIR" \
	&& touch "$$CLANG_DYLIB_DEST" \
	&& cp -r $$LLVM_ROOT/include/clang/* "$$CLANG_HEADERS_DIR" \
	&& mkdir "$$CLANG_RESOURCES_DIR" \
	&& cp Info-clang.plist "$$CLANG_RESOURCES_DIR/Info.plist" \
	&& ln -s A "$$CLANG_FRAMEWORK_DEST_DIR/Versions/Current" \
	&& ln -s Versions/Current/clang "$$CLANG_FRAMEWORK_DEST_DIR" \
	&& ln -s Versions/Current/Headers "$$CLANG_FRAMEWORK_DEST_DIR" \
	&& ln -s Versions/Current/Resources "$$CLANG_FRAMEWORK_DEST_DIR"
makeClangFramework.target = $${CLANG_FRAMEWORK_DEST_DIR}
POST_TARGETDEPS += $${CLANG_FRAMEWORK_DEST_DIR}
QMAKE_EXTRA_TARGETS += makeClangFramework


# Turn LLVM dylib, headers, and command-line binaries into a Framework.
LLVM_FRAMEWORK_DEST_DIR = "$$FRAMEWORKS_DEST_DIR/llvm.framework"
LLVM_DYLIB_DEST = "$$LLVM_FRAMEWORK_DEST_DIR/Versions/A/llvm"
LLVM_FRAMEWORK_HEADERS_DIR = "$$LLVM_FRAMEWORK_DEST_DIR/Versions/A/Headers"
LLVM_FRAMEWORK_HELPERS_DIR = "$$LLVM_FRAMEWORK_DEST_DIR/Versions/A/Helpers"
CLANG_SYS_HEADERS_DIR = "$$LLVM_FRAMEWORK_DEST_DIR/Versions/A/lib/clang/3.2/include"
makeLLVMFramework.commands = \
	   mkdir -p "$$LLVM_FRAMEWORK_DEST_DIR/Versions/A" \
	&& cp "$$LLVM_ROOT/lib/$$LLVM_DYLIB" "$$LLVM_DYLIB_DEST" \
	&& install_name_tool -id "@loader_path/Frameworks/llvm.framework/llvm" "$$LLVM_DYLIB_DEST" \
	&& mkdir "$$LLVM_FRAMEWORK_HEADERS_DIR" \
	&& cp -r $$LLVM_ROOT/include/llvm/* "$$LLVM_FRAMEWORK_HEADERS_DIR" \
	&& mkdir "$$LLVM_FRAMEWORK_HELPERS_DIR" \
	&& cp -r "$$LLVM_ROOT/bin/clang" "$${ROOT}/compiler/binary/ld" "$$LLVM_FRAMEWORK_HELPERS_DIR" \
	&& mkdir -p $$CLANG_SYS_HEADERS_DIR \
	&& cp -r $$LLVM_ROOT/lib/clang/3.2/include/* $$CLANG_SYS_HEADERS_DIR \
	&& install_name_tool -change "@executable_path/../lib/$$LLVM_DYLIB" "@executable_path/../llvm" "$$LLVM_FRAMEWORK_HELPERS_DIR/clang" \
	&& cp -r "$$LLVM_ROOT/bin/llvm-link" "$$LLVM_FRAMEWORK_HELPERS_DIR" \
	&& install_name_tool -change "@executable_path/../lib/$$LLVM_DYLIB" "@executable_path/../llvm" "$$LLVM_FRAMEWORK_HELPERS_DIR/llvm-link" \
	&& ln -s clang "$$LLVM_FRAMEWORK_HELPERS_DIR/clang++" \
	&& mkdir "$$LLVM_FRAMEWORK_DEST_DIR/Versions/A/Resources" \
	&& cp Info-llvm.plist "$$LLVM_FRAMEWORK_DEST_DIR/Versions/A/Resources/Info.plist" \
	&& ln -s A "$$LLVM_FRAMEWORK_DEST_DIR/Versions/Current" \
	&& ln -s Versions/Current/llvm "$$LLVM_FRAMEWORK_DEST_DIR" \
	&& ln -s Versions/Current/Headers "$$LLVM_FRAMEWORK_DEST_DIR" \
	&& ln -s Versions/Current/Helpers "$$LLVM_FRAMEWORK_DEST_DIR" \
	&& ln -s Versions/Current/Resources "$$LLVM_FRAMEWORK_DEST_DIR"
makeLLVMFramework.target = $$LLVM_DYLIB_DEST
POST_TARGETDEPS += $$LLVM_DYLIB_DEST
QMAKE_EXTRA_TARGETS += makeLLVMFramework


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
	&& install_name_tool -change "$$GRAPHVIZ_ROOT/lib/graphviz/libgvplugin_core.6.dylib"       "@rpath/$$MODULES_DEST_DIR/libgvplugin_core.dylib"       "$$VUO_FRAMEWORK_BINARY" \
	&& install_name_tool -change "$$GRAPHVIZ_ROOT/lib/graphviz/libgvplugin_dot_layout.6.dylib" "@rpath/$$MODULES_DEST_DIR/libgvplugin_dot_layout.dylib" "$$VUO_FRAMEWORK_BINARY" \
	&& install_name_tool -change "$$GRAPHVIZ_ROOT/lib/libcdt.5.dylib"                          "@rpath/$$MODULES_DEST_DIR/libcdt.dylib"                 "$$VUO_FRAMEWORK_BINARY" \
	&& install_name_tool -change "$$GRAPHVIZ_ROOT/lib/libgraph.5.dylib"                        "@rpath/$$MODULES_DEST_DIR/libgraph.dylib"               "$$VUO_FRAMEWORK_BINARY" \
	&& install_name_tool -change "$$GRAPHVIZ_ROOT/lib/libgvc.6.dylib"                          "@rpath/$$MODULES_DEST_DIR/libgvc.dylib"                 "$$VUO_FRAMEWORK_BINARY" \
	&& install_name_tool -change "$$GRAPHVIZ_ROOT/lib/libpathplan.4.dylib"                     "@rpath/$$MODULES_DEST_DIR/libpathplan.dylib"            "$$VUO_FRAMEWORK_BINARY" \
	&& install_name_tool -change "$$GRAPHVIZ_ROOT/lib/libxdot.4.dylib"                         "@rpath/$$MODULES_DEST_DIR/libxdot.dylib"                "$$VUO_FRAMEWORK_BINARY" \
	&& install_name_tool -change "@executable_path/../lib/$$LLVM_DYLIB"                        "@rpath/$$LLVM_DYLIB_DEST"                               "$$VUO_FRAMEWORK_BINARY" \
	&& rm -f $$CURRENT_LINK
coverage {
	linkVuoFramework.commands += && install_name_tool -change @executable_path/../lib/libprofile_rt.dylib $$LLVM_ROOT/lib/libprofile_rt.dylib "$$VUO_FRAMEWORK_BINARY"
}

linkVuoFramework.target = $$VUO_FRAMEWORK_BINARY
linkVuoFramework.depends = \
	$$LLVM_DYLIB_DEST \
	../library/libVuoGlContext.dylib \
	../library/libVuoGlPool.dylib \
	../library/libVuoHeap.dylib \
	../base/libVuoBase.a \
	../compiler/libVuoCompiler.a \
	../type/libVuoType.a \
	../type/list/libVuoTypeList.a \
	../library/VuoImageRenderer.o
POST_TARGETDEPS += $$VUO_FRAMEWORK_BINARY
QMAKE_EXTRA_TARGETS += linkVuoFramework


createCurrentLink.commands = \
	( [ -L $$CURRENT_LINK ] || ln -sf $$VUO_VERSION $$CURRENT_LINK ) \
	&& touch $$CURRENT_LINK
createCurrentLink.target = $$CURRENT_LINK
createCurrentLink.depends = $$VUO_FRAMEWORK_BINARY
POST_TARGETDEPS += $$CURRENT_LINK
QMAKE_EXTRA_TARGETS += createCurrentLink

createLinks.commands  =    ln -sf Versions/Current/Vuo Vuo.framework/
createLinks.commands += && ln -sf Versions/Current/Headers Vuo.framework/
createLinks.commands += && ln -sf Versions/Current/Helpers Vuo.framework/
createLinks.commands += && ln -sf Versions/Current/Resources Vuo.framework/
createLinks.commands += && ln -sf Versions/Current/Documentation Vuo.framework/
createLinks.commands += && ln -sf Versions/Current/Modules Vuo.framework/
createLinks.commands += && ln -sf Versions/Current/Frameworks Vuo.framework/
createLinks.target = Vuo.framework/Headers
POST_TARGETDEPS += Vuo.framework/Headers
QMAKE_EXTRA_TARGETS += createLinks


# Some other project files depend on Vuo.framework.
# If qmake was run before Vuo.framework was created, the other projects don't calculate dependencies properly.
# As a litmus test, we know that vuo-link should depend on Vuo.framework's Vuo.h, so re-run qmake if it doesn't.
fixOtherProjects.commands = \
	( \
		grep -q framework/Vuo.framework/Versions/$${VUO_VERSION}/Headers/Vuo.h ../compiler/vuo-link/Makefile \
		|| (cd .. && /usr/local/bin/qmake -spec macx-clang CONFIG+="'$$VUO_QMAKE_CONFIG'" -r) \
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
	Vuo.h \
	"Vuo\\.framework" \
	$$RESOURCES_SUBDIR \
	fixedOtherProjects \
	vuo-compile \
	vuo-debug \
	vuo-export \
	vuo-link \
	vuo-render


# Make available in Qt Creator.
OTHER_FILES = \
	Vuo.stub.h

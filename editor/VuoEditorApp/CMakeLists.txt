add_executable(VuoEditorApp MACOSX_BUNDLE
	VuoEditorApp.qrc
	main.cc
)

# These values are substituted into Info.plist.
set(BUNDLE_SHORT_VERSION_STRING "${VUO_VERSION_AND_BUILD}")
set(BUNDLE_VERSION              "${Vuo_VERSION}")

set_target_properties(VuoEditorApp PROPERTIES
	AUTORCC ON
	OUTPUT_NAME "Vuo"
	MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist

	# Don't automatically add `rpath` search paths to this binary.
	BUILD_WITH_INSTALL_RPATH TRUE
	INSTALL_RPATH "@loader_path/../Frameworks;@loader_path/../Frameworks/Vuo.framework/Versions/Current/Modules"
)
target_link_libraries(VuoEditorApp
	VuoFrameworkDylib
	VuoRenderer
	VuoEditor
	VuoWidget
)
if (VuoPro)
	include(pro/pro.cmake)
endif()


VuoBundleFiles(VuoEditorApp "Resources"
	../GlslIsfQuickReference.md
	${PROJECT_SOURCE_DIR}/renderer/font/PTS55F.ttf
	${PROJECT_SOURCE_DIR}/renderer/font/PTS75F.ttf
	Icons/align-center.png
	Icons/align-left.png
	Icons/align-right.png
	Icons/run.pdf
	Icons/search-clear-pressed.png
	Icons/search-clear.png
	Icons/search-loupe.png
	Icons/share.pdf
	Icons/showEvents.pdf
	Icons/stop.pdf
	Icons/vuo-composition.icns
	Icons/vuo-node.icns
	Icons/vuo.icns
	Icons/zoom-actual.pdf
	Icons/zoom-fit.pdf
	Icons/zoom-in.pdf
	Icons/zoom-out.pdf
	Icons/fontawesome/copy.pdf
	Icons/fontawesome/download.pdf
	Icons/fontawesome/external-link-square.pdf
	Icons/fontawesome/times-circle.pdf
	qt.conf
)


get_property(VuoInputEditors  GLOBAL PROPERTY VuoInputEditors)
get_property(inputEditors     GLOBAL PROPERTY VuoInputEditors)
list(TRANSFORM inputEditors PREPEND ${PROJECT_BINARY_DIR}/lib/lib)
list(TRANSFORM inputEditors APPEND  .dylib)
set(inputEditorsDestDir "${CMAKE_BINARY_DIR}/bin/Vuo.app/Contents/Resources/InputEditors")
get_property(inputEditorsDest GLOBAL PROPERTY VuoInputEditors)
list(TRANSFORM inputEditorsDest PREPEND ${inputEditorsDestDir}/lib)
list(TRANSFORM inputEditorsDest APPEND  .dylib)
add_custom_command(
	DEPENDS
		${VuoInputEditors} ${inputEditors}
	COMMENT "Copying input editors into Vuo.app"
	COMMAND mkdir -p ${inputEditorsDestDir}
	COMMAND rsync --update
		${inputEditors}
		${inputEditorsDestDir}
	COMMAND ${codesignAdHoc} ${inputEditorsDest} ${codesignAdHocIgnoreReplace}
	# Update timestamps, to help `make` believe this target was successfully rebuilt.
	COMMAND touch ${inputEditorsDest}
	OUTPUT ${inputEditorsDest}
)
add_custom_target(VuoEditorAppInputEditors DEPENDS ${inputEditorsDest})
add_dependencies(VuoEditorApp VuoEditorAppInputEditors)


set(frameworksDestDir "${CMAKE_BINARY_DIR}/bin/Vuo.app/Contents/Frameworks")


add_custom_command(
	COMMENT "Copying Qt frameworks and plugins into Vuo.app"
	COMMAND mkdir -p ${frameworksDestDir}
	COMMAND rsync --recursive --links --perms
		--exclude 'Qt*.prl'
		--exclude 'Qt*_debug'
		--exclude Headers
		${CONAN_LIB_DIRS_QT}/QtCore.framework
		${CONAN_LIB_DIRS_QT}/QtGui.framework
		${CONAN_LIB_DIRS_QT}/QtMacExtras.framework
		${CONAN_LIB_DIRS_QT}/QtMultimedia.framework
		${CONAN_LIB_DIRS_QT}/QtMultimediaQuick.framework
		${CONAN_LIB_DIRS_QT}/QtMultimediaWidgets.framework
		${CONAN_LIB_DIRS_QT}/QtNetwork.framework
		${CONAN_LIB_DIRS_QT}/QtOpenGL.framework
		${CONAN_LIB_DIRS_QT}/QtPrintSupport.framework
		${CONAN_LIB_DIRS_QT}/QtQml.framework
		${CONAN_LIB_DIRS_QT}/QtQuick.framework
		${CONAN_LIB_DIRS_QT}/QtQuickWidgets.framework
		${CONAN_LIB_DIRS_QT}/QtSvg.framework
		${CONAN_LIB_DIRS_QT}/QtWidgets.framework
		${CONAN_LIB_DIRS_QT}/QtXml.framework
		${frameworksDestDir}
	COMMAND ${codesignAdHoc} ${frameworksDestDir}/Qt*.framework ${codesignAdHocIgnoreReplace}
	COMMAND rsync --recursive --links --perms --delete
		--include libqcocoa.dylib
		--include libqsvgicon.dylib
		--include libqicns.dylib
		--include libqjpeg.dylib
		--include libqgif.dylib
		--include libqsvg.dylib
		--include libqavfmediaplayer.dylib
		--include libqmacstyle.dylib
		--exclude audio
		--exclude bearer
		--exclude designer
		--exclude generic
		--exclude playlistformats
		--exclude printsupport
		--exclude sqldrivers
		--exclude '*.dSYM'
		--exclude '*.dylib'
		${CONAN_QT_ROOT}/plugins/
		${CMAKE_BINARY_DIR}/bin/Vuo.app/Contents/Frameworks/QtPlugins
	COMMAND rsync --recursive --links --perms --delete
		--exclude '*.dSYM'
		--exclude '*.qmltypes'
		--exclude Qt/labs/folderlistmodel
		--exclude Qt/labs/qmlmodels
		--exclude Qt/labs/sharedimage
		--exclude Qt/labs/wavefrontmesh
		--exclude QtAudioEngine
		--exclude QtQml
		--exclude QtQuick/Dialogs
		--exclude QtQuick/Extras
		--exclude QtQuick/LocalStorage
		--exclude QtQuick/Particles.2
		--exclude QtQuick/PrivateWidgets
		--exclude QtTest
		${CONAN_QT_ROOT}/qml
		${CMAKE_BINARY_DIR}/bin/Vuo.app/Contents
	OUTPUT ${frameworksDestDir}/QtCore.framework/QtCore
)
add_custom_target(VuoEditorAppQt DEPENDS ${frameworksDestDir}/QtCore.framework/QtCore)
add_dependencies(VuoEditorApp VuoEditorAppQt)


get_target_property(VuoFrameworkGeneratedFiles       VuoFramework       VuoGeneratedFiles)
get_target_property(VuoRunnerGeneratedFiles          VuoRunner          VuoGeneratedFiles)
get_target_property(VuoModuleCompilersGeneratedFiles VuoModuleCompilers VuoGeneratedFiles)
set(frameworkDest ${frameworksDestDir}/Vuo.framework/Versions/Current/Vuo)
add_custom_command(
	DEPENDS
		VuoFramework       ${VuoFrameworkGeneratedFiles}
		# Work around apparent CMake bug where it generates nonexistent Ninja targets for macOS frameworks.
		${VuoRunnerGeneratedFiles}  # VuoRunner
		VuoModuleCompilers ${VuoModuleCompilersGeneratedFiles}
		${VuoStubsTarget}  ${VuoStubsGeneratedFiles}
	COMMENT "Copying Vuo.framework and VuoRunner.framework into Vuo.app"
	COMMAND mkdir -p ${frameworksDestDir}
	# Use `--extended-attributes` since `codesign` stores signatures for `.bc` and `.o` files
	# in macOS extended attributes (a.k.a. xattr, resource forks),
	# and those code signatures are required for Notarization.
	COMMAND rsync --recursive --links --update --delete --extended-attributes
		${PROJECT_BINARY_DIR}/lib/Vuo.framework
		${PROJECT_BINARY_DIR}/lib/VuoRunner.framework
		${frameworksDestDir}
	COMMAND ${codesignAdHoc} ${frameworksDestDir}/Vuo.framework ${codesignAdHocIgnoreReplace}

	# Update timestamps, to help `make` believe this target was successfully rebuilt.
	COMMAND touch ${frameworkDest}

	OUTPUT ${frameworkDest}
)
add_custom_target(VuoEditorAppFramework DEPENDS ${frameworkDest})
add_dependencies(VuoEditorApp VuoEditorAppFramework)


add_custom_command(
	DEPENDS
		VuoWidget ${PROJECT_BINARY_DIR}/lib/libVuoWidget.dylib
	COMMENT "Copying libraries into Vuo.app"
	COMMAND mkdir -p ${frameworksDestDir}
	COMMAND rsync --perms
		${PROJECT_BINARY_DIR}/lib/libVuoWidget.dylib
		${frameworksDestDir}
	COMMAND ${codesignAdHoc} ${frameworksDestDir}/libVuoWidget.dylib ${codesignAdHocIgnoreReplace}
	OUTPUT ${frameworksDestDir}/libVuoWidget.dylib
)
add_custom_target(VuoEditorAppLibraries DEPENDS ${frameworksDestDir}/libVuoWidget.dylib)
add_dependencies(VuoEditorApp VuoEditorAppLibraries)


set(contributorsDest "${CMAKE_BINARY_DIR}/bin/Vuo.app/Contents/Resources/CONTRIBUTORS.md")
add_custom_command(
	COMMENT "Fetching CONTRIBUTORS.md"
	COMMAND /usr/bin/curl --silent --show-error https://vuo.org/contributors/markdown --output ${contributorsDest}
	COMMAND grep --quiet 'Thanks to our contributors' ${contributorsDest}
	OUTPUT ${contributorsDest}
)
add_custom_target(VuoEditorAppContributors DEPENDS ${contributorsDest})
add_dependencies(VuoEditorApp VuoEditorAppContributors)


# Code-sign the whole mess after all the frameworks and helpers have been copied into Vuo.app.
add_custom_command(TARGET VuoEditorApp POST_BUILD
	COMMAND ${codesignAdHoc} ${CMAKE_BINARY_DIR}/bin/Vuo.app ${codesignAdHocIgnoreReplace}
)

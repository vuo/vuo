add_executable(vuo-render
	vuo-render.cc
)
set_target_properties(vuo-render PROPERTIES
	# Don't automatically add `rpath` search paths to this dylib.
	BUILD_WITH_INSTALL_RPATH TRUE
)
set_property(TARGET vuo-render PROPERTY INSTALL_RPATH
	# Qt frameworks
	@loader_path/../resources
	# In source tree
	@loader_path/../lib
	@loader_path/../lib/Vuo.framework/Versions/${VuoFrameworkVersion}/Modules
	# In distributable SDK
	@loader_path/..
	@loader_path/../Vuo.framework/Versions/${VuoFrameworkVersion}/Modules
)
target_link_libraries(vuo-render
	Qt5::PrintSupport
	Qt5::Svg
	VuoFrameworkDylib
	VuoRenderer
)
VuoAddInfoPListSection(vuo-render vuo-render-Info.plist)
VuoPCH(vuo-render Qt)

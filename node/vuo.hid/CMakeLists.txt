VuoCompileLibraries(
	VuoHidDevices.cc
	VuoHidIo.cc
	VuoHidUsage.c
	VuoUsbVendor.c
)
target_sources(vuo.hid.libraries PRIVATE
	VuoHid.h
	VuoUsbVendor.h
)
target_link_libraries(vuo.hid.libraries
	PUBLIC
	"-framework IOKit"
	VuoApp
	${PROJECT_BINARY_DIR}/library/VuoIoReturn.o
)

VuoCompileTypes(
	VuoHidControl.c
	VuoHidDevice.c
)
target_sources(vuo.hid.types PRIVATE
	VuoHidControl.h
	VuoHidDevice.h
)
target_link_libraries(vuo.hid.types
	PUBLIC
	vuo.hid.libraries
)

VuoCompileCompoundTypes(
	VuoList_VuoHidControl.cc
	VuoList_VuoHidDevice.cc
)

VuoCompileNodes(
	vuo.hid.filter.control.c
	vuo.hid.filter.control2.c
	vuo.hid.find.name.c
	vuo.hid.find.name2.c
	vuo.hid.get.c
	vuo.hid.get.control.c
	vuo.hid.listDevices.c
	vuo.hid.make.name.c
	vuo.hid.receive.c
	vuo.hid.scale.control.c
	vuo.hid.scale.control2.c
)

VuoNodeSet()

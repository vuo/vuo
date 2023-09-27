VuoCompileLibraries(
	VuoOsc.cc
	VuoOscDevices.cc
)
target_sources(vuo.osc.libraries PRIVATE
	VuoOsc.h
)
target_link_libraries(vuo.osc.libraries
	PRIVATE
		CONAN_PKG::oscpack
)

VuoCompileTypes(
	VuoOscInputDevice.c
	VuoOscOutputDevice.c
	VuoOscMessage.c
	VuoOscType.c
)
target_sources(vuo.osc.types PRIVATE
	VuoOscInputDevice.h
	VuoOscMessage.h
	VuoOscOutputDevice.h
	VuoOscType.h
)

VuoCompileCompoundTypes(
	VuoList_VuoOscMessage.cc
	VuoList_VuoOscInputDevice.cc
	VuoList_VuoOscOutputDevice.cc
	VuoList_VuoOscType.cc
)

VuoCompileGenericNodes(
	vuo.osc.message.get.1.c
	vuo.osc.message.get.2.c
	vuo.osc.message.get.3.c
	vuo.osc.message.get.4.c
	vuo.osc.message.get.11.c
	vuo.osc.message.get.list.c
	vuo.osc.message.make.1.c
	vuo.osc.message.make.2.c
	vuo.osc.message.make.3.c
	vuo.osc.message.make.4.c
	vuo.osc.message.make.11.c
	vuo.osc.message.make.list.c
)

VuoCompileNodes(
	vuo.osc.filter.address.c
	vuo.osc.find.input.name.c
	vuo.osc.find.input.name2.c
	vuo.osc.find.output.name.c
	vuo.osc.find.output.name2.c
	vuo.osc.get.input.c
	vuo.osc.get.output.c
	vuo.osc.listDevices.c
	vuo.osc.make.input.c
	vuo.osc.make.output.c
	vuo.osc.make.output.ip.c
	vuo.osc.receive.c
	vuo.osc.receive2.c
	vuo.osc.send.c
	vuo.osc.skeleton.basic.c
)

VuoNodeSet()

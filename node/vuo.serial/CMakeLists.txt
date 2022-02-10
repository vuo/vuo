VuoCompileLibraries(
	VuoSerialDevices.cc
	VuoSerialIO.cc
)
target_sources(vuo.serial.libraries PRIVATE
	VuoSerialDevice.h
)
target_link_libraries(vuo.serial.libraries
	PUBLIC
	vuo.data.types
)

VuoCompileTypes(
	VuoBaudRate.c
	VuoParity.c
	VuoSerialDevice.c
)
target_sources(vuo.serial.types PRIVATE
	VuoBaudRate.h
	VuoParity.h
	VuoSerial.h
)
target_link_libraries(vuo.serial.types
	PUBLIC
	vuo.data.types
	vuo.serial.libraries
)

VuoCompileNodes(
	vuo.serial.configure.c
	vuo.serial.find.name.c
	vuo.serial.find.name2.c
	vuo.serial.get.c
	vuo.serial.listDevices.c
	vuo.serial.make.name.c
	vuo.serial.make.url.c
	vuo.serial.receive.c
	vuo.serial.send.c
)
target_include_directories(vuo.serial.nodes
	PRIVATE
	../vuo.data
)

VuoNodeSet()

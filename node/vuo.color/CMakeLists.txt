VuoCompileTypes(
	VuoDmxColorMap.c
)
target_sources(vuo.color.types PRIVATE
	VuoDmxColorMap.h
)

VuoCompileLibraries(
	VuoColorspace.m
)
target_sources(vuo.color.libraries PRIVATE
	VuoColorspace.h
)
target_include_directories(vuo.color.libraries
	PRIVATE
		../vuo.data
)

VuoCompileGenericNodes(
	vuo.color.get.icc.c
	vuo.color.make.icc.c
)
target_include_directories(vuo.color.nodes.generic
	PRIVATE
		../vuo.data
)

VuoCompileNodes(
	vuo.color.average.c
	vuo.color.blend.c
	vuo.color.dmx.list.c
	vuo.color.get.cmyk.c
	vuo.color.get.hex.c
	vuo.color.get.hsl.c
	vuo.color.get.rgb.c
	vuo.color.list.dmx.c
	vuo.color.make.hex.c
	vuo.color.make.hsl.c
	vuo.color.make.cmyk.c
	vuo.color.make.rgb.c
)
target_include_directories(vuo.color.nodes
	PRIVATE
		../vuo.data
)

VuoNodeSet()

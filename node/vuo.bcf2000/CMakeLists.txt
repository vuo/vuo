VuoCompileLibraries(
	VuoScribbleStrip.c
)
target_sources(vuo.bcf2000.libraries PRIVATE
	VuoScribbleStrip.h
)

VuoCompileTypes(
	VuoRealRegulation.c
)
target_sources(vuo.bcf2000.types PRIVATE
	VuoRealRegulation.h
)

VuoCompileNodes(
	vuo.bcf2000.buttons1.c
	vuo.bcf2000.buttons2.c
	vuo.bcf2000.faders.c
	vuo.bcf2000.foot.c
	vuo.bcf2000.knobButtons.c
	vuo.bcf2000.knobs.c
	vuo.bcf2000.rightButtons.c
)
target_include_directories(vuo.bcf2000.nodes
	PRIVATE
	../vuo.midi
)

VuoNodeSet()

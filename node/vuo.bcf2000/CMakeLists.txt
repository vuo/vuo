VuoCompileLibraries(
	VuoScribbleStrip.c
)

VuoCompileTypes(
	VuoRealRegulation.c
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

VuoCompileLibraries(
	VuoGradientNoiseCommon.c
)
target_sources(vuo.noise.libraries PRIVATE
	VuoGradientNoiseCommon.h
)

VuoCompileTypes(
	VuoGradientNoise.c
	VuoNoise.c
)
target_sources(vuo.noise.types PRIVATE
	VuoGradientNoise.h
	VuoNoise.h
)

VuoCompileCompoundTypes(
	VuoList_VuoGradientNoise.cc
	VuoList_VuoNoise.cc
)

VuoCompileGenericNodes(
	vuo.noise.gradient.c
	vuo.noise.random.c
	vuo.noise.random.seed.c
	vuo.noise.random.list.c
	vuo.noise.random.list.seed.c
)

VuoNodeSet()

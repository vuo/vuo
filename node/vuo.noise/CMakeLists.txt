VuoCompileLibraries(
	VuoGradientNoiseCommon.c
)

VuoCompileTypes(
	VuoGradientNoise.c
	VuoNoise.c
)

VuoCompileGenericNodes(
	vuo.noise.gradient.c
	vuo.noise.random.c
	vuo.noise.random.seed.c
	vuo.noise.random.list.c
	vuo.noise.random.list.seed.c
)

VuoNodeSet()

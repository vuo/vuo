VuoCompileTypes(
	VuoVertexAttribute.c
)
target_sources(vuo.shader.types PRIVATE
	VuoVertexAttribute.h
)

VuoCompileNodes(
	vuo.shader.make.color.c
	vuo.shader.make.color.unlit.c
	vuo.shader.make.edge.color.c
	vuo.shader.make.frost.c
	vuo.shader.make.image.c
	vuo.shader.make.image.details.c
	vuo.shader.make.image.unlit.c
	vuo.shader.make.normal.c
	vuo.shader.make.oval.c
	vuo.shader.make.wireframe.c
)

VuoNodeSet()

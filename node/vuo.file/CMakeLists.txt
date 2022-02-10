VuoCompileTypes(
	VuoFileType.c
)
target_sources(vuo.file.types PRIVATE
	VuoFileType.h
)

VuoCompileNodes(
	vuo.file.create.folder.c
	vuo.file.exists.c
	vuo.file.list.c
)

VuoNodeSet()

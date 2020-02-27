VuoCompileTypes(
	VuoData.c
)
target_link_libraries(vuo.data.types
	PUBLIC
		${PROJECT_BINARY_DIR}/library/VuoBase64.o
)

VuoCompileGenericNodes(
	vuo.data.areEqual.c
	vuo.data.fetch.c
	vuo.data.hold.c
	vuo.data.hold2.c
	vuo.data.hold.list.c
	vuo.data.hold.list2.c
	vuo.data.isGreaterThan.c
	vuo.data.isLessThan.c
	vuo.data.populated.c
	vuo.data.record.c
	vuo.data.save.c
	vuo.data.share.c
	vuo.data.share.list.c
	vuo.data.summarize.c
)

VuoNodeSet()

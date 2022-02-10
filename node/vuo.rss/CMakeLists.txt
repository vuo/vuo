VuoCompileTypes(
	VuoRssItem.c
)
target_sources(vuo.rss.types PRIVATE
	VuoRssItem.h
)
target_include_directories(vuo.rss.types
	PRIVATE
	../vuo.time
)

VuoCompileNodes(
	vuo.rss.fetch.c
	vuo.rss.find.author.c
	vuo.rss.find.author2.c
	vuo.rss.find.description.c
	vuo.rss.find.description2.c
	vuo.rss.find.title.c
	vuo.rss.find.title2.c
	vuo.rss.get.c
)
target_include_directories(vuo.rss.nodes
	PRIVATE
	../vuo.time
)
target_link_libraries(vuo.rss.nodes
	PRIVATE
		CONAN_PKG::libxml2
)

VuoNodeSet()

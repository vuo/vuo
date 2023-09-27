VuoCompileTypes(
	VuoDurationType.c
	VuoRelativeTime.c
	VuoTime.c
	VuoTimeFormat.c
	VuoTimeUnit.c
	VuoWeekday.c
)
target_sources(vuo.time.types PRIVATE
	VuoDurationType.h
	VuoRelativeTime.h
	VuoTime.h
	VuoTimeFormat.h
	VuoTimeUnit.h
	VuoWeekday.h
)
target_include_directories(vuo.time.types
	PRIVATE
	../vuo.math
)

VuoCompileCompoundTypes(
	# List types needed by targets compiled with Clang rather than the Vuo compiler
	VuoList_VuoTime.cc
	VuoList_VuoTimeUnit.cc

	# Additional list types referenced by headers in this node set
	VuoList_VuoDurationType.cc
	VuoList_VuoTimeFormat.cc
	VuoList_VuoWeekday.cc
)

VuoCompileNodes(
	vuo.time.after.timeOfDay.c
	vuo.time.allowPeriodic.c
	vuo.time.before.timeOfDay.c
	vuo.time.bpm.seconds.c
	vuo.time.changeSpeed.c
	vuo.time.elapsed.c
	vuo.time.equal.dateTime.c
	vuo.time.equal.timeOfDay.c
	vuo.time.fetch.c
	vuo.time.firePeriodically.c
	vuo.time.firePeriodically2.c
	vuo.time.format.c
	vuo.time.get.c
	vuo.time.make.c
	vuo.time.measureTime.c
	vuo.time.offset.c
	vuo.time.relative.get.c
	vuo.time.relative.make.c
	vuo.time.round.c
	vuo.time.schedule.c
	vuo.time.seconds.bpm.c
)
target_include_directories(vuo.time.nodes
	PRIVATE
	../vuo.math
)

VuoNodeSet()

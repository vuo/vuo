VuoCompileLibraries(
	VuoLeap.cc
)
target_sources(vuo.leap.libraries PRIVATE
	VuoLeap.h
)
target_include_directories(vuo.leap.libraries
	PRIVATE
	Leap
)

VuoCompileTypes(
	VuoHorizontalSide.c
	VuoLeapFrame.c
	VuoLeapHand.c
	VuoLeapPointable.c
	VuoLeapPointableType.c
	VuoLeapTouchZone.c
)
target_sources(vuo.leap.types PRIVATE
	VuoHorizontalSide.h
	VuoLeapFrame.h
	VuoLeapHand.h
	VuoLeapPointable.h
	VuoLeapPointableType.h
	VuoLeapTouchZone.h
)

VuoCompileNodes(
	vuo.leap.find.hand.confidence.c
	vuo.leap.find.hand.id.c
	vuo.leap.find.hand.side.c
	vuo.leap.find.pointable.id.c
	vuo.leap.find.pointable.touchZone.c
	vuo.leap.find.pointable.type.c
	vuo.leap.get.frame.c
	vuo.leap.get.hand.c
	vuo.leap.get.pointable.c
	vuo.leap.hand.sort.distance.c
	vuo.leap.hand.sort.distance.x.c
	vuo.leap.hand.sort.distance.y.c
	vuo.leap.hand.sort.distance.z.c
	vuo.leap.pointable.sort.distance.c
	vuo.leap.pointable.sort.distance.x.c
	vuo.leap.pointable.sort.distance.y.c
	vuo.leap.pointable.sort.distance.z.c
	vuo.leap.receive.c
)

VuoNodeSet()

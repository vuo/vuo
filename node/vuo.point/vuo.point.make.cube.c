/**
 * @file
 * vuo.point.make.cube node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
	"title": "Make Points along Cube Edges",
	"keywords": [
		"math", "number", "list",
		"shape", "interpolate", "curve", "linear",
		"rectangular cuboid", "prism", "polyhedron", "quadrilateral", "hexahedron",
	],
	"version": "1.0.0",
	"genericTypes": {
		"VuoGenericType1": {
			"defaultType": "VuoReal",
			"compatibleTypes": [ "VuoReal", "VuoPoint3d" ],
		},
	},
	"node": {
		"exampleCompositions": [],
	},
});

static VuoPoint3d vuo_point_make_cube_getExpandedPoint_VuoReal(VuoReal size)
{
	return (VuoPoint3d){ size, size, size };
}

static VuoPoint3d vuo_point_make_cube_getExpandedPoint_VuoPoint3d(VuoPoint3d size)
{
	return size;
}

void nodeEvent(
	VuoInputData(VuoPoint3d, {"default":[ 0, 0, 0 ], "suggestedMin":[ -1, -1, -1 ], "suggestedMax":[ 1, 1, 1 ]}) center,
	VuoInputData(VuoGenericType1, {"defaults":{"VuoReal":1, "VuoPoint3d":[ 1, 1, 1 ] }, "suggestedMin":"0,0,0", "suggestedMax":"2,2,2"}) size,
	VuoInputData(VuoGenericType1, {"defaults":{"VuoReal":0, "VuoPoint3d":[ 0, 0, 0 ] }, "suggestedMin":"-1,-1,-1", "suggestedMax":"1,1,1"}) edgeOffset,
	VuoInputData(VuoCurve, {"default":"linear"}) xCurve,
	VuoInputData(VuoCurve, {"default":"linear"}) yCurve,
	VuoInputData(VuoCurve, {"default":"linear"}) zCurve,
	VuoInputData(VuoCurveEasing, {"default":"in"}) xEasing,
	VuoInputData(VuoCurveEasing, {"default":"in"}) yEasing,
	VuoInputData(VuoCurveEasing, {"default":"in"}) zEasing,
	VuoInputData(VuoInteger, {"default":8, "suggestedMin":0, "suggestedMax":128, "suggestedStep":8}) xPointCount,
	VuoInputData(VuoInteger, {"default":8, "suggestedMin":0, "suggestedMax":128, "suggestedStep":8}) yPointCount,
	VuoInputData(VuoInteger, {"default":8, "suggestedMin":0, "suggestedMax":128, "suggestedStep":8}) zPointCount,
	VuoOutputData(VuoList_VuoPoint3d) cubePoints)
{
	VuoInteger totalPointCount = (MAX(0, xPointCount) + MAX(0, yPointCount) + MAX(0, zPointCount)) * 2 * 4;
	if (totalPointCount < 1)
	{
		*cubePoints = NULL;
		return;
	}

	VuoPoint3d expandedSize = vuo_point_make_cube_getExpandedPoint_VuoGenericType1(size);
	VuoPoint3d expandedEdgeOffset = vuo_point_make_cube_getExpandedPoint_VuoGenericType1(edgeOffset);
	VuoReal fcx = expandedSize.x / 2;
	VuoReal fcy = expandedSize.y / 2;
	VuoReal fcz = expandedSize.z / 2;

	// Each startPoints[n] -> endPoints[n] pair forms an edge of the cube.
	// Same order as the ports on `Make Cube with Materials`.
	VuoPoint3d startPoints[] = {
		// Front
		(VuoPoint3d){ -fcx, -fcy,  fcz + expandedEdgeOffset.z },  // bottom left front
		(VuoPoint3d){  fcx, -fcy,  fcz + expandedEdgeOffset.z },  // bottom right front
		(VuoPoint3d){  fcx,  fcy,  fcz + expandedEdgeOffset.z },  // top right front
		(VuoPoint3d){ -fcx,  fcy,  fcz + expandedEdgeOffset.z },  // top left  front
		// Left
		(VuoPoint3d){ -fcx - expandedEdgeOffset.x, -fcy,  fcz },  // bottom left front
		(VuoPoint3d){ -fcx - expandedEdgeOffset.x, -fcy, -fcz },  // bottom left back
		(VuoPoint3d){ -fcx - expandedEdgeOffset.x,  fcy, -fcz },  // top left back
		(VuoPoint3d){ -fcx - expandedEdgeOffset.x,  fcy,  fcz },  // top left front
		// Right
		(VuoPoint3d){  fcx + expandedEdgeOffset.x, -fcy,  fcz },  // bottom right front
		(VuoPoint3d){  fcx + expandedEdgeOffset.x, -fcy, -fcz },  // bottom right back
		(VuoPoint3d){  fcx + expandedEdgeOffset.x,  fcy, -fcz },  // top right back
		(VuoPoint3d){  fcx + expandedEdgeOffset.x,  fcy,  fcz },  // top right front
		// Back
		(VuoPoint3d){ -fcx, -fcy, -fcz - expandedEdgeOffset.z },  // bottom left back
		(VuoPoint3d){  fcx, -fcy, -fcz - expandedEdgeOffset.z },  // bottom right back
		(VuoPoint3d){  fcx,  fcy, -fcz - expandedEdgeOffset.z },  // top right back
		(VuoPoint3d){ -fcx,  fcy, -fcz - expandedEdgeOffset.z },  // top left back
		// Top
		(VuoPoint3d){ -fcx,  fcy + expandedEdgeOffset.y,  fcz },  // top left front
		(VuoPoint3d){  fcx,  fcy + expandedEdgeOffset.y,  fcz },  // top right front
		(VuoPoint3d){  fcx,  fcy + expandedEdgeOffset.y, -fcz },  // top right back
		(VuoPoint3d){ -fcx,  fcy + expandedEdgeOffset.y, -fcz },  // top left back
		// Bottom
		(VuoPoint3d){ -fcx, -fcy - expandedEdgeOffset.y,  fcz },  // bottom left front
		(VuoPoint3d){  fcx, -fcy - expandedEdgeOffset.y,  fcz },  // bottom right front
		(VuoPoint3d){  fcx, -fcy - expandedEdgeOffset.y, -fcz },  // bottom right back
		(VuoPoint3d){ -fcx, -fcy - expandedEdgeOffset.y, -fcz },  // bottom left back
	};

	VuoPoint3d endPoints[] = {
		// Front
		(VuoPoint3d){  fcx, -fcy,  fcz + expandedEdgeOffset.z },  // bottom right front
		(VuoPoint3d){  fcx,  fcy,  fcz + expandedEdgeOffset.z },  // top right front
		(VuoPoint3d){ -fcx,  fcy,  fcz + expandedEdgeOffset.z },  // top left front
		(VuoPoint3d){ -fcx, -fcy,  fcz + expandedEdgeOffset.z },  // bottom left front
		// Left
		(VuoPoint3d){ -fcx - expandedEdgeOffset.x, -fcy, -fcz },  // bottom left back
		(VuoPoint3d){ -fcx - expandedEdgeOffset.x,  fcy, -fcz },  // top left back
		(VuoPoint3d){ -fcx - expandedEdgeOffset.x,  fcy,  fcz },  // top left front
		(VuoPoint3d){ -fcx - expandedEdgeOffset.x, -fcy,  fcz },  // bottom left front
		// Right
		(VuoPoint3d){  fcx + expandedEdgeOffset.x, -fcy, -fcz },  // bottom right back
		(VuoPoint3d){  fcx + expandedEdgeOffset.x,  fcy, -fcz },  // top right back
		(VuoPoint3d){  fcx + expandedEdgeOffset.x,  fcy,  fcz },  // top right front
		(VuoPoint3d){  fcx + expandedEdgeOffset.x, -fcy,  fcz },  // bottom right front
		// Back
		(VuoPoint3d){  fcx, -fcy, -fcz - expandedEdgeOffset.z },  // bottom right back
		(VuoPoint3d){  fcx,  fcy, -fcz - expandedEdgeOffset.z },  // top right back
		(VuoPoint3d){ -fcx,  fcy, -fcz - expandedEdgeOffset.z },  // top left back
		(VuoPoint3d){ -fcx, -fcy, -fcz - expandedEdgeOffset.z },  // bottom left Back
		// Top
		(VuoPoint3d){  fcx,  fcy + expandedEdgeOffset.y,  fcz },  // top right front
		(VuoPoint3d){  fcx,  fcy + expandedEdgeOffset.y, -fcz },  // top right back
		(VuoPoint3d){ -fcx,  fcy + expandedEdgeOffset.y, -fcz },  // top left back
		(VuoPoint3d){ -fcx,  fcy + expandedEdgeOffset.y,  fcz },  // top left front
		// Bottom
		(VuoPoint3d){  fcx, -fcy - expandedEdgeOffset.y,  fcz },  // bottom right front
		(VuoPoint3d){  fcx, -fcy - expandedEdgeOffset.y, -fcz },  // bottom right back
		(VuoPoint3d){ -fcx, -fcy - expandedEdgeOffset.y, -fcz },  // bottom left back
		(VuoPoint3d){ -fcx, -fcy - expandedEdgeOffset.y,  fcz },  // bottom left front
	};

	*cubePoints = VuoListCreateWithCount_VuoPoint3d(totalPointCount, (VuoPoint3d){0,0,0});
	VuoPoint3d *cubePointData = VuoListGetData_VuoPoint3d(*cubePoints);
	VuoPoint3d *cubePointDataP = cubePointData;
	for (int edge = 0; edge < 4 * 6; ++edge)
	{
		VuoPoint3d edgeVector = endPoints[edge] - startPoints[edge];
		float direction;
		int edgePointCount;
		VuoCurve edgeCurve;
		VuoCurveEasing edgeEasing;
		if (!VuoReal_areEqual(edgeVector.x, 0))
		{
			direction = edgeVector.x;
			edgePointCount = xPointCount;
			edgeCurve = xCurve;
			edgeEasing = xEasing;
		}
		else if (!VuoReal_areEqual(edgeVector.y, 0))
		{
			direction = edgeVector.y;
			edgePointCount = yPointCount;
			edgeCurve = yCurve;
			edgeEasing = yEasing;
		}
		else
		{
			direction = edgeVector.z;
			edgePointCount = zPointCount;
			edgeCurve = zCurve;
			edgeEasing = zEasing;
		}

		if (direction < 0)
		{
			// Make the easing curves always go in the +x/+y/+z direction, for consistency with `Make Points in 3D Grid`.
			if (edgeEasing == VuoCurveEasing_In)
				edgeEasing = VuoCurveEasing_Out;
			else if (edgeEasing == VuoCurveEasing_Out)
				edgeEasing = VuoCurveEasing_In;
		}

		for (int pointAlongEdge = 0; pointAlongEdge < edgePointCount; ++pointAlongEdge)
			*(cubePointData++) = center + VuoPoint3d_curve(
				pointAlongEdge,
				startPoints[edge],
				endPoints[edge],
				edgePointCount - 1,
				edgeCurve,
				edgeEasing,
				VuoLoopType_None);
	}
}

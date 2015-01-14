/**
 * @file
 * vuo.point.sort.distance.x node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Sort 3D Points by X Distance",
					  "description" :
						  "<p>Puts a list of points in order of nearest to farthest from a target point, \
						  comparing only the distance along the x-axis.</p> \
						  ",
					  "keywords" : [ "organize", "sort", "distance", "nearest", "filter", "shuffle", "point" ],
					  "version" : "1.0.0",
					  "node": {
						  "isInterface" : false
					  }
				  });

typedef struct
{
	int index;
	float value;
} sortable_pointValue;


int compare (const void * a, const void * b)
{
	sortable_pointValue *x = (sortable_pointValue*)a;
	sortable_pointValue *y = (sortable_pointValue*)b;

	return (x->value - y->value);
}

void nodeEvent
(
		VuoInputData(VuoList_VuoPoint3d) list,
		VuoInputData(VuoPoint3d, {"default":{"x":0, "y":0, "z":0}}) point,
		VuoOutputData(VuoList_VuoPoint3d) sorted
)
{
	*sorted = VuoListCreate_VuoPoint3d();

	int count = VuoListGetCount_VuoPoint3d(list);

	sortable_pointValue pointValues[count];

	for(int i = 0; i < count; i++)
		pointValues[i] = (sortable_pointValue){i, fabs(VuoListGetValueAtIndex_VuoPoint3d(list, i+1).x - point.x)};

	qsort (pointValues, count, sizeof(sortable_pointValue), compare);

	for(int i = 0; i < count; i++)
		VuoListAppendValue_VuoPoint3d(*sorted, VuoListGetValueAtIndex_VuoPoint3d(list, pointValues[i].index+1) );
}

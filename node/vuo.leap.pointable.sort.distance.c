/**
 * @file
 * vuo.leap.pointable.sort.distance node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Sort Pointables by Distance",
					  "description" :
						  "<p>Puts the pointables in order of nearest to farthest from a target point.</p> \
						  <p>The distance to the target point is calculated from each pointable's tip.</p> \
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
		VuoInputData(VuoList_VuoLeapPointable) pointables,
		VuoInputData(VuoPoint3d, {"default":{"x":0, "y":0, "z":0}}) target,
		VuoOutputData(VuoList_VuoLeapPointable) sortedPointables
)
{
	*sortedPointables = VuoListCreate_VuoLeapPointable();

	int count = VuoListGetCount_VuoLeapPointable(pointables);

	sortable_pointValue pointValues[count];

	for(int i = 0; i < count; i++)
		pointValues[i] = (sortable_pointValue){i, VuoPoint3d_squaredMagnitude(VuoPoint3d_subtract(VuoListGetValueAtIndex_VuoLeapPointable(pointables, i+1).tipPosition, target))};

	qsort (pointValues, count, sizeof(sortable_pointValue), compare);

	for(int i = 0; i < count; i++)
		VuoListAppendValue_VuoLeapPointable(*sortedPointables, VuoListGetValueAtIndex_VuoLeapPointable(pointables, pointValues[i].index+1) );
}

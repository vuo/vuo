/**
 * @file
 * vuo.point.sort.distance node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Sort 2D Points by Distance",
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


static int compare (const void * a, const void * b)
{
	sortable_pointValue *x = (sortable_pointValue*)a;
	sortable_pointValue *y = (sortable_pointValue*)b;

	return (x->value - y->value);
}

void nodeEvent
(
		VuoInputData(VuoList_VuoPoint2d) list,
		VuoInputData(VuoPoint2d, {"default":{"x":0, "y":0}}) point,
		VuoOutputData(VuoList_VuoPoint2d) sorted
)
{
	*sorted = VuoListCreate_VuoPoint2d();

	int count = VuoListGetCount_VuoPoint2d(list);

	sortable_pointValue pointValues[count];

	for(int i = 0; i < count; i++)
		pointValues[i] = (sortable_pointValue){i, VuoPoint2d_squaredMagnitude(VuoPoint2d_subtract(VuoListGetValueAtIndex_VuoPoint2d(list, i+1), point))};

	qsort (pointValues, count, sizeof(sortable_pointValue), compare);

	for(int i = 0; i < count; i++)
		VuoListAppendValue_VuoPoint2d(*sorted, VuoListGetValueAtIndex_VuoPoint2d(list, pointValues[i].index+1) );
}

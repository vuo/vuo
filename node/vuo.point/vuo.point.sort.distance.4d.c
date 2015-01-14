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
					  "title" : "Sort 4D Points by Distance",
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
		VuoInputData(VuoList_VuoPoint4d) list,
		VuoInputData(VuoPoint4d, {"default":{"x":0, "y":0, "z":0, "w":0}}) point,
		VuoOutputData(VuoList_VuoPoint4d) sorted
)
{
	*sorted = VuoListCreate_VuoPoint4d();

	int count = VuoListGetCount_VuoPoint4d(list);

	sortable_pointValue pointValues[count];

	for(int i = 0; i < count; i++)
		pointValues[i] = (sortable_pointValue){i, VuoPoint4d_squaredMagnitude(VuoPoint4d_subtract(VuoListGetValueAtIndex_VuoPoint4d(list, i+1), point))};

	qsort (pointValues, count, sizeof(sortable_pointValue), compare);

	for(int i = 0; i < count; i++)
		VuoListAppendValue_VuoPoint4d(*sorted, VuoListGetValueAtIndex_VuoPoint4d(list, pointValues[i].index+1) );
}

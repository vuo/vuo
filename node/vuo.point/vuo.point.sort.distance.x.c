/**
 * @file
 * vuo.point.sort.distance.x node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Sort Points by X Distance",
					  "keywords" : [ "organize", "order", "nearest" ],
					  "version" : "1.0.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoPoint2d", "VuoPoint3d", "VuoPoint4d" ]
						  }
					  },
					  "node": {
						  "exampleCompositions" : [ ]
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
		VuoInputData(VuoList_VuoGenericType1) list,
		VuoInputData(VuoGenericType1, {"default":{"x":0, "y":0, "z":0, "w":0}}) point,
		VuoOutputData(VuoList_VuoGenericType1) sorted
)
{
	*sorted = VuoListCreate_VuoGenericType1();

	int count = VuoListGetCount_VuoGenericType1(list);

	sortable_pointValue pointValues[count];

	for(int i = 0; i < count; i++)
		pointValues[i] = (sortable_pointValue){i, fabs(VuoListGetValue_VuoGenericType1(list, i+1).x - point.x)};

	qsort (pointValues, count, sizeof(sortable_pointValue), compare);

	for(int i = 0; i < count; i++)
		VuoListAppendValue_VuoGenericType1(*sorted, VuoListGetValue_VuoGenericType1(list, pointValues[i].index+1) );
}

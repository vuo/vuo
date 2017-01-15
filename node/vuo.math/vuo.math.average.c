/**
 * @file
 * vuo.math.average node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Average",
					 "keywords" : [ "mix", "combine", "mean", "midpoint", "middle" ],
					 "version" : "1.0.0",
					 "node" : {
						  "exampleCompositions" : [ "FollowMidpoint.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoReal) values,
		VuoOutputData(VuoReal) averageValue
)
{
	*averageValue = VuoReal_average(values);
}

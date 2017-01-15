/**
 * @file
 * vuo.noise.random.list node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <time.h>
#include <stdlib.h>

VuoModuleMetadata({
					  "title" : "Make Random List",
					  "keywords" : [ "uncorrelated", "arbitrary", "aleatoric", "chance", "pseudorandom",
							"prng", "rng", "arc4random", "uniform", "distribution", "white" ],
					  "version" : "1.0.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "defaultType" : "VuoReal",
							  "compatibleTypes" : [ "VuoInteger", "VuoReal", "VuoPoint2d", "VuoPoint3d", "VuoPoint4d" ]
						  }
					  },
					  "node" : {
						  "exampleCompositions" : [ "Scribble.vuo" ]
					  }
				  });


void nodeEvent
(
		VuoInputData(VuoGenericType1, {"defaults":{"VuoInteger":  0, "VuoReal":0., "VuoPoint2d":{"x":-1.,"y":-1.}, "VuoPoint3d":{"x":-1.,"y":-1.,"z":-1.}, "VuoPoint4d":{"x":-1.,"y":-1.,"z":-1.,"w":-1.}}}) minimum,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoInteger":100, "VuoReal":1., "VuoPoint2d":{"x": 1.,"y": 1.}, "VuoPoint3d":{"x": 1.,"y": 1.,"z": 1.}, "VuoPoint4d":{"x": 1.,"y": 1.,"z": 1.,"w": 1.}}}) maximum,
		VuoInputData(VuoInteger, {"default":10, "suggestedMin":1}) count,
		VuoOutputData(VuoList_VuoGenericType1) list
)
{
	*list = VuoListCreate_VuoGenericType1();
	for (VuoInteger i = 0; i < count; ++i)
		VuoListAppendValue_VuoGenericType1(*list, VuoGenericType1_random(minimum,maximum));
}

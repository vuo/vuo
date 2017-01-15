/**
 * @file
 * vuo.noise.random.list.seed node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <time.h>
#include <stdlib.h>

VuoModuleMetadata({
					  "title" : "Make Random List with Seed",
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
						  "exampleCompositions" : [ "PlaceSpheresRandomly.vuo" ]
					  }
				  });


unsigned short *nodeInstanceInit
(
		VuoInputData(VuoInteger, {"default":1, "suggestedMin":0}) setSeed
)
{
	unsigned short *state = (unsigned short *)calloc(1, sizeof(unsigned short)*3);
	VuoRegister(state, free);
	VuoInteger_setRandomState(state, setSeed);
	return state;
}

void nodeInstanceEvent
(
		VuoInstanceData(unsigned short *) state,
		VuoInputData(VuoInteger, {"default":1, "suggestedMin":0}) setSeed,
		VuoInputEvent({"eventBlocking":"none","data":"setSeed"}) setSeedEvent,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoInteger":  0, "VuoReal":0., "VuoPoint2d":{"x":-1.,"y":-1.}, "VuoPoint3d":{"x":-1.,"y":-1.,"z":-1.}, "VuoPoint4d":{"x":-1.,"y":-1.,"z":-1.,"w":-1.}}}) minimum,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoInteger":100, "VuoReal":1., "VuoPoint2d":{"x": 1.,"y": 1.}, "VuoPoint3d":{"x": 1.,"y": 1.,"z": 1.}, "VuoPoint4d":{"x": 1.,"y": 1.,"z": 1.,"w": 1.}}}) maximum,
		VuoInputData(VuoInteger, {"default":10, "suggestedMin":1}) count,
		VuoOutputData(VuoList_VuoGenericType1) list
)
{
	if (setSeedEvent)
		VuoInteger_setRandomState(*state, setSeed);

	*list = VuoListCreate_VuoGenericType1();
	for (VuoInteger i = 0; i < count; ++i)
		VuoListAppendValue_VuoGenericType1(*list, VuoGenericType1_randomWithState(*state, minimum, maximum));
}

void nodeInstanceFini(VuoInstanceData(unsigned short *) state)
{
}

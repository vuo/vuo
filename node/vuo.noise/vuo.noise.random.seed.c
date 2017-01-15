/**
 * @file
 * vuo.noise.random.seed node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <time.h>
#include <stdlib.h>

VuoModuleMetadata({
					  "title" : "Make Random Value with Seed",
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
						  "exampleCompositions" : [ "DisplayRandomImages.vuo" ]
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
		VuoOutputData(VuoGenericType1) value
)
{
	if (setSeedEvent)
		VuoInteger_setRandomState(*state, setSeed);

	*value = VuoGenericType1_randomWithState(*state, minimum, maximum);
}

void nodeInstanceFini(VuoInstanceData(unsigned short *) state)
{
}

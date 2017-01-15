/**
 *@file
 * vuo.math.noise node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleDetails({
	"name" : "Make Noise",
	"description" : "Generates noise.",
	"keywords" : [ ],
	"version" : "1.0.0",
	"node": {
		"isInterface" : false
	}
});

void nodeEvent
(
	VuoInputData(VuoNoise, "white") noise,
	VuoOutputData(VuoReal) value
)
{
	// do something
}

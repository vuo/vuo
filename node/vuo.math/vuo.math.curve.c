/**
 *@file
 * vuo.math.curve node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
// #include "VuoCurveCommon.h"

VuoModuleDetails({
	"name" : "Curve",
	"description" : "Calculates value given a time, type of curve, domain, and phase.",
	"keywords" : [ ],
	"version" : "1.0.0",
	"dependencies" : [
		"VuoCurveCommon"
	],
	"node": {
		"isInterface" : false
	}
});

void nodeEvent
(
	VuoInputData(VuoReal,"0.0") time,
	VuoInputData(VuoCurve, "linear") curve,
	VuoInputData(VuoCurveDomain, "clamp") domain,
	VuoOutputData(VuoReal) value
)
{
	// do something
}

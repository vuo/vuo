/**
 * @file
 * vuo.hid.scale.control node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoHidControl.h"

VuoModuleMetadata({
	"title" : "Filter and Scale Control",
	"keywords" : [],
	"version" : "2.0.0",
	"node" : {
		"exampleCompositions" : [ "MoveDotsWithTwoMice.vuo", "MoveIcosahedronWithSpacenavigator.vuo" ]
	}
});

void nodeEvent(
	VuoInputData(VuoHidControl) control,
	VuoInputEvent({ "eventBlocking" : "door", "data" : "control" }) controlEvent,
	VuoInputData(VuoText, { "default" : "*" }) name,
	VuoInputEvent({ "eventBlocking" : "wall", "data" : "name" }) nameEvent,
	VuoInputData(VuoReal, { "default" : 0.0 }) minimum,
	VuoInputEvent({ "eventBlocking" : "wall", "data" : "minimum" }) minimumEvent,
	VuoInputData(VuoReal, { "default" : 1.0 }) maximum,
	VuoInputEvent({ "eventBlocking" : "wall", "data" : "maximum" }) maximumEvent,
	VuoOutputData(VuoReal) value,
	VuoOutputEvent({ "data" : "value" }) valueEvent)
{
	if (!VuoText_compare(control.name, (VuoTextComparison){VuoTextComparison_MatchesWildcard, true}, name))
		return;

	VuoReal v = (double)(control.value - control.min) / (double)(control.max - control.min);
	*value = v * (maximum - minimum) + minimum;
	*valueEvent = true;
}

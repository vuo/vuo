/**
 * @file
 * vuo.hid.scale.control node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoHidControl.h"

VuoModuleMetadata({
					 "title" : "Filter and Scale Control",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ "MoveDotsWithTwoMice.vuo", "MoveIcosahedronWithSpacenavigator.vuo" ]
					 }
				 });


void nodeEvent
(
		VuoInputData(VuoHidControl) control,
		VuoInputEvent({"eventBlocking":"door","data":"control"}) controlEvent,

		VuoInputData(VuoText) name,
		VuoInputEvent({"eventBlocking":"wall", "data":"name"}) nameEvent,

		VuoInputData(VuoReal, {"default":0.0}) minimum,
		VuoInputEvent({"eventBlocking":"wall", "data":"minimum"}) minimumEvent,

		VuoInputData(VuoReal, {"default":1.0}) maximum,
		VuoInputEvent({"eventBlocking":"wall", "data":"maximum"}) maximumEvent,

		VuoOutputData(VuoReal) value,
		VuoOutputEvent({"data":"value"}) valueEvent
)
{
	if (!control.name || !name || !strstr(control.name, name))
		return;

	VuoReal v = (double)(control.value - control.min) / (double)(control.max - control.min);
	*value = v * (maximum - minimum) + minimum;
	*valueEvent = true;
}

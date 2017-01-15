/**
 * @file
 * vuo.hid.filter.control node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoHidControl.h"

VuoModuleMetadata({
					 "title" : "Filter Control",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });


void nodeEvent
(
		VuoInputData(VuoHidControl) control,
		VuoInputEvent({"eventBlocking":"door","data":"control"}) controlEvent,

		VuoInputData(VuoText) name,
		VuoInputEvent({"eventBlocking":"wall", "data":"name"}) nameEvent,

		VuoOutputData(VuoHidControl) filteredControl,
		VuoOutputEvent({"data":"filteredControl"}) filteredControlEvent
)
{
	if (!control.name || !name || !strstr(control.name, name))
		return;

	*filteredControl = control;
	*filteredControlEvent = true;
}

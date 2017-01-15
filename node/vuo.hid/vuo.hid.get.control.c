/**
 * @file
 * vuo.hid.get.control node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoHidControl.h"

VuoModuleMetadata({
					  "title" : "Get Control Values",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoHidControl) control,
		VuoOutputData(VuoText) name,
		VuoOutputData(VuoInteger) value,
		VuoOutputData(VuoInteger) minimum,
		VuoOutputData(VuoInteger) maximum
)
{
	*name = control.name;
	*value = control.value;
	*minimum = control.min;
	*maximum = control.max;
}

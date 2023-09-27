/**
 * @file
 * vuo.osc.make.input node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoOscInputDevice.h"

VuoModuleMetadata({
					  "title" : "Specify OSC Input",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":"Vuo OSC Server"}) name,
		VuoInputData(VuoInteger, {"default":0, "suggestedMin":0, "suggestedMax":65535}) port,
		VuoOutputData(VuoOscInputDevice) device
)
{
	*device = VuoOscInputDevice_make(name, NULL, port);
}

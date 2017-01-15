/**
 * @file
 * vuo.osc.make.output node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoOscOutputDevice.h"

VuoModuleMetadata({
					  "title" : "Make OSC Output",
					  "keywords" : [ "broadcast" ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":"Vuo OSC Client"}) name,
		VuoInputData(VuoInteger, {"default":0, "suggestedMin":0, "suggestedMax":65535}) port,
		VuoOutputData(VuoOscOutputDevice) device
)
{
	*device = VuoOscOutputDevice_makeBroadcast(name, port);
}

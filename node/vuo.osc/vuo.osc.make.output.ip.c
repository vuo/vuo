/**
 * @file
 * vuo.osc.make.output.ip node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoOscOutputDevice.h"

VuoModuleMetadata({
					  "title" : "Specify OSC IP Output",
					  "keywords" : [ "unicast" ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":"Vuo OSC Client"}) name,
		VuoInputData(VuoText, {"default":"10.0.0.1", "name":"IP Address"}) ipAddress,
		VuoInputData(VuoInteger, {"default":0, "suggestedMin":0, "suggestedMax":65535}) port,
		VuoOutputData(VuoOscOutputDevice) device
)
{
	*device = VuoOscOutputDevice_makeUnicast(name, ipAddress, port);
}

/**
 * @file
 * vuo.osc.get.output node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoOscOutputDevice.h"

VuoModuleMetadata({
					  "title" : "Get OSC Output Values",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoOscOutputDevice) device,
		VuoOutputData(VuoText) name,
		VuoOutputData(VuoText, {"name":"IP Address"}) ipAddress,
		VuoOutputData(VuoInteger) port
)
{
	*name = device.name;
	*ipAddress = device.ipAddress;
	*port = device.port;
}

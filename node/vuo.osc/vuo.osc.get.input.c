/**
 * @file
 * vuo.osc.get.input node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoOscInputDevice.h"

VuoModuleMetadata({
					  "title" : "Get OSC Input Values",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoOscInputDevice) device,
		VuoOutputData(VuoText) name,
		VuoOutputData(VuoText, {"name":"IP Address"}) ipAddress,
		VuoOutputData(VuoInteger) port
)
{
	*name = device.name;
	*ipAddress = device.ipAddress;
	*port = device.port;
}

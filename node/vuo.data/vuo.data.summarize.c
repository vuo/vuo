/**
 * @file
 * vuo.data.summarize node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Summarize Value",
					  "keywords" : [ "convert", "text", "brief", "shorten", "debug", "troubleshoot" ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ "ShowMousePosition.vuo" ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoGenericType1) value,
		VuoOutputData(VuoText) summary
)
{
	*summary = VuoGenericType1_getSummary(value);
	VuoRegister(*summary, free);
}

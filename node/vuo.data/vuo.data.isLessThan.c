/**
 * @file
 * vuo.data.isLessThan node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Is Less Than",
					  "keywords" : [ "least", "small", "few", "low", "<", "compare", "conditional", "before" ],
					  "version" : "1.0.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [
								  /* Sync with vuo.event.decreased2 */
								  "VuoInteger", /* VuoInteger first, so this node can be compiled when unspecialized, since it's a core type */
								  "VuoArtNetInputDevice",
								  "VuoArtNetOutputDevice",
								  "VuoBaudRate",
								  "VuoColor",
								  "VuoData",
								  "VuoDistribution3d",
								  "VuoDragEvent",
								  "VuoFont",
								  "VuoGridType",
								  "VuoHidControl",
								  "VuoHidDevice",
								  "VuoImageFormat",
								  "VuoMidiPitchBend",
								  "VuoMultisample",
								  "VuoNumberFormat",
								  "VuoOscInputDevice",
								  "VuoOscOutputDevice",
								  "VuoParity",
								  "VuoRange",
								  "VuoReal",
								  "VuoRectangle",
								  "VuoSerialDevice",
								  "VuoText",
								  "VuoTimeUnit",
								  "VuoTime",
								  "VuoUrl",
								  "VuoVertexAttribute",
								  "VuoWeekday",
							  ]
						  }
					  },
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoGenericType1) a,
		VuoInputData(VuoGenericType1) b,
		VuoOutputData(VuoBoolean) lessThan
)
{
	*lessThan = VuoGenericType1_isLessThan(a, b);
}

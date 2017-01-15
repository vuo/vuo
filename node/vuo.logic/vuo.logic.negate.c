/**
 * @file
 * vuo.logic.negate node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Negate",
					 "keywords" : [ "boolean", "gate", "not", "!", "flip", "inverse", "reverse", "opposite", "switch", "0", "1", "true", "false" ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoBoolean, {"default":false}) value,
		VuoOutputData(VuoBoolean) notValue
)
{
	*notValue = !value;
}

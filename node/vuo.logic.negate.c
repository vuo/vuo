/**
 * @file
 * vuo.logic.negate node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Negate",
					 "description" :
						"<p>Outputs <i>false</i> if the input was <i>true</i>, and <i>true</i> if the input was <i>false</i>.</p>",
					 "keywords" : [ "boolean", "gate", "not", "!", "flip", "inverse", "reverse", "opposite", "switch" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
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

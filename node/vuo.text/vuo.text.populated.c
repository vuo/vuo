/**
 * @file
 * vuo.text.populated node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Is Text Populated",
					 "keywords" : [
							"letters", "length", "size", "strlen", "string", "amount", "how many",
							"characters", "number", "size", "empty", "non-empty", "nonempty"
					   ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":""}) text,
		VuoOutputData(VuoBoolean) populated
)
{
	*populated = (VuoText_length(text) >= 1);
}

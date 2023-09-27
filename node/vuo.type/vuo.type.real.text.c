/**
 * @file
 * vuo.type.real.text node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

VuoModuleMetadata({
					 "title" : "Convert Real to Text",
					 "keywords" : [ ],
					 "version" : "1.1.0",
					 "node": {
						   "exampleCompositions" : [ ],
						   "isDeprecated": true
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":0.0}) real,
		VuoOutputData(VuoText) text
)
{
	*text = VuoText_format("%g", real);
	VuoRegister(*text, free);
}

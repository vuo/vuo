/**
 * @file
 * vuo.type.real.text node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

VuoModuleMetadata({
					 "title" : "Convert Real to Text",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":0.0}) real,
		VuoOutputData(VuoText) text
)
{
	char *textAsCString = VuoReal_stringFromValue(real);
	*text = VuoText_make(textAsCString);
	free(textAsCString);
}

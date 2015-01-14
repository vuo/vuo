/**
 * @file
 * vuo.type.integer.text node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <stdlib.h>
#include <string.h>

VuoModuleMetadata({
					 "title" : "Convert Integer to Text",
					 "description" :
						 "<p>Outputs an integer number as text.</p> \
						 <p>The text contains the integer's base-10 (decimal) representation.</p> \
						 <p>Integers can be used for math. Text can be used for reading and writing.</p> \
						 <p>This node is useful as a type converter to connect ports with different data types.</p>",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
	VuoInputData(VuoInteger, {"default":0}) integer,
	VuoOutputData(VuoText) text
)
{
	char *textAsCString = VuoInteger_stringFromValue(integer);
	*text = VuoText_make(textAsCString);
	free(textAsCString);
}

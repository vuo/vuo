/**
 * @file
 * vuo.type.text.integer node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <stdlib.h>

VuoModuleMetadata({
					 "title" : "Convert Text to Integer",
					 "description" :
						 "<p>Reads an integer number from text.</p> \
						 <p>If the text consists of an integer (such as <i>-5</i> or <i>123</i>), then this node outputs that integer. \
						 The integer may be in scientific notation (such as <i>1e7</i>, or <i>2e-7</i>).</p> \
						 <p>If the text consists of an integer followed by other characters (such as <i>-5a</i> or <i>123.456</i>), \
						 then this node outputs the integer (such as -5 or 123).</p> \
						 <p>If the text doesn't start with an integer (such as <i>x</i> or <i>.5</i>), then this node outputs 0.</p> \
						 <p>If the text consists of an integer that is too large (positive or negative) to represent in 64 bits, \
						 then this node outputs the closest integer that can be represented.</p> \
						 <p>Text can be used for reading and writing. Integers can be used for math.</p> \
						 <p>This node is useful as a type converter to connect ports with different data types.</p>",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":""}) text,
		VuoOutputData(VuoInteger) integer
)
{
	*integer = VuoInteger_valueFromString(text);
}

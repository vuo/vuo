/**
 * @file
 * vuo.type.text.real node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <stdlib.h>

VuoModuleMetadata({
					 "title" : "Convert Text to Real",
					 "description" :
						 "<p>Reads a real number from text.</p> \
						 <p>If the text consists of a real number (such as <i>-5</i>, <i>0.6</i>, or <i>123.456</i>), \
						 then this node outputs that real number. \
						 The real number may be in scientifc notation (such as <i>10e23</i> or <i>7.8e-9</i>). \
						 Note that <i>0.6</i> is treated as a real number but <i>.6<i/> is not. </p> \
						 <p>If the text consists of a real number followed by other characters (such as <i>-5a</i> or <i>123.456.789</i>), \
						 then this node outputs the real number (such as -5 or 123.456).</p> \
						 <p>If the text doesn't start with a real number (such as <i>x</i> or <i>.6</i>), then this node outputs 0.</p> \
						 <p>If the text consists of a real number that is too large or small to represent in 64 bits, \
						 then this node outputs the closest real number that can be represented.</p> \
						 <p>Text can be used for reading and writing. Real numbers can be used for math.</p> \
						 <p>This node is useful as a type converter to connect ports with different data types.</p>",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText,{"default":""}) text,
		VuoOutputData(VuoReal) real
)
{
	*real = VuoReal_valueFromString(text);
}

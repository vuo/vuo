/**
 * @file
 * vuo.type.integer.real node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert Integer to Real Number",
					 "description" :
						 "<p>Outputs the real number equivalent to an integer.</p> \
						 <p>The integer (such as -5 or 123) is converted to its real number format (such as -5.0 or 123.0).</p> \
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
	VuoOutputData(VuoReal) real
)
{
	*real = integer;
}

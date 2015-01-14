/**
 * @file
 * vuo.type.boolean.integer node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert Boolean to Integer",
					 "description" :
						 "<p>Outputs 0 if the input is <i>false<i> or 1 if the input is <i>true</i>.</p> \
						 <p>Boolean values can be used for logic. Integers can be used for math.</p> \
						 <p>This node is useful as a type converter to connect ports with different data types.</p>",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
	VuoInputData(VuoBoolean, {"default":false}) boolean,
	VuoOutputData(VuoInteger) integer
)
{
	*integer = boolean ? 1 : 0;
}

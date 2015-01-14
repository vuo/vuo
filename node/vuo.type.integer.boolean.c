/**
 * @file
 * vuo.type.integer.boolean node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert Integer to Boolean",
					 "description" :
						 "<p>Outputs <i>false<i> if the input is 0 or <i>true</i> if the input is anything else.</p> \
						 <p>Integers can be used for math. Boolean values can be used for logic.</p> \
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
	VuoOutputData(VuoBoolean) boolean
)
{
	*boolean = integer == 0 ? false : true;
}

/**
 * @file
 * vuo.type.text.boolean node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <stdlib.h>

VuoModuleMetadata({
					 "title" : "Convert Text to Boolean",
					 "description" :
						 "<p>Outputs <i>true</i> if the text says <i>true</i> or <i>false</i> if the text says anything else.</p> \
						 <p>Text can be used for reading and writing. Boolean values can be used for logic.</p> \
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
		VuoOutputData(VuoBoolean) boolean
)
{
	*boolean = VuoBoolean_valueFromString(text);
}

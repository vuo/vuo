/**
 * @file
 * vuo.type.integer.boolean node implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert Integer to Boolean",
					 "keywords" : [ "0", "1", "true", "false" ],
					 "version" : "1.0.0"
				 });

void nodeEvent
(
	VuoInputData(VuoInteger, {"default":0}) integer,
	VuoOutputData(VuoBoolean) boolean
)
{
	*boolean = integer == 0 ? false : true;
}

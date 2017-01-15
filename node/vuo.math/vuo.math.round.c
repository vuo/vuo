/**
 * @file
 * vuo.math.round node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "math.h"

VuoModuleMetadata({
					 "title" : "Round",
					 "keywords" : [ "near", "close", "approximate", "integer", "whole", "real" ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":0.0}) real,
		VuoOutputData(VuoInteger) rounded
)
{
	*rounded = lround(real);
}

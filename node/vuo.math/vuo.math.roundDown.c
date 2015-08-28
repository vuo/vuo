/**
 * @file
 * vuo.math.roundDown node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "math.h"

VuoModuleMetadata({
					 "title" : "Round Down",
					 "keywords" : [ "floor", "near", "close", "approximate", "integer", "whole", "real" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":0.0}) real,
		VuoOutputData(VuoInteger) roundedDownInteger
)
{
	*roundedDownInteger = (VuoInteger)floor(real);
}

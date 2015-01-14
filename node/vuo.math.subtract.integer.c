/**
 * @file
 * vuo.math.subtract.integer node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Subtract",
					 "description" :
						"<p>Calculates `a` minus `b`.</p>",
					 "keywords" : [ "difference", "minus", "-", "arithmetic", "calculate" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":0}) a,
		VuoInputData(VuoInteger, {"default":0}) b,
		VuoOutputData(VuoInteger) difference
)
{
	*difference = a - b;
}

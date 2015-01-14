/**
 * @file
 * vuo.math.divide.real node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Divide",
					 "description" :
						"<p>Calculates the quotient of `a` divided by `b`.</p>",
					 "keywords" : [ "quotient", "fraction", "/", "÷", "arithmetic", "calculate" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
	VuoInputData(VuoReal, {"default":0}) a,
	VuoInputData(VuoReal, {"default":1}) b,
	VuoOutputData(VuoReal) quotient
)
{
	*quotient = a / b;
}

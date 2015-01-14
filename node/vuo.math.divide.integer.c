/**
 * @file
 * vuo.math.divide.integer node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Divide",
					 "description" :
						"<p>Calculates the quotient and remainder of `a` divided by `b`.</p>",
					 "keywords" : [ "quotient", "remainder", "modulus", "fraction", "/", "÷", "%", "arithmetic", "calculate" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
	VuoInputData(VuoInteger, {"default":0}) a,
	VuoInputData(VuoInteger, {"default":1}) b,
	VuoOutputData(VuoInteger) quotient,
	VuoOutputData(VuoInteger) remainder
)
{
	*quotient = a / b;
	*remainder = a % b;
}

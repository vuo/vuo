/**
 * @file
 * %NodeName% node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "%NodeDisplayName%",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [ ],
					 "genericTypes" : {
						 "VuoGenericType1" : {
							"defaultType" : "VuoReal",
							"compatibleTypes" : [ "VuoInteger", "VuoReal" ]
						 }
					 },
					 "compatibleOperatingSystems": {
						 "macosx" : { "min": "10.7" }
					 },
					 "node" : {
 						  "isInterface" : false,
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":"1.0"}) value,
		VuoOutputData(VuoReal) outputValue
)
{
}

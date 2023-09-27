/**
 * @file
 * %NodeName% node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

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
						 "macosx" : { "min": "10.10" }
					 },
					 "node" : {
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

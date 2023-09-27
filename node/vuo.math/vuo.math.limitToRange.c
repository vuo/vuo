/**
 * @file
 * vuo.math.limitToRange node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					  "title" : "Limit to Range",
					  "keywords" : [ "clamp", "restrict", "wrap", "%", "limit", "bound", "range" ],
					  "version" : "1.0.1",
					  "node": {
						  "exampleCompositions" : [ ]
					  },
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "defaultType" : "VuoReal",
							  "compatibleTypes" : [ "VuoInteger", "VuoReal" ]
						  }
					  },
				  });

void nodeEvent
(
		VuoInputData(VuoGenericType1, {"default": 0}) value,
		VuoInputData(VuoGenericType1, {"default": 0}) minimum,
		VuoInputData(VuoGenericType1, {"default": 10}) maximum,
		VuoInputData(VuoWrapMode, {"default":"wrap"}) wrapMode,
		VuoOutputData(VuoGenericType1) limitedValue
)
{
	*limitedValue = value;

	switch(wrapMode)
	{
		case VuoWrapMode_Wrap:
			*limitedValue = VuoGenericType1_wrap(*limitedValue, minimum, maximum);
			break;

		case VuoWrapMode_Saturate:
			*limitedValue = VuoGenericType1_clamp(*limitedValue, minimum, maximum);
			break;
	}
}

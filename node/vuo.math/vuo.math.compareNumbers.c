/**
 * @file
 * vuo.math.compareNumbers node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include "VuoNumberComparison.h"

VuoModuleMetadata({
					  "title" : "Compare Numbers",
					  "keywords" : [
							"comparison", "relational", "operator", "mathematical", "expression", "evaluate",
							"==", "!=", "≠", "<", "<=", "≤", ">", ">=", "≥",
							"same", "identical", "equivalent", "match", "approximate", "tolerance", "conditional",
							"unequal", "inequality", "different",
							"less than", "greater than"
						],
					  "version" : "1.0.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "defaultType" : "VuoReal",
							  "compatibleTypes" : [ "VuoInteger", "VuoReal" ]
						  }
					  },
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoGenericType1, {"defaults":{"VuoInteger":0, "VuoReal":0.0}}) a,
		VuoInputData(VuoNumberComparison, {"default":"="}) comparison,
		VuoInputData(VuoGenericType1, {"defaults":{"VuoInteger":0, "VuoReal":0.0}}) b,
		VuoOutputData(VuoBoolean) result
)
{
	switch(comparison)
	{
		case VuoNumberComparison_NotEqual:
			*result = (! VuoGenericType1_areEqual(a, b));
			break;

		case VuoNumberComparison_LessThan:
			*result = (a < b);
			break;

		case VuoNumberComparison_LessThanOrEqual:
			*result = (a <= b);
			break;

		case VuoNumberComparison_GreaterThan:
			*result = (a > b);
			break;

		case VuoNumberComparison_GreaterThanOrEqual:
			*result = (a >= b);
			break;

		default: // VuoNumberComparison_Equal
			*result = VuoGenericType1_areEqual(a, b);
	}
}

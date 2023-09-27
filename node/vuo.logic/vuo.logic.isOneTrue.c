/**
 * @file
 * vuo.logic.isOneTrue node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Is One True",
					 "keywords" : [ "boolean", "condition", "test", "check", "gate", "xor", "^", "0", "1", "false" ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ "IsMouseWithinIntersectingRectangles.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoBoolean) values,
		VuoOutputData(VuoBoolean) oneTrue
)
{
	int trueCount = 0;
	unsigned long termsCount = VuoListGetCount_VuoBoolean(values);
	for (unsigned long i = 1; i <= termsCount; ++i)
		trueCount += VuoListGetValue_VuoBoolean(values, i);
	*oneTrue = (trueCount == 1);
}

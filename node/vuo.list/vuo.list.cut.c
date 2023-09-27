/**
 * @file
 * vuo.list.cut node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Cut List",
					 "keywords" : [ "part", "piece", "extract", "truncate", "trim", "sublist", "size" ],
					 "version" : "1.0.1",
					 "node" : {
						  "exampleCompositions" : [ "SpliceSquares.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) list,
		VuoInputData(VuoInteger, {"default":1,"suggestedMin":1}) startPosition,
		VuoInputData(VuoInteger, {"default":1,"suggestedMin":1}) itemCount,
		VuoOutputData(VuoList_VuoGenericType1) partialList
)
{
	*partialList = VuoListSubset_VuoGenericType1(list, startPosition, itemCount);
}

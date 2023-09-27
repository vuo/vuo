/**
 * @file
 * vuo.list.get.first node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Get First Item in List",
					 "keywords" : [ "pick", "select", "choose", "element", "member", "front", "beginning" ],
					 "version" : "1.0.0",
					 "node" : {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) list,
		VuoOutputData(VuoGenericType1) firstItem
)
{
	*firstItem = VuoListGetValue_VuoGenericType1(list, 1);
}

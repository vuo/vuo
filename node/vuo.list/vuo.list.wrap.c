/**
 * @file
 * Sort by index implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
                      "title" : "Wrap List",
                      "keywords" : [ "sort", "indecies", "big", "high", "small", "most", ">", "<", "upper", "limit", "bound", "range" ],
                      "version" : "1.0.0",
                      "genericTypes" : {
                          "VuoGenericType1" : {
                              "defaultType" : "VuoReal",
                              "compatibleTypes" : [ "VuoInteger", "VuoReal", "VuoPoint2d", "VuoPoint3d", "VuoPoint4d", "VuoImage", "VuoLayer" ]
                          }
                      },
                      "node": {
                          "exampleCompositions" : ["ShiftList.vuo"]
                      }
                  });

void nodeEvent
(
        VuoInputData(VuoList_VuoGenericType1) list,
        VuoInputData(VuoInteger, {"default":1,"suggestedMin":1}) startPosition,
        VuoOutputData(VuoList_VuoGenericType1) wrappedList
)

{
    *wrappedList = VuoListCreate_VuoGenericType1();
    VuoInteger listSize = VuoListGetCount_VuoGenericType1(list);

    for(int i = 0; i < listSize; i++)
    {
        VuoGenericType1 value = VuoListGetValue_VuoGenericType1(list,(((startPosition - 1) + i) % listSize) + 1);
        VuoListAppendValue_VuoGenericType1(*wrappedList, value);
    }
}

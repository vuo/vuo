/**
 * @file
 * Combine Lists implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

    VuoModuleMetadata({
                          "title" : "Interleave List Item Groups",
                          "keywords" : [ "sort", "indecies", "big", "high", "small", "most", ">", "<", "upper", "limit", "bound", "range" ],
                          "version" : "2.1.0",
                          "genericTypes" : {
                              "VuoGenericType1" : {
                                  "defaultType" : "VuoReal",
                                  "compatibleTypes" : [ "VuoInteger", "VuoReal", "VuoPoint2d", "VuoPoint3d", "VuoPoint4d", "VuoImage", "VuoLayer" ]
                              }
                          },
                          "node": {
                              "exampleCompositions" : ["CombineLists.vuo"]
                          }
                      });

    void nodeEvent
    (
            VuoInputData(VuoList_VuoGenericType1) list1,
            VuoInputData(VuoList_VuoGenericType1) list2,
            VuoInputData(VuoInteger, {"default":1,"suggestedMin":1}) itemPerGroup1,
            VuoInputData(VuoInteger, {"default":1,"suggestedMin":1}) itemPerGroup2,
            VuoOutputData(VuoList_VuoGenericType1) interleavedList
    )

{
    *interleavedList = VuoListCreate_VuoGenericType1();

    unsigned long list1Count = floor((VuoListGetCount_VuoGenericType1(list1) / itemPerGroup1)) + itemPerGroup1;
    unsigned long list2Count = floor((VuoListGetCount_VuoGenericType1(list2) / itemPerGroup2)) + itemPerGroup2;

    for (unsigned long i = 0, j = 0; i <= list1Count && j <= list2Count; i+= itemPerGroup1, j+= itemPerGroup2)
    {
        if(i <= list1Count)
        {
            for(unsigned long k = 1; k <= itemPerGroup1; ++k)
            {
                VuoListAppendValue_VuoGenericType1(*interleavedList, VuoListGetValue_VuoGenericType1(list1,i + k));
            }
        }

        if(j <= list2Count)
        {
            for(unsigned long m = 1; m <= itemPerGroup2; ++m)
            {
                VuoListAppendValue_VuoGenericType1(*interleavedList, VuoListGetValue_VuoGenericType1(list2,j + m));
            }
        }
    }
}

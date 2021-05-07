/**
 * @file
 * Average List Items implementation
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "math.h"

VuoModuleMetadata({
                      "title" : "Average List Items Groups",
                      "keywords" : [ "magnitude", "bigger", "smaller", "some", "small", "most", ">", "<", "upper", "range" ],
                      "version" : "1.0.0",
                      "node": {
                          "exampleCompositions" : [ "AverageListItems.vuo" ]
                      }
                  });

void nodeEvent
(
        VuoInputData(VuoList_VuoReal) list,
        VuoInputData(VuoInteger, {"default":1,"suggestedMin":1}) itemsPerGroup,
        VuoOutputData(VuoList_VuoReal) averageValues
)
{
    unsigned long listCount = VuoListGetCount_VuoReal(list);
    *averageValues = VuoListCreate_VuoReal();

    VuoList_VuoReal tempList = VuoListCreateWithCount_VuoReal(itemsPerGroup, 0.0);

    for(int i = 1; i <= listCount; i = i + itemsPerGroup)
    {
        for(int j = 1; j <= VuoListGetCount_VuoReal(tempList); ++j)
        {
            VuoListSetValue_VuoReal(tempList, VuoListGetValue_VuoReal(list, i+ (j - 1)), j, false);
        }
        VuoListAppendValue_VuoReal(*averageValues, VuoReal_average(tempList));
    }
}

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
                      "title" : "Split XYZ Lists",
                      "keywords" : [ "sort", "indecies", "big", "high", "small", "most", ">", "<", "upper", "limit", "bound", "range" ],
                      "version" : "1.0.0",
                      "genericTypes" : {
                          "VuoGenericType1" : {
                              "defaultType" : "VuoReal",
                              "compatibleTypes" : [ "VuoInteger", "VuoReal", "VuoPoint2d", "VuoPoint3d", "VuoPoint4d", "VuoImage", "VuoLayer" ]
                          }
                      },
                      "node": {
                          "exampleCompositions" : ["SplitXYZList.vuo"]
                      }
                  });

void nodeEvent
(
        VuoInputData(VuoList_VuoPoint3d) points,
        VuoOutputData(VuoList_VuoReal) x,
        VuoOutputData(VuoList_VuoReal) y,
        VuoOutputData(VuoList_VuoReal) z
)

{
    *x = VuoListCreate_VuoReal();
    *y = VuoListCreate_VuoReal();
    *z = VuoListCreate_VuoReal();
    VuoInteger listSize = VuoListGetCount_VuoPoint3d(points);

    for(int i = 1; i <= listSize; i++)
    {
        VuoPoint3d point = VuoListGetData_VuoPoint3d(points)[i -1];
        VuoListAppendValue_VuoReal(*x, point.x);
        VuoListAppendValue_VuoReal(*y, point.y);
        VuoListAppendValue_VuoReal(*z, point.z);
    }
}

/**
 * @file
 * Sort by index implementation.
 *
 * @copyright Copyright © 2012–2018 Magneson.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
                      "title" : "Split XYZW Lists",
                      "keywords" : [ "sort", "indecies", "big", "high", "small", "most", ">", "<", "upper", "limit", "bound", "range" ],
                      "version" : "1.0.0",
                      "genericTypes" : {
                          "VuoGenericType1" : {
                              "defaultType" : "VuoReal",
                              "compatibleTypes" : [ "VuoInteger", "VuoReal", "VuoPoint2d", "VuoPoint3d", "VuoPoint4d", "VuoImage", "VuoLayer" ]
                          }
                      },
                      "node": {
                          "exampleCompositions" : ["SplitXYZWList.vuo"]
                      }
                  });

void nodeEvent
(
        VuoInputData(VuoList_VuoPoint4d) points,
        VuoOutputData(VuoList_VuoReal) x,
        VuoOutputData(VuoList_VuoReal) y,
        VuoOutputData(VuoList_VuoReal) z,
        VuoOutputData(VuoList_VuoReal) w
)

{
    *x = VuoListCreate_VuoReal();
    *y = VuoListCreate_VuoReal();
    *z = VuoListCreate_VuoReal();
    *w = VuoListCreate_VuoReal();
    VuoInteger listSize = VuoListGetCount_VuoPoint4d(points);

    for(int i = 1; i <= listSize; i++)
    {
        VuoPoint4d point = VuoListGetValue_VuoPoint4d(points, i);
        VuoListAppendValue_VuoReal(*x, point.x);
        VuoListAppendValue_VuoReal(*y, point.y);
        VuoListAppendValue_VuoReal(*z, point.z);
        VuoListAppendValue_VuoReal(*w, point.w);
    }
}

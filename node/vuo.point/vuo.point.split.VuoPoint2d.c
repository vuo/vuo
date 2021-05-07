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
                      "title" : "Split XY Lists",
                      "keywords" : [ "sort", "indecies", "big", "high", "small", "most", ">", "<", "upper", "limit", "bound", "range" ],
                      "version" : "2.1.0",
                      "genericTypes" : {
                          "VuoGenericType1" : {
                              "defaultType" : "VuoReal",
                              "compatibleTypes" : [ "VuoInteger", "VuoReal", "VuoPoint2d", "VuoPoint3d", "VuoPoint4d", "VuoImage", "VuoLayer" ]
                          }
                      },
                      "node": {
                          "exampleCompositions" : ["SplitXYList.vuo"]
                      }
                  });

void nodeEvent
(
        VuoInputData(VuoList_VuoPoint3d) points,
        VuoOutputData(VuoList_VuoReal) x,
        VuoOutputData(VuoList_VuoReal) y
)

{
    *x = VuoListCreate_VuoReal();
    *y = VuoListCreate_VuoReal();
    VuoInteger listSize = VuoListGetCount_VuoPoint3d(points);

    for(int i = 1; i <= listSize; i++)
    {
        VuoPoint3d point = VuoListGetValue_VuoPoint3d(points, i);
        VuoListAppendValue_VuoReal(*x, point.x);
        VuoListAppendValue_VuoReal(*y, point.y);
    }
}

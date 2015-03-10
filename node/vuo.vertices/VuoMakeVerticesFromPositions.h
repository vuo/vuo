/**
 * @file
 * VuoMakeVerticesFromPositions interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

/**
 * Wrapper function to adapt VuoVertices functions for generic types.
 */
static VuoVertices VuoMakeVerticesFromPositions_VuoPoint2d(VuoList_VuoPoint2d positions, VuoVertices_ElementAssemblyMethod elementAssemblyMethod)
{
	return VuoVertices_makeFrom2dPoints(positions, elementAssemblyMethod);
}

/**
 * Wrapper function to adapt VuoVertices functions for generic types.
 */
static VuoVertices VuoMakeVerticesFromPositions_VuoPoint3d(VuoList_VuoPoint3d positions, VuoVertices_ElementAssemblyMethod elementAssemblyMethod)
{
	return VuoVertices_makeFrom3dPoints(positions, elementAssemblyMethod);
}

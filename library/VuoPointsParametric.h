/**
 * @file
 * VuoVerticesParametric interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "node.h"

VuoList_VuoPoint3d VuoPointsParametric1d_generate(
	VuoReal time,
	VuoText xExp,
	VuoText yExp,
	VuoText zExp,
	VuoInteger subdivisions,
	VuoReal uMin,
	VuoReal uMax);

VuoList_VuoPoint3d VuoPointsParametric2d_generate(
	VuoReal time,
	VuoText xExp,
	VuoText yExp,
	VuoText zExp,
	VuoInteger rows,
	VuoInteger columns,
	VuoReal uMin,
	VuoReal uMax,
	VuoReal vMin,
	VuoReal vMax);

#ifdef __cplusplus
}
#endif

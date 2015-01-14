/**
 * @file
 * VuoVerticesParametric interface.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "node.h"

VuoList_VuoVertices VuoVerticesParametric_generate( VuoText xExp, VuoText yExp, VuoText zExp, VuoText uExp, VuoText vExp, VuoInteger uSubdivisions, VuoInteger vSubdivisions, bool closeU, bool closeV );

#ifdef __cplusplus
}
#endif

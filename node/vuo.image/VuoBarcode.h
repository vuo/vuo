/**
 * @file
 * VuoBarcode interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "VuoImage.h"
#include "VuoText.h"
#include "VuoList_VuoText.h"

VuoText VuoBarcode_read(VuoImage image, VuoRectangle *outputPosition);

#ifdef __cplusplus
}
#endif

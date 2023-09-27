/**
 * @file
 * VuoBarcode interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "VuoImage.h"
#include "VuoText.h"
#include "VuoList_VuoText.h"

VuoText VuoBarcode_read(VuoImage image, VuoInteger format, VuoText *outputFormat, VuoRectangle *outputPosition);

#ifdef __cplusplus
}
#endif

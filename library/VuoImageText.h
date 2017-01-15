/**
 * @file
 * VuoImageText interface.
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
#include "../node/vuo.font/VuoFont.h"

VuoRectangle VuoImage_getTextRectangle(VuoText text, VuoFont font);
VuoImage VuoImage_makeText(VuoText text, VuoFont font, float backingScaleFactor);

#ifdef __cplusplus
}
#endif

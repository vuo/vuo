/**
 * @file
 * VuoTextHtml interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "VuoText.h"

VuoText VuoText_removeHtml(VuoText text);

#ifdef __cplusplus
}
#endif

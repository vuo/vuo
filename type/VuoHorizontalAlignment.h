/**
 * @file
 * VuoHorizontalAlignment C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoHorizontalAlignment_h
#define VuoHorizontalAlignment_h

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup VuoTypes
 * @defgroup VuoHorizontalAlignment VuoHorizontalAlignment
 * Horizontal alignment.
 *
 * @{
 */

/**
 * Horizontal alignment.
 */
typedef enum
{
	VuoHorizontalAlignment_Left,
	VuoHorizontalAlignment_Center,
	VuoHorizontalAlignment_Right
} VuoHorizontalAlignment;

#define VuoHorizontalAlignment_SUPPORTS_COMPARISON  ///< Instances of this type can be compared and sorted.
#include "VuoList_VuoHorizontalAlignment.h"

VuoHorizontalAlignment VuoHorizontalAlignment_makeFromJson(struct json_object * js);
struct json_object * VuoHorizontalAlignment_getJson(const VuoHorizontalAlignment value);
VuoList_VuoHorizontalAlignment VuoHorizontalAlignment_getAllowedValues(void);
char * VuoHorizontalAlignment_getSummary(const VuoHorizontalAlignment value);

bool VuoHorizontalAlignment_areEqual(const VuoHorizontalAlignment valueA, const VuoHorizontalAlignment valueB);
bool VuoHorizontalAlignment_isLessThan(const VuoHorizontalAlignment valueA, const VuoHorizontalAlignment valueB);

/**
 * Automatically generated function.
 */
///@{
char * VuoHorizontalAlignment_getString(const VuoHorizontalAlignment value);
void VuoHorizontalAlignment_retain(VuoHorizontalAlignment value);
void VuoHorizontalAlignment_release(VuoHorizontalAlignment value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

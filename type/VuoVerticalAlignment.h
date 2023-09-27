/**
 * @file
 * VuoVerticalAlignment C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoVerticalAlignment_h
#define VuoVerticalAlignment_h

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup VuoTypes
 * @defgroup VuoVerticalAlignment VuoVerticalAlignment
 * Vertical alignment.
 *
 * @{
 */

/**
 * Vertical alignment.
 */
typedef enum
{
	VuoVerticalAlignment_Top,
	VuoVerticalAlignment_Center,
	VuoVerticalAlignment_Bottom
} VuoVerticalAlignment;

#define VuoVerticalAlignment_SUPPORTS_COMPARISON  ///< Instances of this type can be compared and sorted.
#include "VuoList_VuoVerticalAlignment.h"

VuoVerticalAlignment VuoVerticalAlignment_makeFromJson(struct json_object * js);
struct json_object * VuoVerticalAlignment_getJson(const VuoVerticalAlignment value);
VuoList_VuoVerticalAlignment VuoVerticalAlignment_getAllowedValues(void);
char * VuoVerticalAlignment_getSummary(const VuoVerticalAlignment value);

bool VuoVerticalAlignment_areEqual(const VuoVerticalAlignment valueA, const VuoVerticalAlignment valueB);
bool VuoVerticalAlignment_isLessThan(const VuoVerticalAlignment valueA, const VuoVerticalAlignment valueB);

/**
 * Automatically generated function.
 */
///@{
char * VuoVerticalAlignment_getString(const VuoVerticalAlignment value);
void VuoVerticalAlignment_retain(VuoVerticalAlignment value);
void VuoVerticalAlignment_release(VuoVerticalAlignment value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

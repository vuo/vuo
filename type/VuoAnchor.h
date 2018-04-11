/**
 * @file
 * VuoAnchor C type definition.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#include "VuoHorizontalAlignment.h"
#include "VuoVerticalAlignment.h"

/// @{
typedef const struct VuoList_VuoAnchor_struct { void *l; } * VuoList_VuoAnchor;
#define VuoList_VuoAnchor_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoAnchor VuoAnchor
 * Combination vertical + horizontal alignment.
 *
 * @{
 */

/**
 * Combination vertical + horizontal alignment.
 */
typedef struct
{
	VuoHorizontalAlignment horizontalAlignment;
	VuoVerticalAlignment verticalAlignment;

	char blah[42];	///< @todo https://b33p.net/kosada/node/4124
} VuoAnchor;

VuoAnchor VuoAnchor_makeFromJson(struct json_object * js);
struct json_object * VuoAnchor_getJson(const VuoAnchor value);
char * VuoAnchor_getSummary(const VuoAnchor value);

/**
 * Returns a VuoAnchor with horizontal and vertical alignments.
 */
static inline VuoAnchor VuoAnchor_make(VuoHorizontalAlignment horizontal, VuoVerticalAlignment vertical) __attribute__((const));
static inline VuoAnchor VuoAnchor_make(VuoHorizontalAlignment horizontal, VuoVerticalAlignment vertical)
{
	return (VuoAnchor) { horizontal, vertical, "" };
}

/**
 * Returns true if the two values are equal.
 */
static inline bool VuoAnchor_areEqual(const VuoAnchor value1, const VuoAnchor value2)
{
	return (value1.horizontalAlignment == value2.horizontalAlignment &&
			value1.verticalAlignment == value2.verticalAlignment);
}

/**
 * Automatically generated function.
 */
///@{
VuoAnchor VuoAnchor_makeFromString(const char *str);
char * VuoAnchor_getString(const VuoAnchor value);
void VuoAnchor_retain(VuoAnchor value);
void VuoAnchor_release(VuoAnchor value);
///@}

/**
 * @}
 */



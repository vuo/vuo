/**
 * @file
 * VuoAnchor C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoAnchor_h
#define VuoAnchor_h

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoHorizontalAlignment.h"
#include "VuoPoint2d.h"
#include "VuoVerticalAlignment.h"

/**
 * @ingroup VuoTypes
 * @defgroup VuoAnchor VuoAnchor
 * Combination vertical + horizontal alignment.
 *
 * @{
 */

/**
 * Combination vertical + horizontal alignment.
 *
 * (VuoVerticalAlignment << 2) + VuoHorizontalAlignment
 */
typedef int64_t VuoAnchor;

#define VuoAnchor_SUPPORTS_COMPARISON  ///< Instances of this type can be compared and sorted.
#include "VuoList_VuoAnchor.h"

VuoAnchor VuoAnchor_makeFromJson(struct json_object * js);
struct json_object * VuoAnchor_getJson(const VuoAnchor value);
char * VuoAnchor_getSummary(const VuoAnchor value);
VuoList_VuoAnchor VuoAnchor_getAllowedValues(void);

/**
 * Returns a VuoAnchor with horizontal and vertical alignments.
 */
static inline VuoAnchor VuoAnchor_make(VuoHorizontalAlignment horizontal, VuoVerticalAlignment vertical) __attribute__((const));
static inline VuoAnchor VuoAnchor_make(VuoHorizontalAlignment horizontal, VuoVerticalAlignment vertical)
{
	return (vertical << 2) + horizontal;
}

/**
 * Returns the horizontal component of a VuoAnchor.
 */
static inline VuoHorizontalAlignment VuoAnchor_getHorizontal(VuoAnchor anchor) __attribute__((const));
static inline VuoHorizontalAlignment VuoAnchor_getHorizontal(VuoAnchor anchor)
{
	return (VuoHorizontalAlignment)(anchor & 0x3);
}

/**
 * Returns the horizontal component of a VuoAnchor.
 */
static inline VuoVerticalAlignment VuoAnchor_getVertical(VuoAnchor anchor) __attribute__((const));
static inline VuoVerticalAlignment VuoAnchor_getVertical(VuoAnchor anchor)
{
	return (VuoVerticalAlignment)((anchor >> 2) & 0x3);
}

/**
 * Returns a VuoAnchor with both horizontal and vertical alignments centered.
 */
static inline VuoAnchor VuoAnchor_makeCentered(void) __attribute__((const));
static inline VuoAnchor VuoAnchor_makeCentered(void)
{
	return VuoAnchor_make(VuoHorizontalAlignment_Center, VuoVerticalAlignment_Center);
}

VuoPoint2d VuoAnchor_getOffset(VuoAnchor anchor);

bool VuoAnchor_areEqual(const VuoAnchor value1, const VuoAnchor value2);
bool VuoAnchor_isLessThan(const VuoAnchor value1, const VuoAnchor value2);

/**
 * Automatically generated function.
 */
///@{
char * VuoAnchor_getString(const VuoAnchor value);
void VuoAnchor_retain(VuoAnchor value);
void VuoAnchor_release(VuoAnchor value);
///@}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

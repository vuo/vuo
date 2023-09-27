/**
 * @file
 * VuoSortOrder C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoSortOrder_h
#define VuoSortOrder_h

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup VuoTypes
 * @defgroup VuoSortOrder VuoSortOrder
 * Ascending or descending order.
 *
 * @{
 */

/**
 * Ascending or descending order.
 */
typedef enum
{
	VuoSortOrder_Ascending,
	VuoSortOrder_Descending
} VuoSortOrder;

#include "VuoList_VuoSortOrder.h"

VuoSortOrder VuoSortOrder_makeFromJson(struct json_object * js);
struct json_object * VuoSortOrder_getJson(const VuoSortOrder value);
VuoList_VuoSortOrder VuoSortOrder_getAllowedValues(void);
char * VuoSortOrder_getSummary(const VuoSortOrder value);

/// @{
/**
 * Automatically generated function.
 */
char * VuoSortOrder_getString(const VuoSortOrder value);
void VuoSortOrder_retain(VuoSortOrder value);
void VuoSortOrder_release(VuoSortOrder value);
/// @}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

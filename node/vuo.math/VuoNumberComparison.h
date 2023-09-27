/**
 * @file
 * VuoNumberComparison C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup VuoTypes
 * @defgroup VuoNumberComparison VuoNumberComparison
 * Options for numerical comparison operators.
 *
 * @{
 */

/**
 * An enum defining different numerical comparison operators.
 */
typedef enum {
	VuoNumberComparison_Equal,
	VuoNumberComparison_NotEqual,
	VuoNumberComparison_LessThan,
	VuoNumberComparison_LessThanOrEqual,
	VuoNumberComparison_GreaterThan,
	VuoNumberComparison_GreaterThanOrEqual,
} VuoNumberComparison;

#include "VuoList_VuoNumberComparison.h"

VuoNumberComparison VuoNumberComparison_makeFromJson(struct json_object * js);
struct json_object * VuoNumberComparison_getJson(const VuoNumberComparison value);
VuoList_VuoNumberComparison VuoNumberComparison_getAllowedValues(void);
char * VuoNumberComparison_getSummary(const VuoNumberComparison value);

/// @{
/**
 * Automatically generated function.
 */
char * VuoNumberComparison_getString(const VuoNumberComparison value);
void VuoNumberComparison_retain(VuoNumberComparison value);
void VuoNumberComparison_release(VuoNumberComparison value);
/// @}

/**
 * @}
*/

#ifdef __cplusplus
}
#endif

/**
 * @file
 * VuoBoolean C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoBoolean_h
#define VuoBoolean_h

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @ingroup VuoTypes
 * @defgroup VuoBoolean VuoBoolean
 * A Boolean.
 *
 * @{
 */

/**
 * A Boolean
 */
typedef unsigned long VuoBoolean;

#define VuoBoolean_SUPPORTS_COMPARISON  ///< Instances of this type can be compared and sorted.
#include "VuoList_VuoBoolean.h"

VuoBoolean VuoBoolean_makeFromJson(struct json_object * js);
struct json_object * VuoBoolean_getJson(const VuoBoolean value);
VuoList_VuoBoolean VuoBoolean_getAllowedValues(void);
char * VuoBoolean_getSummary(const VuoBoolean value);

/// @{
/**
 * Automatically generated function.
 */
char * VuoBoolean_getString(const VuoBoolean value);
void VuoBoolean_retain(VuoBoolean value);
void VuoBoolean_release(VuoBoolean value);
/// @}

bool VuoBoolean_areEqual(const VuoBoolean value1, const VuoBoolean value2);
bool VuoBoolean_isLessThan(const VuoBoolean a, const VuoBoolean b);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

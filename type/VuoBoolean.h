/**
 * @file
 * vuo.boolean C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOBOOLEAN_H
#define VUOBOOLEAN_H
struct json_object;

/// @{
typedef void * VuoList_VuoBoolean;
#define VuoList_VuoBoolean_TYPE_DEFINED
/// @}

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

VuoBoolean VuoBoolean_makeFromJson(struct json_object * js);
struct json_object * VuoBoolean_getJson(const VuoBoolean value);
VuoList_VuoBoolean VuoBoolean_getAllowedValues(void);
char * VuoBoolean_getSummary(const VuoBoolean value);

/// @{
/**
 * Automatically generated function.
 */
VuoBoolean VuoBoolean_makeFromString(const char *str);
char * VuoBoolean_getString(const VuoBoolean value);
void VuoBoolean_retain(VuoBoolean value);
void VuoBoolean_release(VuoBoolean value);
/// @}

/**
 * Returns true if the two values are equal.
 */
static inline bool VuoBoolean_areEqual(const VuoBoolean value1, const VuoBoolean value2)
{
	return value1 == value2;
}

/**
 * @}
 */

#endif

/**
 * @file
 * VuoDurationType C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUODURATIONTYPE_H
#define VUODURATIONTYPE_H

/// @{
typedef const struct VuoList_VuoDurationType_struct { void *l; } * VuoList_VuoDurationType;
#define VuoList_VuoDurationType_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoDurationType VuoDurationType
 * An enum defining different types of durations for scheduled events.
 *
 * @{
 */

/**
 * An enum defining different types of durations for scheduled events.
 */
typedef enum {
	VuoDurationType_Single,
	VuoDurationType_UntilNext,
	VuoDurationType_UntilReset
} VuoDurationType;

VuoDurationType VuoDurationType_makeFromJson(struct json_object * js);
struct json_object * VuoDurationType_getJson(const VuoDurationType value);
VuoList_VuoDurationType VuoDurationType_getAllowedValues(void);
char * VuoDurationType_getSummary(const VuoDurationType value);

/// @{
/**
 * Automatically generated function.
 */
VuoDurationType VuoDurationType_makeFromString(const char *str);
char * VuoDurationType_getString(const VuoDurationType value);
void VuoDurationType_retain(VuoDurationType value);
void VuoDurationType_release(VuoDurationType value);
/// @}

/**
 * @}
*/

#endif

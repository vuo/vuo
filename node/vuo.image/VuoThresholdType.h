/**
 * @file
 * VuoThresholdType C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOTHRESHOLDTYPE_H
#define VUOTHRESHOLDTYPE_H

/// @{
typedef const struct VuoList_VuoThresholdType_struct { void *l; } * VuoList_VuoThresholdType;
#define VuoList_VuoThresholdType_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoThresholdType VuoThresholdType
 * Defines the color mask to be applied.
 *
 * @{
 */

/**
 * An enum defining different types of color masks.
 */
typedef enum {
	VuoThresholdType_Luminance,
	VuoThresholdType_Red,
	VuoThresholdType_Green,
	VuoThresholdType_Blue,
	VuoThresholdType_Alpha
} VuoThresholdType;

VuoThresholdType VuoThresholdType_valueFromJson(struct json_object * js);
struct json_object * VuoThresholdType_jsonFromValue(const VuoThresholdType value);
VuoList_VuoThresholdType VuoThresholdType_allowedValues(void);
char * VuoThresholdType_summaryFromValue(const VuoThresholdType value);

/// @{
/**
 * Automatically generated function.
 */
VuoThresholdType VuoThresholdType_valueFromString(const char *str);
char * VuoThresholdType_stringFromValue(const VuoThresholdType value);
/// @}

/**
 * @}
*/

#endif

/**
 * @file
 * VuoWrapMode C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOWRAPMODE_H
#define VUOWRAPMODE_H

/// @{
typedef const struct VuoList_VuoWrapMode_struct { void *l; } * VuoList_VuoWrapMode;
#define VuoList_VuoWrapMode_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoWrapMode VuoWrapMode
 * An enum defining different types of wrapping.
 *
 * @{
 */

/**
 * An enum defining different types of wrapping.
 */
typedef enum {
	VuoWrapMode_Wrap,
	VuoWrapMode_Saturate
} VuoWrapMode;

VuoWrapMode VuoWrapMode_valueFromJson(struct json_object * js);
struct json_object * VuoWrapMode_jsonFromValue(const VuoWrapMode value);
VuoList_VuoWrapMode VuoWrapMode_allowedValues(void);
char * VuoWrapMode_summaryFromValue(const VuoWrapMode value);

/// @{
/**
 * Automatically generated function.
 */
VuoWrapMode VuoWrapMode_valueFromString(const char *str);
char * VuoWrapMode_stringFromValue(const VuoWrapMode value);
void VuoWrapMode_retain(VuoWrapMode value);
void VuoWrapMode_release(VuoWrapMode value);
/// @}

/**
 * @}
*/

#endif

/**
 * @file
 * VuoSizingMode C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOSIZINGMODE_H
#define VUOSIZINGMODE_H

/// @{
typedef const struct VuoList_VuoSizingMode_struct { void *l; } * VuoList_VuoSizingMode;
#define VuoList_VuoSizingMode_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoSizingMode VuoSizingMode
 * An enum defining different image fill modes.
 *
 * @{
 */

/**
 * An enum defining different image fill modes.
 */
typedef enum {
	VuoSizingMode_Stretch,
	VuoSizingMode_Fit,
	VuoSizingMode_Fill
} VuoSizingMode;

VuoSizingMode VuoSizingMode_makeFromJson(struct json_object * js);
struct json_object * VuoSizingMode_getJson(const VuoSizingMode value);
VuoList_VuoSizingMode VuoSizingMode_getAllowedValues(void);
char * VuoSizingMode_getSummary(const VuoSizingMode value);

/// @{
/**
 * Automatically generated function.
 */
VuoSizingMode VuoSizingMode_makeFromString(const char *str);
char * VuoSizingMode_getString(const VuoSizingMode value);
void VuoSizingMode_retain(VuoSizingMode value);
void VuoSizingMode_release(VuoSizingMode value);
/// @}

/**
 * @}
*/

#endif

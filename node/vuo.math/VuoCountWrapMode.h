/**
 * @file
 * VuoCountWrapMode C type definition.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOUNTWRAPMODE_H
#define VUOCOUNTWRAPMODE_H

/**
 * @ingroup VuoTypes
 * @defgroup VuoCountWrapMode VuoCountWrapMode
 * An enum defining different types of wrapping.
 *
 * @{
 */

/**
 * An enum defining different types of wrapping.
 */
typedef enum {
	VuoCountWrapMode_Wrap,
	VuoCountWrapMode_Saturate
} VuoCountWrapMode;

VuoCountWrapMode VuoCountWrapMode_valueFromJson(struct json_object * js);
struct json_object * VuoCountWrapMode_jsonFromValue(const VuoCountWrapMode value);
char * VuoCountWrapMode_summaryFromValue(const VuoCountWrapMode value);

/// @{
/**
 * Automatically generated function.
 */
VuoCountWrapMode VuoCountWrapMode_valueFromString(const char *str);
char * VuoCountWrapMode_stringFromValue(const VuoCountWrapMode value);
/// @}

/**
 * @}
*/

#endif

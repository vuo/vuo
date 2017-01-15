/**
 * @file
 * %TypeName% C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/// @{
typedef void * VuoList_%TypeName%;
#define VuoList_%TypeName%_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup %TypeName% %TypeName%
 * %TypeDescription%
 *
 * @{
 */

/**
 * %TypeDescription%
 */
typedef enum
{
	%TypeName%_%Type0KeyCamelCase%,
	%TypeName%_%Type1KeyCamelCase%,
	%TypeName%_%Type2KeyCamelCase%
} %TypeName%;

%TypeName% %TypeName%_makeFromJson(struct json_object *js);
struct json_object *%TypeName%_getJson(const %TypeName% value);
VuoList_%TypeName% %TypeName%_getAllowedValues(void);
char *%TypeName%_getSummary(const %TypeName% value);

#define %TypeName%_SUPPORTS_COMPARISON
bool %TypeName%_areEqual(const %TypeName% valueA, const %TypeName% valueB);
bool %TypeName%_isLessThan(const %TypeName% valueA, const %TypeName% valueB); 

/**
 * Automatically generated function.
 */
///@{
%TypeName% %TypeName%_makeFromString(const char *str);
char *%TypeName%_getString(const %TypeName% value);
void %TypeName%_retain(%TypeName% value);
void %TypeName%_release(%TypeName% value);
///@}

/**
 * @}
 */

/**
 * @file
 * %TypeName% C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef %TypeName:u%_H
#define %TypeName:u%_H

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
	%TypeName%_%Type2KeyCamelCase%,
} %TypeName%;

%TypeName% %TypeName%_valueFromJson(struct json_object * js);
struct json_object * %TypeName%_jsonFromValue(const %TypeName% value);
VuoList_%TypeName% %TypeName%_allowedValues(void);
char * %TypeName%_summaryFromValue(const %TypeName% value);

/**
 * Automatically generated function.
 */
///@{
%TypeName% %TypeName%_valueFromString(const char *str);
char * %TypeName%_stringFromValue(const %TypeName% value);
void %TypeName%_retain(%TypeName% value);
void %TypeName%_release(%TypeName% value);
///@}

/**
 * @}
 */

#endif // %TypeName:u%_H
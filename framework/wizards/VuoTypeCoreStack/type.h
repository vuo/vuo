/**
 * @file
 * %TypeName% C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/**
 * @ingroup VuoTypes
 * @defgroup %TypeName% %TypeName%
 * %TypeDescription%
 *
 * @{
 */

#include "VuoInteger.h"

/**
 * %TypeDescription%
 */
typedef struct
{
	VuoInteger replaceThis;	///< Explain what each member does.
} %TypeName%;

%TypeName% %TypeName%_makeFromJson(struct json_object *js);
struct json_object *%TypeName%_getJson(const %TypeName% value);
char *%TypeName%_getSummary(const %TypeName% value);
bool %TypeName%_areEqual(const %TypeName% valueA, const %TypeName% valueB);
bool %TypeName%_isLessThan(const %TypeName% valueA, const %TypeName% valueB); 

%TypeName% %TypeName%_make(VuoInteger replaceThis);

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

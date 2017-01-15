/**
 * @file
 * VuoMathExpressionList C type definition.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOMATHEXPRESSIONLIST_H
#define VUOMATHEXPRESSIONLIST_H

#include "VuoMathExpressionParser.h"
#include "VuoDictionary_VuoText_VuoReal.h"

/**
 * @ingroup VuoTypes
 * @defgroup VuoMathExpressionList VuoMathExpressionList
 * A list of mathematical expressions (e.g. ["sum=n1+n2", "product=n1*n2", "n1-n2+5"]).
 *
 * @{
 */

/**
 * An ordered list of mathematical expressions.
 */
typedef struct
{
	VuoList_VuoText expressions;  ///< The original expressions.
	VuoMathExpressionParser parser;  ///< The parsed expressions, or null if parsing failed.
	char unused;  ///< @todo https://b33p.net/kosada/node/4124
} VuoMathExpressionList;

struct json_object;

VuoMathExpressionList VuoMathExpressionList_make(VuoList_VuoText expressions);
VuoMathExpressionList VuoMathExpressionList_makeFromJson(struct json_object * js);
struct json_object * VuoMathExpressionList_getJson(const VuoMathExpressionList value);
char * VuoMathExpressionList_getSummary(const VuoMathExpressionList value);

VuoDictionary_VuoText_VuoReal VuoMathExpressionList_calculate(const VuoMathExpressionList expressionList,
															  const VuoDictionary_VuoText_VuoReal variablesAndValues);

/// @{
/**
 * Automatically generated function.
 */
VuoMathExpressionList VuoMathExpressionList_makeFromString(const char *str);
char * VuoMathExpressionList_getString(const VuoMathExpressionList value);
void VuoMathExpressionList_retain(VuoMathExpressionList value);
void VuoMathExpressionList_release(VuoMathExpressionList value);
/// @}

/**
 * @}
 */

#endif

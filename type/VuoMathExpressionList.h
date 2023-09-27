/**
 * @file
 * VuoMathExpressionList C type definition.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifndef VuoMathExpressionList_h
#define VuoMathExpressionList_h

#ifdef __cplusplus
extern "C" {
#endif

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
} VuoMathExpressionList;

VuoMathExpressionList VuoMathExpressionList_make(VuoList_VuoText expressions);
VuoMathExpressionList VuoMathExpressionList_makeFromJson(struct json_object * js);
struct json_object * VuoMathExpressionList_getJson(const VuoMathExpressionList value);
char * VuoMathExpressionList_getSummary(const VuoMathExpressionList value);

VuoDictionary_VuoText_VuoReal VuoMathExpressionList_calculate(const VuoMathExpressionList expressionList,
															  const VuoDictionary_VuoText_VuoReal variablesAndValues);

void VuoMathExpressionList_retain(VuoMathExpressionList value);
void VuoMathExpressionList_release(VuoMathExpressionList value);

/// @{
/**
 * Automatically generated function.
 */
char * VuoMathExpressionList_getString(const VuoMathExpressionList value);
/// @}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

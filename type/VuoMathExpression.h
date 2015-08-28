/**
 * @file
 * VuoMathExpression C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOMATHEXPRESSION_H
#define VUOMATHEXPRESSION_H

#include "VuoMathExpressionParser.h"

/**
 * @ingroup VuoTypes
 * @defgroup VuoMathExpression VuoMathExpression
 * A mathematical expression (e.g. "a + b" or "y = x + 1").
 *
 * @{
 */

/**
 * A mathematical expression.
 */
typedef struct
{
	VuoText expression;  ///< The original expression.
	VuoMathExpressionParser parser;  ///< The parsed expression, or null if parsing failed.
} VuoMathExpression;

struct json_object;

VuoMathExpression VuoMathExpression_valueFromJson(struct json_object * js);
struct json_object * VuoMathExpression_jsonFromValue(const VuoMathExpression value);
char * VuoMathExpression_summaryFromValue(const VuoMathExpression value);

/// @{
/**
 * Automatically generated function.
 */
VuoMathExpression VuoMathExpression_valueFromString(const char *str);
char * VuoMathExpression_stringFromValue(const VuoMathExpression value);
void VuoMathExpression_retain(VuoMathExpression value);
void VuoMathExpression_release(VuoMathExpression value);
/// @}

/**
 * @}
 */

#endif

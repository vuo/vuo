/**
 * @file
 * vuo.math.calculate.list node implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoDictionary_VuoText_VuoReal.h"

VuoModuleMetadata({
					  "title" : "Calculate List",
					  "keywords" : [
						"maths", "mathematics",
						"add", "sum", "+", "subtract", "minus", "difference", "-", "multiply", "product", "*",
						"divide", "quotient", "/", "power", "^", "modulus", "%", "if", "condition",
						"and", "&&", "or", "==", "less", "<", "<=", "greater", ">", ">=", "equal", "==", "compare",
						"sin", "cos", "tan", "asin", "acos", "atan", "atan2", "sinh", "cosh", "tanh", "asinh", "acosh", "atanh",
						"log2", "log10", "log", "ln", "exp", "sqrt", "sign", "rint", "abs", "min", "max", "sum", "avg", "pi",
						"expression", "equation", "formula", "logic", "trigonometry"
					  ],
					  "version" : "1.1.0",
					  "node": {
						  "exampleCompositions" : [ "MoveDotsWithPerlinNoise.vuo" ]
					  }
				 });


void nodeEvent
(
		VuoInputData(VuoMathExpressionList, {"default":{"expressions":["X * A"],"inputVariables":["X","A"],"outputVariables":["Result"]}}) expression,
		VuoInputData(VuoList_VuoReal) xValues,
		VuoInputData(VuoDictionary_VuoText_VuoReal, {"name":"Constants"}) values,
		VuoOutputData(VuoList_VuoReal) results
)
{
	*results = VuoMathExpressionParser_calculateList(expression.parser, xValues, values);
}

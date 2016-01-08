/**
 * @file
 * vuo.math.calculate node implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoDictionary_VuoText_VuoReal.h"

VuoModuleMetadata({
					  "title" : "Calculate",
					  "keywords" : [
						"maths", "mathematics",
						"add", "sum", "+", "subtract", "minus", "difference", "-", "multiply", "product", "*",
						"divide", "quotient", "/", "power", "^", "modulus", "%", "if", "condition",
						"and", "&&", "or", "==", "less", "<", "<=", "greater", ">", ">=", "equal", "==", "compare",
						"sin", "cos", "tan", "asin", "acos", "atan", "sinh", "cosh", "tanh", "asinh", "acosh", "atanh",
						"log2", "log10", "log", "ln", "exp", "sqrt", "sign", "rint", "abs", "min", "max", "sum", "avg", "pi",
						"expression", "equation", "formula", "logic", "trigonometry"
					  ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ "GraphFunctions.vuo" ]
					  }
				 });


void nodeEvent
(
		VuoInputData(VuoMathExpressionList, {"default":{"expressions":["A + B"],"inputVariables":["A","B"],"outputVariables":["Result"]}}) expression,
		VuoInputData(VuoDictionary_VuoText_VuoReal) values,
		VuoOutputData(VuoReal) result
)
{
	VuoDictionary_VuoText_VuoReal results = VuoMathExpressionList_calculate(expression, values);
	VuoList_VuoText outputVariables = VuoDictionaryGetKeys_VuoText_VuoReal(results);
	*result = (VuoListGetCount_VuoText(outputVariables) == 0) ?
				  0 :
				  VuoDictionaryGetValueForKey_VuoText_VuoReal(results, VuoListGetValue_VuoText(outputVariables, 1));

	VuoRetain(outputVariables);
	VuoRelease(outputVariables);
	VuoDictionary_VuoText_VuoReal_retain(results);
	VuoDictionary_VuoText_VuoReal_release(results);
}

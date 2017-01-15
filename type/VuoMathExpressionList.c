/**
 * @file
 * VuoMathExpressionList implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "type.h"
#include "VuoMathExpressionList.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Math Expression List",
					  "description" : "A list of mathematical expressions that can be used for calculations.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						"VuoDictionary_VuoText_VuoReal",
						"VuoMathExpressionParser"
					  ]
				  });
#endif
/// @}

/**
 * @ingroup VuoMathExpressionList
 * Creates a VuoMathExpressionList with the given math expressions.
 *
 * Takes ownership of @a expressions: @a expressions should not later be modified or freed by the caller.
 */
VuoMathExpressionList VuoMathExpressionList_make(VuoList_VuoText expressions)
{
	VuoMathExpressionList me;

	if (expressions && VuoListGetCount_VuoText(expressions) > 0)
	{
		me.expressions = expressions;

		VuoMathExpressionError error = NULL;
		me.parser = VuoMathExpressionParser_makeFromMultipleExpressions(me.expressions, &error);
		VuoMathExpressionError_free(error);
	}
	else
	{
		me.expressions = VuoListCreate_VuoText();
		me.parser = NULL;
	}

	return me;
}

/**
 * @ingroup VuoMathExpressionList
 * Decodes the JSON object to create a new value.
 *
 * @eg{
 *   {
 *     "expression" : [ "y = x + 4", "2 * x" ]
 *   }
 * }
 */
VuoMathExpressionList VuoMathExpressionList_makeFromJson(json_object *js)
{
	json_object *o = NULL;

	VuoList_VuoText expressions = NULL;
	if (json_object_object_get_ex(js, "expressions", &o))
		expressions = VuoList_VuoText_makeFromJson(o);

	return VuoMathExpressionList_make(expressions);
}

/**
 * @ingroup VuoMathExpressionList
 * Encodes the value as a JSON object.
 *
 * Includes the expression's variables in the JSON object, to be used when generating the Calculate node class.
 * However, the variables are ignored by VuoMathExpression_makeFromJson().
 *
 * @eg{
 *   {
 *     "expression" : [ "y = x + 4", "2 * x" ],
 *     "inputVariables" : [ "x" ],
 *     "outputVariables" : [ "y", "result" ]
 *   }
 * }
 */
json_object * VuoMathExpressionList_getJson(const VuoMathExpressionList me)
{
	json_object *js = json_object_new_object();

	json_object_object_add(js, "expressions", VuoList_VuoText_getJson(me.expressions));

	if (me.parser)
	{
		VuoList_VuoText inputVariables = VuoMathExpressionParser_getInputVariables(me.parser);
		json_object_object_add(js, "inputVariables", VuoList_VuoText_getJson(inputVariables));
		VuoRetain(inputVariables);
		VuoRelease(inputVariables);

		VuoList_VuoText outputVariables = VuoMathExpressionParser_getOutputVariables(me.parser);
		json_object_object_add(js, "outputVariables", VuoList_VuoText_getJson(outputVariables));
		VuoRetain(outputVariables);
		VuoRelease(outputVariables);
	}

	return js;
}

/**
 * @ingroup VuoMathExpressionList
 * Returns a string representation of the value.
 */
char * VuoMathExpressionList_getSummary(const VuoMathExpressionList me)
{
	return VuoList_VuoText_getSummary(me.expressions);
}

/**
 * @ingroup VuoMathExpressionList
 * Returns the output variables and values that result from evaluating the math expressions with the given
 * input variables and values.
 */
VuoDictionary_VuoText_VuoReal VuoMathExpressionList_calculate(const VuoMathExpressionList expressionList,
															  const VuoDictionary_VuoText_VuoReal variablesAndValues)
{
	if (! expressionList.parser)
		return VuoDictionaryCreate_VuoText_VuoReal();

	return VuoMathExpressionParser_calculate(expressionList.parser, variablesAndValues);
}

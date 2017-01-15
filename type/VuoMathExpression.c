/**
 * @file
 * VuoMathExpression implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <string.h>

#include "type.h"
#include "VuoMathExpression.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Math Expression",
					  "description" : "A mathematical expression that can be used for calculations.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						"c",
						"VuoMathExpressionParser"
					  ]
				  });
#endif
/// @}

/**
 * @ingroup VuoMathExpression
 * Decodes the JSON object to create a new value.
 *
 * @eg{
 *   {
 *     "expression" : "y = x + 4"
 *   }
 * }
 */
VuoMathExpression VuoMathExpression_makeFromJson(json_object *js)
{
	VuoMathExpression me;
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "expression", &o))
	{
		me.expression = VuoText_makeFromJson(o);

		VuoMathExpressionError error = NULL;
		me.parser = VuoMathExpressionParser_makeFromSingleExpression(me.expression, &error);
		free(error);
	}
	else
	{
		me.expression = VuoText_make("");
		me.parser = NULL;
	}

	return me;
}

/**
 * @ingroup VuoMathExpression
 * Encodes the value as a JSON object.
 *
 * Includes the expression's variables in the JSON object, to be used when generating the Calculate node class.
 * However, the variables are ignored by VuoMathExpression_makeFromJson().
 *
 * @eg{
 *   {
 *     "expression" : "y = x + 4",
 *     "inputVariables" : [ "x" ],
 *     "outputVariables" : [ "y" ]
 *   }
 * }
 */
json_object * VuoMathExpression_getJson(const VuoMathExpression me)
{
	json_object *js = json_object_new_object();

	json_object_object_add(js, "expression", VuoText_getJson(me.expression));

	if (me.parser)
	{
		VuoList_VuoText inputVariables = VuoMathExpressionParser_getInputVariables(me.parser);
		json_object_object_add(js, "inputVariables", VuoList_VuoText_getJson(inputVariables));
		VuoRelease(inputVariables);

		VuoList_VuoText outputVariables = VuoMathExpressionParser_getOutputVariables(me.parser);
		json_object_object_add(js, "outputVariables", VuoList_VuoText_getJson(outputVariables));
		VuoRelease(outputVariables);
	}

	return js;
}

/**
 * @ingroup VuoMathExpression
 * Returns a string representation of the value.
 */
char * VuoMathExpression_getSummary(const VuoMathExpression me)
{
	return VuoText_getSummary(me.expression);
}

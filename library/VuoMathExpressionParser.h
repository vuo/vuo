/**
 * @file
 * VuoMathExpressionParser interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOMATHEXPRESSIONPARSER_H
#define VUOMATHEXPRESSIONPARSER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "VuoInteger.h"
#include "VuoReal.h"
#include "VuoText.h"
#include "VuoList_VuoInteger.h"
#include "VuoList_VuoReal.h"
#include "VuoList_VuoText.h"
#include "VuoDictionary_VuoText_VuoReal.h"

typedef void * VuoMathExpressionParser;  ///< Parses and performs calculations with mathematical expressions.
typedef void * VuoMathExpressionError;  ///< Error caused by invalid expression given to VuoMathExpressionParser.

void VuoMathExpressionParser_defineStandardLibrary(void *muparser);

VuoMathExpressionParser VuoMathExpressionParser_makeFromSingleExpression(VuoText expression,
																		 VuoMathExpressionError *error);
VuoMathExpressionParser VuoMathExpressionParser_makeFromMultipleExpressions(VuoList_VuoText expressions,
																			VuoMathExpressionError *error);
VuoList_VuoText VuoMathExpressionParser_getInputVariables(VuoMathExpressionParser m);
VuoList_VuoText VuoMathExpressionParser_getOutputVariables(VuoMathExpressionParser m);
VuoDictionary_VuoText_VuoReal VuoMathExpressionParser_calculate(VuoMathExpressionParser m, VuoDictionary_VuoText_VuoReal inputValues);

const char * VuoMathExpressionError_getMessage(VuoMathExpressionError error);
VuoList_VuoInteger VuoMathExpressionError_getExpressionIndices(VuoMathExpressionError error);
void VuoMathExpressionError_free(VuoMathExpressionError error);

#ifdef __cplusplus
}
#endif

#endif

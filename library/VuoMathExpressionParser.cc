/**
 * @file
 * VuoMathExpressionParser implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "muParser.h"
#include <set>
#include <sstream>
using namespace std;

extern "C"
{
#include "module.h"
#include "VuoMathExpressionParser.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "VuoMathExpressionParser",
					  "dependencies" : [
						"muParser"
					  ]
				  });
#endif
}


/**
 * C++ implementation of opaque C type VuoMathExpressionError.
 */
class VuoMathExpressionErrorInternal
{
public:
	string message;  ///< A description of the error.
	vector<unsigned long> expressionIndices;  ///< Indices of the math expressions causing the error.

	/**
	 * Creates an error object copied from the muParser error.
	 */
	VuoMathExpressionErrorInternal(const mu::ParserError &error)
	{
		message = error.GetMsg();

		size_t positionIndex = message.find(" at position ");
		if (positionIndex != string::npos)
			message = message.substr(0, positionIndex);
	}

	/**
	 * Creates an error object with the given description.
	 */
	VuoMathExpressionErrorInternal(const string &message)
	{
		this->message = message;
	}

	/**
	 * Creates an error object with the given description.
	 */
	VuoMathExpressionErrorInternal(const char *message)
	{
		this->message = message;
	}
};

/**
 * Returns a description of the error.
 *
 * The returned pointer is owned by @a error and should not be freed by the caller.
 */
const char * VuoMathExpressionError_getMessage(VuoMathExpressionError error)
{
	VuoMathExpressionErrorInternal *e = static_cast<VuoMathExpressionErrorInternal *>(error);
	return e->message.c_str();
}

/**
 * Returns a sorted list of the indices of the math expressions causing the error.
 *
 * The indices refer to the VuoList_VuoText passed to VuoMathExpressionParser_makeFromMultipleExpressions().
 */
VuoList_VuoInteger VuoMathExpressionError_getExpressionIndices(VuoMathExpressionError error)
{
	VuoMathExpressionErrorInternal *e = static_cast<VuoMathExpressionErrorInternal *>(error);

	VuoList_VuoInteger indices = VuoListCreate_VuoInteger();
	for (vector<unsigned long>::iterator i = e->expressionIndices.begin(); i != e->expressionIndices.end(); ++i)
		VuoListAppendValue_VuoInteger(indices, *i);

	return indices;
}

/**
 * Destructor.
 */
void VuoMathExpressionError_free(VuoMathExpressionError error)
{
	VuoMathExpressionErrorInternal *e = static_cast<VuoMathExpressionErrorInternal *>(error);
	delete e;
}


static const size_t MAX_VARIABLE_COUNT = 64;  ///< Maximum number of variables that can be parsed by a VuoMathExpressionParser.

/**
 * Converts degrees to radians.
 */
static double deg2rad(double degrees)
{
	return degrees * 0.0174532925;
}

/**
 * Converts radians to degrees.
 */
static double rad2deg(double radians)
{
	return radians * 57.2957795;
}

/// @{
/**
 * Trigonometric functions that take degrees instead of radians.
 */
static double sinInDegrees(double degrees) { return sin(deg2rad(degrees)); }
static double cosInDegrees(double degrees) { return cos(deg2rad(degrees)); }
static double tanInDegrees(double degrees) { return tan(deg2rad(degrees)); }
static double asinInDegrees(double x) { return rad2deg(asin(x)); }
static double acosInDegrees(double x) { return rad2deg(acos(x)); }
static double atanInDegrees(double x) { return rad2deg(atan(x)); }
static double sinhInDegrees(double degrees) { return sinh(deg2rad(degrees)); }
static double coshInDegrees(double degrees) { return cosh(deg2rad(degrees)); }
static double tanhInDegrees(double degrees) { return tanh(deg2rad(degrees)); }
static double asinhInDegrees(double x) { return rad2deg(asinh(x)); }
static double acoshInDegrees(double x) { return rad2deg(acosh(x)); }
static double atanhInDegrees(double x) { return rad2deg(atanh(x)); }
/// @}

/**
 * C++ implementation of opaque C type VuoMathExpressionParser.
 */
class VuoMathExpressionParserInternal
{
public:
	mu::Parser muParser;  ///< Does most of the actual parsing work.
	size_t variableCount;  ///< Number of variables parsed.
	double variableValues[MAX_VARIABLE_COUNT];  ///< Storage for variables parsed, used to hold values for calculations.
	map<string, size_t> variableNamesAndIndices;  ///< A mapping of variable names to indices in @c variableValues.
	vector<string> inputVariableNames;  ///< The unique input variable names parsed, in alphabetical order.
	vector<string> outputVariableNames;  ///< The output variable name for each expression parsed, in the order the expressions were given.

	/**
	 * Callback needed by mu::Parser to parse each variable.
	 */
	static double * addVariable(const char *variable, void *userData)
	{
		VuoMathExpressionParserInternal *mi = static_cast<VuoMathExpressionParserInternal *>(userData);

		if (mi->variableCount == MAX_VARIABLE_COUNT)
			throw mu::ParserError("Too many variables");

		size_t index = mi->variableCount;
		mi->variableCount++;
		mi->variableValues[index] = 0;
		mi->variableNamesAndIndices[variable] = index;
		return &mi->variableValues[index];
	}

	/**
	 * Parses the mathematical expression(s) (e.g. "a + b" or "y = x + 1" or "sum=n1+n2,product=n1*n2").
	 *
	 * If @a outputVariableNames_ is given, assigns unique output variable names for any empty elements.
	 *
	 * @throw mu::ParserError @a expression contains a syntax error.
	 */
	VuoMathExpressionParserInternal(const string &expression, const vector<string> &outputVariableNames_ = vector<string>())
	{
		this->variableCount = 0;
		this->outputVariableNames = outputVariableNames_;

		muParser.DefineOprt("%", fmod, mu::prMUL_DIV, mu::oaLEFT, true);
		muParser.DefineFun("deg2rad", deg2rad, false);
		muParser.DefineFun("rad2deg", rad2deg, false);
		muParser.DefineConst("PI", (double)3.14159265359);

		muParser.DefineFun("sin", sinInDegrees, false);
		muParser.DefineFun("cos", cosInDegrees, false);
		muParser.DefineFun("tan", tanInDegrees, false);
		muParser.DefineFun("asin", asinInDegrees, false);
		muParser.DefineFun("acos", acosInDegrees, false);
		muParser.DefineFun("atan", atanInDegrees, false);
		muParser.DefineFun("sinh", sinhInDegrees, false);
		muParser.DefineFun("cosh", coshInDegrees, false);
		muParser.DefineFun("tanh", tanhInDegrees, false);
		muParser.DefineFun("asinh", asinhInDegrees, false);
		muParser.DefineFun("acosh", acoshInDegrees, false);
		muParser.DefineFun("atanh", atanhInDegrees, false);

		muParser.SetVarFactory(addVariable, this);
		muParser.SetExpr(expression);  // checks for some errors
		muParser.GetUsedVar();  // checks for more errors

		if (! outputVariableNames.empty())
		{
			string baseName = "result";
			if (muParser.GetNumResults() == 1)
			{
				if (outputVariableNames[0].empty())
					outputVariableNames[0] = baseName;
			}
			else
			{
				for (size_t i = 0; i < outputVariableNames.size(); ++i)
				{
					if (outputVariableNames[i].empty())
					{
						string name;
						int suffix = 1;
						do {
							ostringstream oss;
							oss << baseName << suffix;
							name = oss.str();
							++suffix;
						} while (find(outputVariableNames.begin(), outputVariableNames.end(), name) != outputVariableNames.end());
						outputVariableNames[i] = name;
					}
				}
			}
		}

		for (map<string, size_t>::iterator i = variableNamesAndIndices.begin(); i != variableNamesAndIndices.end(); ++i)
			if (find(outputVariableNames.begin(), outputVariableNames.end(), i->first) == outputVariableNames.end())
				inputVariableNames.push_back(i->first);

		std::sort(inputVariableNames.begin(), inputVariableNames.end());
	}

	/**
	 * Returns the token that separates expressions when multiple of them appear in a string (e.g. ",").
	 */
	static string getExpressionSeparator(void)
	{
		return string(1, mu::Parser().GetArgSep());
	}
};


/**
 * Returns the parts of @a expression separated by (and not including) the assignment operator ("=").
 *
 * Assumes that @a expression satisfies checkSyntaxOfSingleExpression().
 */
static vector<string> splitSingleExpressionOnAssignmentOperators(string expression)
{
	vector<string> expressionParts;

	size_t partStartIndex, searchStartIndex, assignmentIndex;
	partStartIndex = searchStartIndex = 0;
	while ((assignmentIndex = expression.find("=", searchStartIndex)) != string::npos)
	{
		char charBefore = expression[assignmentIndex - 1];
		char charAfter = expression[assignmentIndex + 1];
		if (charBefore == '<' || charBefore == '>' || charBefore == '!')
			searchStartIndex = assignmentIndex + 1;
		else if (charAfter == '=')
			searchStartIndex = assignmentIndex + 2;
		else
		{
			string part = expression.substr(partStartIndex, assignmentIndex - partStartIndex);
			expressionParts.push_back(part);
			partStartIndex = searchStartIndex = assignmentIndex + 1;
		}
	}

	string part = expression.substr(partStartIndex);
	expressionParts.push_back(part);

	return expressionParts;
}

/**
 * Checks that @a expression contains a syntactically valid, single math expression with at most one assignment operator.
 */
static void checkSyntaxOfSingleExpression(string expression, VuoMathExpressionError *error)
{
	try
	{
		VuoMathExpressionParserInternal parser(expression);

		if (parser.muParser.GetNumResults() > 1)
		{
			string separator = VuoMathExpressionParserInternal::getExpressionSeparator();
			*error = new VuoMathExpressionErrorInternal("Unexpected \"" + separator + "\"");
			return;
		}
	}
	catch (mu::ParserError &e)
	{
		*error = new VuoMathExpressionErrorInternal(e);
		return;
	}

	vector<string> expressionParts = splitSingleExpressionOnAssignmentOperators(expression);
	if (expressionParts.size() > 2)
	{
		*error = new VuoMathExpressionErrorInternal("Too many \"=\" operators");
		return;
	}
}

/**
 * Checks that each single math expression in @a expressions satisfies checkSyntaxOfSingleExpression().
 */
static void checkSyntaxOfMultipleExpressions(VuoList_VuoText expressions, VuoMathExpressionError *error)
{
	unsigned long count = VuoListGetCount_VuoText(expressions);
	for (unsigned long i = 1; i <= count; ++i)
	{
		VuoText expression = VuoListGetValueAtIndex_VuoText(expressions, i);
		checkSyntaxOfSingleExpression(expression, error);
		if (*error)
		{
			VuoMathExpressionErrorInternal *e = static_cast<VuoMathExpressionErrorInternal *>(*error);
			e->expressionIndices.push_back(i);
			return;
		}
	}
}

/**
 * Parses the unique variable names from @a expression and puts them in @a inputVariableNames and @a outputVariableName.
 *
 * Assumes that @a expression satisfies checkSyntaxOfSingleExpression().
 */
static void parseVariablesFromSingleExpression(string expression,
											   vector<string> &inputVariableNames, string &outputVariableName,
											   VuoMathExpressionError *error)
{
	vector<string> expressionParts = splitSingleExpressionOnAssignmentOperators(expression);

	if (expressionParts.size() > 1)
	{
		string outputExpression = expressionParts.front();
		VuoMathExpressionParserInternal outputParser(outputExpression);
		outputVariableName = *outputParser.inputVariableNames.begin();
	}

	VuoMathExpressionParserInternal inputParser(expressionParts.back());
	inputVariableNames = inputParser.inputVariableNames;

	if (! outputVariableName.empty() && find(inputVariableNames.begin(), inputVariableNames.end(), outputVariableName) != inputVariableNames.end())
	{
		*error = new VuoMathExpressionErrorInternal("Variable \"" + outputVariableName + "\" can't be on both sides of the \"=\" operator");
		return;
	}
}

/**
 * Parses the unique variable names from @a expressions and puts them in @a inputVariableNames and @a outputVariableNames.
 *
 * Assumes that @a expressions satisfies checkSyntaxOfMultipleExpressions().
 */
static void parseVariablesFromMultipleExpressions(VuoList_VuoText expressions,
												  vector<string> &inputVariableNames, vector<string> &outputVariableNames,
												  VuoMathExpressionError *error)
{
	map<string, vector<unsigned long> > expressionsForInputVariable;
	map<string, vector<unsigned long> > expressionsForOutputVariable;

	unsigned long count = VuoListGetCount_VuoText(expressions);
	for (unsigned long i = 1; i <= count; ++i)
	{
		VuoText expression = VuoListGetValueAtIndex_VuoText(expressions, i);

		vector<string> currInputVariableNames;
		string currOutputVariableName;
		parseVariablesFromSingleExpression(expression, currInputVariableNames, currOutputVariableName, error);
		if (*error)
		{
			VuoMathExpressionErrorInternal *e = static_cast<VuoMathExpressionErrorInternal *>(*error);
			e->expressionIndices.push_back(i);
			return;
		}

		for (vector<string>::iterator j = currInputVariableNames.begin(); j != currInputVariableNames.end(); ++j)
			expressionsForInputVariable[*j].push_back(i);
		expressionsForOutputVariable[currOutputVariableName].push_back(i);

		inputVariableNames.insert(inputVariableNames.end(), currInputVariableNames.begin(), currInputVariableNames.end());

		if (! currOutputVariableName.empty() && find(outputVariableNames.begin(), outputVariableNames.end(), currOutputVariableName) != outputVariableNames.end())
		{
			VuoMathExpressionErrorInternal *e = new VuoMathExpressionErrorInternal("Variable \"" + currOutputVariableName + "\" can't be on the left side of the \"=\" operator in multiple expressions");
			e->expressionIndices = expressionsForOutputVariable[currOutputVariableName];
			*error = e;
			return;
		}
		outputVariableNames.push_back(currOutputVariableName);
	}

	set<string> allVariableNames;
	allVariableNames.insert(inputVariableNames.begin(), inputVariableNames.end());
	allVariableNames.insert(outputVariableNames.begin(), outputVariableNames.end());
	for (set<string>::iterator i = allVariableNames.begin(); i != allVariableNames.end(); ++i)
	{
		if (find(inputVariableNames.begin(), inputVariableNames.end(), *i) != inputVariableNames.end() &&
				find(outputVariableNames.begin(), outputVariableNames.end(), *i) != outputVariableNames.end())
		{
			VuoMathExpressionErrorInternal *e = new VuoMathExpressionErrorInternal("Variable \"" + *i + "\" can't be on both sides of the \"=\" operator");

			vector<unsigned long> expressionsForVariable( expressionsForInputVariable[*i].size() + expressionsForOutputVariable[*i].size() );
			merge( expressionsForInputVariable[*i].begin(), expressionsForInputVariable[*i].end(),
					expressionsForOutputVariable[*i].begin(), expressionsForOutputVariable[*i].end(),
					expressionsForVariable.begin() );
			vector<unsigned long>::iterator endIter = unique( expressionsForVariable.begin(), expressionsForVariable.end() );
			expressionsForVariable.resize( distance( expressionsForVariable.begin(), endIter ) );
			e->expressionIndices = expressionsForVariable;

			*error = e;
			return;
		}
	}
}


/**
 * Destroys @a me, a VuoMathExpressionParser.
 */
void VuoMathExpressionParser_free(void *me)
{
	delete static_cast<VuoMathExpressionParserInternal *>(me);
}

/**
 * Parses @a expression (e.g. "a + b" or "y = x + 1").
 *
 * If parsing fails, returns null and sets @a error.
 */
VuoMathExpressionParser VuoMathExpressionParser_makeFromSingleExpression(VuoText expression,
																		 VuoMathExpressionError *error)
{
	checkSyntaxOfSingleExpression(expression, error);
	if (*error)
		return NULL;

	vector<string> inputVariableNames;
	string outputVariableName;
	parseVariablesFromSingleExpression(expression, inputVariableNames, outputVariableName, error);
	if (*error)
		return NULL;
	vector<string> outputVariableNames(1, outputVariableName);

	VuoMathExpressionParser m = (VuoMathExpressionParser)new VuoMathExpressionParserInternal(expression, outputVariableNames);
	VuoRegister(m, VuoMathExpressionParser_free);
	return m;
}

/**
 * Parses @a expressions (e.g. ["sum=n1+n2", "product=n1*n2", "n1-n2+5"]).
 *
 * If parsing fails, returns null and sets @a error.
 */
VuoMathExpressionParser VuoMathExpressionParser_makeFromMultipleExpressions(VuoList_VuoText expressions,
																			VuoMathExpressionError *error)
{
	checkSyntaxOfMultipleExpressions(expressions, error);
	if (*error)
		return NULL;

	vector<string> inputVariableNames;
	vector<string> outputVariableNames;
	parseVariablesFromMultipleExpressions(expressions, inputVariableNames, outputVariableNames, error);
	if (*error)
		return NULL;

	string joinedExpression;
	string separator = VuoMathExpressionParserInternal::getExpressionSeparator();
	unsigned long count = VuoListGetCount_VuoText(expressions);
	for (unsigned long i = 1; i <= count; ++i)
	{
		VuoText assignmentExpression = VuoListGetValueAtIndex_VuoText(expressions, i);
		joinedExpression += assignmentExpression;
		if (i < count)
			joinedExpression += separator;
	}

	VuoMathExpressionParser m = (VuoMathExpressionParser)new VuoMathExpressionParserInternal(joinedExpression, outputVariableNames);
	VuoRegister(m, VuoMathExpressionParser_free);
	return m;
}

/**
 * Returns the unique input variable names from the parsed expression(s), in alphabetical order.
 */
VuoList_VuoText VuoMathExpressionParser_getInputVariables(VuoMathExpressionParser m)
{
	VuoMathExpressionParserInternal *mi = static_cast<VuoMathExpressionParserInternal *>(m);
	VuoList_VuoText inputVariables = VuoListCreate_VuoText();

	for (vector<string>::iterator i = mi->inputVariableNames.begin(); i != mi->inputVariableNames.end(); ++i)
		VuoListAppendValue_VuoText(inputVariables, VuoText_make((*i).c_str()));

	return inputVariables;
}

/**
 * Returns the output variable names from the parsed expression(s), in the order they appeared.
 *
 * For a non-assignment expression (e.g. "x + 1", as opposed to "y = x + 1"), a unique name is automatically assigned.
 */
VuoList_VuoText VuoMathExpressionParser_getOutputVariables(VuoMathExpressionParser m)
{
	VuoMathExpressionParserInternal *mi = static_cast<VuoMathExpressionParserInternal *>(m);
	VuoList_VuoText outputVariables = VuoListCreate_VuoText();

	for (vector<string>::iterator i = mi->outputVariableNames.begin(); i != mi->outputVariableNames.end(); ++i)
		VuoListAppendValue_VuoText(outputVariables, VuoText_make((*i).c_str()));

	return outputVariables;
}

/**
 * Evaluates the parsed expression(s).
 *
 * The value of each input variable is set from @a inputValues (a mapping of variable names to values).
 * Any variables not in @a inputValues are set to 0.
 *
 * Returns a mapping of output variable names to values.
 */
VuoDictionary_VuoText_VuoReal VuoMathExpressionParser_calculate(VuoMathExpressionParser m, VuoDictionary_VuoText_VuoReal inputValues)
{
	VuoMathExpressionParserInternal *mi = static_cast<VuoMathExpressionParserInternal *>(m);

	for (size_t i = 0; i < MAX_VARIABLE_COUNT; ++i)
		mi->variableValues[i] = 0;

	VuoList_VuoText inputVariables = VuoDictionaryGetKeys_VuoText_VuoReal(inputValues);
	unsigned long inputCount = VuoListGetCount_VuoText(inputVariables);
	for (int i = 0; i < inputCount; ++i)
	{
		VuoText variable = VuoListGetValueAtIndex_VuoText(inputVariables, i+1);
		map<string, size_t>::iterator variableIter = mi->variableNamesAndIndices.find(variable);
		if (variableIter != mi->variableNamesAndIndices.end())
		{
			size_t index = variableIter->second;
			VuoReal value = VuoDictionaryGetValueForKey_VuoText_VuoReal(inputValues, variable);
			mi->variableValues[index] = value;
		}
	}
	VuoRetain(inputVariables);
	VuoRelease(inputVariables);

	int outputCount;
	double *results = mi->muParser.Eval(outputCount);

	VuoDictionary_VuoText_VuoReal outputValues = VuoDictionaryCreate_VuoText_VuoReal();
	for (int i = 0; i < outputCount; ++i)
	{
		VuoText key = VuoText_make( mi->outputVariableNames[i].c_str() );
		VuoReal value = results[i];
		VuoDictionarySetKeyValue_VuoText_VuoReal(outputValues, key, value);
	}

	return outputValues;
}

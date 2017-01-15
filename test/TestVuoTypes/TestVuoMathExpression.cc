/**
 * @file
 * TestVuoMathExpression implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoMathExpressionList.h"
}

/**
 * Tests the VuoMathExpression and VuoMathExpressionList types.
 */
class TestVuoMathExpression : public QObject
{
	Q_OBJECT

private slots:

	void testParsingSingleExpression_data()
	{
		QTest::addColumn<QString>("serialized");
		QTest::addColumn<QString>("reserialized");

		{
			const char *expression = "";
			QTest::newRow("empty expression") << QString("{\"expressions\":[\"%1\"]}").arg(expression)
											  << QString("{\"expressions\":[\"%1\"]}").arg(expression);
		}
		{
			const char *expression = "log10( (6+4) * 100 )";
			QTest::newRow("no variables") << QString("{\"expressions\":[\"%1\"]}").arg(expression)
										  << QString("{\"expressions\":[\"%1\"],\"inputVariables\":[],\"outputVariables\":[\"result\"]}").arg(expression);
		}
		{
			const char *expression = "2*x_a + 3*x_b";
			QTest::newRow("input variables") << QString("{\"expressions\":[\"%1\"]}").arg(expression)
											 << QString("{\"expressions\":[\"%1\"],\"inputVariables\":[\"x_a\",\"x_b\"],\"outputVariables\":[\"result\"]}").arg(expression);
		}
		{
			const char *expression = "force = mass * acceleration";
			QTest::newRow("input and output variables") << QString("{\"expressions\":[\"%1\"]}").arg(expression)
														<< QString("{\"expressions\":[\"%1\"],\"inputVariables\":[\"acceleration\",\"mass\"],\"outputVariables\":[\"force\"]}").arg(expression);
		}
		{
			const char *expression = "a =";
			QTest::newRow("missing right side of assignment") << QString("{\"expressions\":[\"%1\"]}").arg(expression)
															 << QString("{\"expressions\":[\"%1\"]}").arg(expression);
		}
		{
			const char *expression = "= 1";
			QTest::newRow("missing left side of assignment") << QString("{\"expressions\":[\"%1\"]}").arg(expression)
															 << QString("{\"expressions\":[\"%1\"]}").arg(expression);
		}
		{
			const char *expression = "1 = 2";
			QTest::newRow("non-variable on left side of assignment") << QString("{\"expressions\":[\"%1\"]}").arg(expression)
																	 << QString("{\"expressions\":[\"%1\"]}").arg(expression);
		}
		{
			const char *expression = "a = a + 1";
			QTest::newRow("variable used on both sides of assignment") << QString("{\"expressions\":[\"%1\"]}").arg(expression)
																	   << QString("{\"expressions\":[\"%1\"]}").arg(expression);
		}
	}
	void testParsingSingleExpression()
	{
		QFETCH(QString, serialized);
		QFETCH(QString, reserialized);

		VuoMathExpressionList v = VuoMathExpressionList_makeFromString(serialized.toUtf8().data());
		QCOMPARE(QString::fromUtf8(VuoMathExpressionList_getString(v)), reserialized);
	}

	void testParsingMultipleExpressions_data()
	{
		QTest::addColumn<QString>("serialized");
		QTest::addColumn<QString>("reserialized");

		{
			const char *expression = "";
			QTest::newRow("empty list") << QString("{\"expressions\":[\"%1\"]}").arg(expression)
										<< QString("{\"expressions\":[\"%1\"]}").arg(expression);
		}
		{
			const char *expression = "y = 2*x^2";
			QTest::newRow("single assignment") << QString("{\"expressions\":[\"%1\"]}").arg(expression)
											   << QString("{\"expressions\":[\"%1\"],\"inputVariables\":[\"x\"],\"outputVariables\":[\"y\"]}").arg(expression);
		}
		{
			const char *expression1 = "y1 = 2*x1 + a";
			const char *expression2 = "y2 = 2*x2 + a";
			QTest::newRow("multiple assignments") << QString("{\"expressions\":[\"%1\",\"%2\"]}").arg(expression1).arg(expression2)
												  << QString("{\"expressions\":[\"%1\",\"%2\"],\"inputVariables\":[\"a\",\"x1\",\"x2\"],\"outputVariables\":[\"y1\",\"y2\"]}").arg(expression1).arg(expression2);
		}
		{
			const char *expression1 = "a = b + 1";
			const char *expression2 = "c + 2";
			const char *expression3 = "d = b + 3";
			const char *expression4 = "c + 4";
			QTest::newRow("mix of assignments and non-assignments") << QString("{\"expressions\":[\"%1\",\"%2\",\"%3\",\"%4\"]}").arg(expression1).arg(expression2).arg(expression3).arg(expression4)
																	<< QString("{\"expressions\":[\"%1\",\"%2\",\"%3\",\"%4\"],\"inputVariables\":[\"b\",\"c\"],\"outputVariables\":[\"a\",\"result1\",\"d\",\"result2\"]}").arg(expression1).arg(expression2).arg(expression3).arg(expression4);
		}
		{
			const char *expression1 = "a = b";
			const char *expression2 = "b = c";
			QTest::newRow("variable used on both sides of assignments") << QString("{\"expressions\":[\"%1\",\"%2\"]}").arg(expression1).arg(expression2)
																		<< QString("{\"expressions\":[\"%1\",\"%2\"]}").arg(expression1).arg(expression2);
		}
		{
			const char *expression1 = "a = b";
			const char *expression2 = "a = c";
			QTest::newRow("variable used on left side of multiple assignments") << QString("{\"expressions\":[\"%1\",\"%2\"]}").arg(expression1).arg(expression2)
																				<< QString("{\"expressions\":[\"%1\",\"%2\"]}").arg(expression1).arg(expression2);
		}
	}
	void testParsingMultipleExpressions()
	{
		QFETCH(QString, serialized);
		QFETCH(QString, reserialized);

		VuoMathExpressionList v = VuoMathExpressionList_makeFromString(serialized.toUtf8().data());
		QCOMPARE(QString::fromUtf8(VuoMathExpressionList_getString(v)), reserialized);
	}

	void testCalculating_data()
	{
		QTest::addColumn<QString>("serializedExpressions");
		QTest::addColumn<QString>("serializedInputVariables");
		QTest::addColumn<QString>("serializedInputValues");
		QTest::addColumn<QString>("serializedOutputVariables");
		QTest::addColumn<QString>("serializedOutputValues");

		{
			const char *expression1 = "log10( (6+4) * 100 )";
			QTest::newRow("no variables") << QString("[\"%1\"]").arg(expression1)
										  << "[]" << "[]"
										  << "[\"result\"]" << "[3]";
		}
		{
			const char *expression1 = "y = 2*x^2";
			QTest::newRow("single assignment") << QString("[\"%1\"]").arg(expression1)
											   << "[\"x\"]" << "[10]"
											   << "[\"y\"]" << "[200]";
		}
		{
			const char *expression1 = "y1 = 2*x1 + a";
			const char *expression2 = "y2 = 2*x2 + a";
			QTest::newRow("multiple assignments") << QString("[\"%1\",\"%2\"]").arg(expression1).arg(expression2)
												  << "[\"a\",\"x1\",\"x2\"]" << "[0.1, 10, 100]"
												  << "[\"y1\",\"y2\"]" << "[20.1, 200.1]";
		}
		{
			const char *expression1 = "a = b + 1";
			const char *expression2 = "c + 2";
			const char *expression3 = "d = b + 3";
			const char *expression4 = "c + 4";
			QTest::newRow("mix of assignments and non-assignments") << QString("[\"%1\",\"%2\",\"%3\",\"%4\"]").arg(expression1).arg(expression2).arg(expression3).arg(expression4)
																	<< "[\"b\",\"c\"]" << "[10, 100]"
																	<< "[\"a\",\"result1\",\"d\",\"result2\"]" << "[11, 102, 13, 104]";
		}
	}
	void testCalculating()
	{
		QFETCH(QString, serializedExpressions);
		QFETCH(QString, serializedInputVariables);
		QFETCH(QString, serializedInputValues);
		QFETCH(QString, serializedOutputVariables);
		QFETCH(QString, serializedOutputValues);

		VuoList_VuoText expressions = VuoList_VuoText_makeFromString(serializedExpressions.toUtf8().data());
		VuoList_VuoText inputVariables = VuoList_VuoText_makeFromString(serializedInputVariables.toUtf8().data());
		VuoList_VuoReal inputValues = VuoList_VuoReal_makeFromString(serializedInputValues.toUtf8().data());
		VuoList_VuoText outputVariables = VuoList_VuoText_makeFromString(serializedOutputVariables.toUtf8().data());
		VuoList_VuoReal outputValues = VuoList_VuoReal_makeFromString(serializedOutputValues.toUtf8().data());

		VuoDictionary_VuoText_VuoReal inputVariablesAndValues = { inputVariables, inputValues };
		VuoDictionary_VuoText_VuoReal outputVariablesAndValues = { outputVariables, outputValues };

		VuoMathExpressionError error = NULL;
		VuoMathExpressionParser parser = VuoMathExpressionParser_makeFromMultipleExpressions(expressions, &error);
		QVERIFY(parser != NULL);
		VuoDictionary_VuoText_VuoReal actualOutputVariablesAndValues = VuoMathExpressionParser_calculate(parser, inputVariablesAndValues);
		QCOMPARE(QString::fromUtf8(VuoDictionary_VuoText_VuoReal_getString(actualOutputVariablesAndValues)),
				 QString::fromUtf8(VuoDictionary_VuoText_VuoReal_getString(outputVariablesAndValues)));
	}
};

QTEST_APPLESS_MAIN(TestVuoMathExpression)

#include "TestVuoMathExpression.moc"

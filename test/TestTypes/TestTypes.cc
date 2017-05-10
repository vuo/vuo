/**
 * @file
 * TestTypes implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "TestCompositionExecution.hh"
#include <Vuo/Vuo.h>

/**
 * Tests each type for common mistakes.
 */
class TestTypes : public TestCompositionExecution
{
	Q_OBJECT

private:
	VuoCompiler *compiler;

private slots:

	void initTestCase()
	{
		compiler = initCompiler();
	}

	void cleanupTestCase()
	{
		delete compiler;
	}

	/**
	 * Tests that each node class lists all necessary dependencies. If not, a composition that contains just that
	 * node class will fail to build.
	 *
	 * Tests that each generic node class successfully compiles when its generic port types are specialized.
	 */
	void testEachType_data()
	{
		QTest::addColumn<bool>("dummy");

		map<string, VuoCompilerType *> allTypes = compiler->getTypes();
		for (map<string, VuoCompilerType *>::iterator i = allTypes.begin(); i != allTypes.end(); ++i)
			QTest::newRow(i->first.c_str()) << true;
	}
	void testEachType()
	{
		printf("%s\n", QTest::currentDataTag()); fflush(stdout);

		string type = QTest::currentDataTag();
		string portIdentifier = "ShareValue__value";

		VuoRunner *runner = VuoCompiler::newCurrentProcessRunnerFromCompositionString(
			"digraph G { ShareValue [type=\"vuo.data.share." + type + "\"]; }", ".");
		runner->start();


		// Test whether basic type functions can handle all the top-level JSON types without crashing.
		json_object *jsonTypes[] = {
			NULL,
			json_object_new_boolean(false),
			json_object_new_boolean(true),
			json_object_new_double(42.42),
			json_object_new_int(42),
			json_object_new_object(),
			json_object_new_array(),
			json_object_new_string("")
		};
		for (int i = 0; i < sizeof(jsonTypes)/sizeof(json_object *); ++i)
		{
//			VLog("%s",json_object_to_json_string(jsonTypes[i]));
			// Tests `VuoType_makeFromJson()`.
			runner->setInputPortValue(portIdentifier, jsonTypes[i]);
			// Tests `VuoType_getJson()`.
			runner->getInputPortValue(portIdentifier);
			// Tests `VuoType_getSummary()`.
			runner->getInputPortSummary(portIdentifier);
		}


		runner->stop();
		delete runner;
	}
};

QTEST_APPLESS_MAIN(TestTypes)
#include "TestTypes.moc"

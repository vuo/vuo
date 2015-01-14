/**
 * @file
 * TestVuoCompilerComposition interface and implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "TestVuoCompiler.hh"
#include "VuoCompilerComposition.hh"

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(VuoComposition *);

/**
 * Tests for using compositions.
 */
class TestVuoCompilerComposition : public TestVuoCompiler
{
	Q_OBJECT

private slots:

	void initTestCase()
	{
		initCompiler();
	}

	void cleanupTestCase()
	{
		cleanupCompiler();
	}

	void testDiff_data()
	{
		QTest::addColumn<VuoComposition *>("oldComposition");
		QTest::addColumn<VuoComposition *>("newComposition");
		QTest::addColumn<QString>("expectedDiff");

		VuoNode *startNode = compiler->getNodeClass("vuo.event.fireOnStart")->newNode("FireOnStart1");
		VuoNode *roundNode = compiler->getNodeClass("vuo.math.round")->newNode("Round1");

		{
			VuoComposition *oldComposition = new VuoComposition();
			VuoComposition *newComposition = new VuoComposition();
			QString expectedDiff = "[]";
			QTest::newRow("empty composition -> empty composition") << oldComposition << newComposition << expectedDiff;
		}

		{
			VuoComposition *oldComposition = new VuoComposition();
			VuoComposition *newComposition = new VuoComposition();
			newComposition->addNode(startNode);
			QString expectedDiff = "[{\"add\":\"FireOnStart1\",\"value\":{\"nodeClass\":\"vuo.event.fireOnStart\"}}]";
			QTest::newRow("empty composition -> one node") << oldComposition << newComposition << expectedDiff;
		}

		{
			VuoComposition *oldComposition = new VuoComposition();
			oldComposition->addNode(startNode);
			VuoComposition *newComposition = new VuoComposition();
			QString expectedDiff = "[{\"remove\":\"FireOnStart1\"}]";
			QTest::newRow("one node -> empty composition") << oldComposition << newComposition << expectedDiff;
		}

		{
			VuoComposition *oldComposition = new VuoComposition();
			oldComposition->addNode(startNode);
			VuoComposition *newComposition = new VuoComposition();
			newComposition->addNode(roundNode);
			QString expectedDiff = "[{\"remove\":\"FireOnStart1\"},{\"add\":\"Round1\",\"value\":{\"nodeClass\":\"vuo.math.round\"}}]";
			QTest::newRow("remove a node and add a node") << oldComposition << newComposition << expectedDiff;
		}

		{
			VuoComposition *oldComposition = new VuoComposition();
			oldComposition->addNode(startNode);
			oldComposition->addNode(roundNode);
			VuoComposition *newComposition = new VuoComposition();
			newComposition->addNode(startNode);
			newComposition->addNode(roundNode);
			QString expectedDiff = "[]";
			QTest::newRow("keep the same nodes") << oldComposition << newComposition << expectedDiff;
		}
	}
	void testDiff()
	{
		QFETCH(VuoComposition *, oldComposition);
		QFETCH(VuoComposition *, newComposition);
		QFETCH(QString, expectedDiff);

		VuoCompilerComposition oldCompilerComposition(oldComposition, NULL);
		string oldCompositionSerialized = oldCompilerComposition.getGraphvizDeclaration();
		VuoCompilerComposition newCompilerComposition(newComposition, NULL);

		string actualDiff = newCompilerComposition.diffAgainstOlderComposition(oldCompositionSerialized, compiler);
		QCOMPARE(QString::fromStdString(actualDiff), expectedDiff);
	}

};

QTEST_APPLESS_MAIN(TestVuoCompilerComposition)
#include "TestVuoCompilerComposition.moc"

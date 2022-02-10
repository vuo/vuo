/**
 * @file
 * TestVuoTree implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoTree.h"
#include "VuoList_VuoTree.h"
#include "VuoUrlFetch.h"
}

/**
 * Convenient representation of a partial VuoTree — name and attributes only.
 */
typedef struct
{
	QString elementName;
	QStringList attributeNames;
	QStringList attributeValues;
} ElementAndAttributes;

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(VuoTree);
typedef QMap<QString, QString> QStringMap;
Q_DECLARE_METATYPE(QStringMap);
typedef QList<ElementAndAttributes> ElementAndAttributesList;
Q_DECLARE_METATYPE(ElementAndAttributesList);
Q_DECLARE_OPAQUE_POINTER(json_object *)
Q_DECLARE_METATYPE(json_object *)

/**
 * Tests the VuoTree type.
 */
class TestVuoTree : public QObject
{
	Q_OBJECT

private:
	VuoTree makeTreeFromFile(const char *path, bool isXml, bool includeWhitespaceInXml)
	{
		void *data = NULL;
		unsigned int dataLength = 0;
		VuoText pathT = VuoText_make(path);
		VuoLocal(pathT);
		VuoUrl_fetch(pathT, &data, &dataLength);
		VuoTree tree = (isXml ?
							VuoTree_makeFromXmlText((const char *)data, includeWhitespaceInXml) :
							VuoTree_makeFromJsonText((const char *)data));
		free(data);
		return tree;
	}

	VuoTree makeTreeFromXmlFile(const char *path, bool includeWhitespace)
	{
		return makeTreeFromFile(path, true, includeWhitespace);
	}

	VuoTree makeTreeFromJsonFile(const char *path)
	{
		return makeTreeFromFile(path, false, false);
	}

	void checkTreeName(VuoTree tree, QString expectedName)
	{
		VuoText actualName = VuoTree_getName(tree);
		QCOMPARE(QString::fromUtf8(actualName), expectedName);
		VuoRetain(actualName);
		VuoRelease(actualName);
	}

	void checkTreeContent(VuoTree tree, QString expectedContent, bool includeDescendants, bool simplifyWhitespace)
	{
		VuoText actualContent = VuoTree_getContent(tree, includeDescendants);
		QString actualContentAdjusted = QString::fromUtf8(actualContent);
		if (simplifyWhitespace)
			actualContentAdjusted = actualContentAdjusted.simplified();
		QCOMPARE(actualContentAdjusted, expectedContent);
		VuoRetain(actualContent);
		VuoRelease(actualContent);
	}

private slots:

	void testMakeAndGet_data()
	{
		QTest::addColumn<VuoTree>("tree");
		QTest::addColumn<json_object *>("jsonObjectToRelease");
		QTest::addColumn<QString>("name");
		QTest::addColumn<QStringMap>("attributes");
		QTest::addColumn<QString>("content");
		QTest::addColumn<QString>("contentIncludingDescendants");
		QTest::addColumn<int>("childCount");

		QMap<QString, QString> noAttributes;
		VuoDictionary_VuoText_VuoText noAttributesDict = VuoDictionaryCreate_VuoText_VuoText();
		json_object *noJson = nullptr;

		{
			json_object *o = json_tokener_parse(QUOTE({"xml":""}));
			QTest::newRow("empty XML string")	<< VuoTree_makeFromJson(o) << o
												<< "" << noAttributes << "" << "" << 0;
		}
		{
			json_object *o = json_tokener_parse(QUOTE({"json":""}));
			QTest::newRow("empty JSON string")	<< VuoTree_makeFromJson(o) << o
												<< "" << noAttributes << "" << "" << 0;
		}
		{
			QTest::newRow("empty tree, constructed")	<< VuoTree_makeEmpty() << noJson
														<< "" << noAttributes << "" << "" << 0;
		}
		{
			QTest::newRow("empty tree, parsed, XML")	<< VuoTree_makeFromXmlText("", false) << noJson
														<< "" << noAttributes << "" << "" << 0;
		}
		{
			QTest::newRow("empty tree, parsed, JSON")	<< VuoTree_makeFromJsonText("") << noJson
														<< "" << noAttributes << "" << "" << 0;
		}
		{
			VuoTree tree = VuoTree_makeEmpty();
			json_object *o = VuoTree_getInterprocessJson(tree);
			QTest::newRow("empty tree, interprocess")	<< VuoTree_makeFromJson(o) << o
														<< "" << noAttributes << "" << "" << 0;
			VuoTree_retain(tree);
			VuoTree_release(tree);
		}
		{
			QTest::newRow("tree with null ingredients")	<< VuoTree_make(NULL, noAttributesDict, NULL, NULL) << noJson
														<< "" << noAttributes << "" << "" << 0;
		}
		{
			VuoList_VuoTree children = VuoListCreate_VuoTree();
			QTest::newRow("tree with empty ingredients")	<< VuoTree_make("", noAttributesDict, "", children) << noJson
															<< "" << noAttributes << "" << "" << 0;
		}
		{
			QTest::newRow("leaf node, XML")		<< VuoTree_makeFromXmlText("<a>b</a>", false) << noJson
												<< "a" << noAttributes << "b" << "b" << 0;
		}
		{
			QTest::newRow("leaf node, JSON")	<< VuoTree_makeFromJsonText(QUOTE({"a":"b"})) << noJson
												<< "a" << noAttributes << "b" << "b" << 0;
		}
		{
			QTest::newRow("leaf node, JSON, non-XML-compliant key") << VuoTree_makeFromJsonText(QUOTE({"50":"#ffebee"})) << noJson
												<< "50" << noAttributes << "#ffebee" << "#ffebee" << 0;
		}
		{
			QTest::newRow("leaf node, constructed")	<< VuoTree_make("a", noAttributesDict, "b", NULL) << noJson
													<< "a" << noAttributes << "b" << "b" << 0;
		}
		{
			VuoTree tree = VuoTree_makeFromXmlText("<a>b</a>", false);
			json_object *o = VuoTree_getInterprocessJson(tree);
			QTest::newRow("leaf node, XML interprocess")	<< VuoTree_makeFromJson(o) << o
															<< "a" << noAttributes << "b" << "b" << 0;
			VuoTree_retain(tree);
			VuoTree_release(tree);
		}
		{
			VuoTree tree = VuoTree_makeFromJsonText(QUOTE({"a":"b"}));
			json_object *o = VuoTree_getInterprocessJson(tree);
			QTest::newRow("leaf node, JSON interprocess")	<< VuoTree_makeFromJson(o) << o
															<< "a" << noAttributes << "b" << "b" << 0;
			VuoTree_retain(tree);
			VuoTree_release(tree);
		}
		{
			QTest::newRow("parent node, XML")	<< VuoTree_makeFromXmlText("<patient><height>1.5</height><weight>45</weight></patient>", false) << noJson
												<< "patient" << noAttributes << "" << "1.545" << 2;
		}
		{
			QTest::newRow("parent node, JSON")	<< VuoTree_makeFromJsonText(QUOTE({"patient":{"height":1.5,"weight":45}})) << noJson
												<< "patient" << noAttributes << "" << "1.545" << 2;
		}
		{
			VuoTree height = VuoTree_make("height", noAttributesDict, "1.5", NULL);
			VuoTree weight = VuoTree_make("weight", noAttributesDict, "45", NULL);
			VuoList_VuoTree children = VuoListCreate_VuoTree();
			VuoListAppendValue_VuoTree(children, height);
			VuoListAppendValue_VuoTree(children, weight);
			QTest::newRow("parent node, constructed")	<< VuoTree_make("patient", noAttributesDict, "", children) << noJson
														<< "patient" << noAttributes << "" << "1.545" << 2;
		}
		{
			VuoTree tree = VuoTree_makeFromJsonText(QUOTE({"patient":{"height":1.5,"weight":45}}));
			json_object *o = VuoTree_getJson(tree);
			QTest::newRow("parent node, JSON intraprocess")	<< VuoTree_makeFromJson(o) << o
															<< "patient" << noAttributes << "" << "1.545" << 2;
		}
		{
			VuoTree height = VuoTree_make("height", noAttributesDict, "1.5", NULL);
			VuoTree weight = VuoTree_make("weight", noAttributesDict, "45", NULL);
			VuoList_VuoTree children = VuoListCreate_VuoTree();
			VuoListAppendValue_VuoTree(children, height);
			VuoListAppendValue_VuoTree(children, weight);
			VuoTree tree = VuoTree_make("patient", noAttributesDict, "", children);
			json_object *o = VuoTree_getJson(tree);
			QTest::newRow("parent node, constructed intraprocess")	<< VuoTree_makeFromJson(o) << o
																	<< "patient" << noAttributes << "" << "1.545" << 2;
		}
		{
			QTest::newRow("grandparent node, XML")	<< VuoTree_makeFromXmlText("<trip><destination>Helsinki</destination><transportation><train>VR</train><bus>HRT</bus></transportation></trip>", false) << noJson
													<< "trip" << noAttributes << "" << "HelsinkiVRHRT" << 2;
		}
		{
			QTest::newRow("grandparent node, JSON")	<< VuoTree_makeFromJsonText(QUOTE({"trip":{"destination":"Helsinki","transportation":{"train":"VR","bus":"HRT"}}})) << noJson
													<< "trip" << noAttributes << "" << "HelsinkiVRHRT" << 2;
		}
		{
			QMap<QString, QString> attributes;
			attributes["id"] = "greeting";
			attributes["class"] = "salutation";
			QTest::newRow("attributes, XML")	<< VuoTree_makeFromXmlText("<div id=\"greeting\" class=\"salutation\">Hello!</div>", false) << noJson
												<< "div" << attributes << "Hello!" << "Hello!" << 0;
		}
		{
			QTest::newRow("CDATA, XML")			<< VuoTree_makeFromXmlText("<definition><![CDATA[The < and > are angle brackets.]]></definition>", false) << noJson
												<< "definition" << noAttributes << "The < and > are angle brackets." << "The < and > are angle brackets." << 0;
		}
		{
			const char *xml = "<p>&lt; (less than), &gt; (greater than), &apos; (apostrophe), &quot; (quote), and &amp; (ampersand)</p>";
			const char *content = "< (less than), > (greater than), ' (apostrophe), \" (quote), and & (ampersand)";
			QTest::newRow("character entities, XML")	<< VuoTree_makeFromXmlText(xml, false) << noJson
														<< "p" << noAttributes << content << content << 0;
		}
		{
			QTest::newRow("no content, XML")	<< VuoTree_makeFromXmlText("<nickname></nickname>", false) << noJson
												<< "nickname" << noAttributes << "" << "" << 0;
		}
		{
			QTest::newRow("mixed content, XML")	<< VuoTree_makeFromXmlText("<p>The <strong>cat</strong> is <i>so very</i> sleepy.</p>", false) << noJson
												<< "p" << noAttributes << "The  is  sleepy." << "The cat is so very sleepy." << 2;
		}
		{
			QTest::newRow("mixed content, JSON")	<< VuoTree_makeFromJsonText(QUOTE({"p":["The ",{"strong":"cat"}," is ",{"i":"so very"}," sleepy."]})) << noJson
													<< "" << noAttributes << "" << "The cat is so very sleepy." << 5;
		}
		{
			QTest::newRow("children with same name, XML")	<< VuoTree_makeFromXmlText("<experiment><trial>heads</trial><trial>tails</trial><trial>tails</trial></experiment>", false) << noJson
															<< "experiment" << noAttributes << "" << "headstailstails" << 3;
		}
		{
			QTest::newRow("children with same name, JSON")	<< VuoTree_makeFromJsonText(QUOTE({"experiment":{"trial":["heads","tails","tails"]}})) << noJson
															<< "experiment" << noAttributes << "" << "headstailstails" << 3;
		}
		{
			QTest::newRow("no root, object, JSON")	<< VuoTree_makeFromJsonText(QUOTE({"first":1,"second":2})) << noJson
													<< "" << noAttributes << "" << "12" << 2;
		}
		{
			QTest::newRow("no root, array, JSON")	<< VuoTree_makeFromJsonText(QUOTE([1, 1, 2, 3, 5])) << noJson
													<< "" << noAttributes << "" << "11235" << 5;
		}
		{
			QTest::newRow("no root, string, JSON")	<< VuoTree_makeFromJsonText(QUOTE("solitary")) << noJson
													<< "" << noAttributes << "solitary" << "solitary" << 0;
		}
		{
			QTest::newRow("file with indentation, exclude whitespace, XML")	<< makeTreeFromXmlFile("resources/TestVuoTree.html", false) << noJson
																			<< "html" << noAttributes << "" << "My blog" << 1;
		}
		{
			QTest::newRow("file with indentation, include whitespace, XML")	<< makeTreeFromXmlFile("resources/TestVuoTree.html", true) << noJson
																			<< "html" << noAttributes << "\n\t\n" << "\n\t\n\t\tMy blog\n\t\n" << 1;
		}
		{
			QTest::newRow("duplicate object key, JSON")	<< VuoTree_makeFromJsonText(QUOTE({"a":"b","a":"c"})) << noJson
														<< "a" << noAttributes << "c" << "c" << 0;
		}
		{
			VuoTree b = VuoTree_make("b", noAttributesDict, "c", NULL);
			VuoList_VuoTree children = VuoListCreate_VuoTree();
			VuoListAppendValue_VuoTree(children, b);

			VuoDictionary_VuoText_VuoText attributesDict = VuoDictionaryCreate_VuoText_VuoText();
			VuoDictionarySetKeyValue_VuoText_VuoText(attributesDict, VuoText_make("d"), VuoText_make("e"));

			QMap<QString, QString> attributes;
			attributes["d"] = "e";

			QTest::newRow("no name, constructed") << VuoTree_make(NULL, attributesDict, "a", children) << noJson
												  << "" << attributes << "a" << "ac" << 1;
		}

		QTest::newRow("linebreaks in string content, JSON")
			<< VuoTree_makeFromJsonText(QUOTE({"Shader":{"renderpass":[{"code":"\/*\n\n*"}]}})) << noJson
			<< "Shader" << noAttributes << "" << "/*\n\n*" << 1;

		QTest::newRow("ampersand in string content, JSON")
			<< VuoTree_makeFromJsonText(QUOTE("if (a & b)")) << noJson
			<< "" << noAttributes << "if (a & b)" << "if (a & b)" << 0;

		QTest::newRow("ampersands in string content, JSON")
			<< VuoTree_makeFromJsonText(QUOTE("if (a && b)")) << noJson
			<< "" << noAttributes << "if (a && b)" << "if (a && b)" << 0;

		VuoDictionary_VuoText_VuoText_retain(noAttributesDict);
		VuoDictionary_VuoText_VuoText_release(noAttributesDict);
	}
	void testMakeAndGet()
	{
		QFETCH(VuoTree, tree);
		QFETCH(json_object *, jsonObjectToRelease);
		QFETCH(QString, name);
		QFETCH(QStringMap, attributes);
		QFETCH(QString, content);
		QFETCH(QString, contentIncludingDescendants);
		QFETCH(int, childCount);

		VuoTree_retain(tree);
		json_object_put(jsonObjectToRelease);

		checkTreeName(tree, name);
		checkTreeContent(tree, content, false, false);
		checkTreeContent(tree, contentIncludingDescendants, true, false);

		VuoDictionary_VuoText_VuoText actualAttributes = VuoTree_getAttributes(tree);
		unsigned long actualAttributeCount = VuoListGetCount_VuoText(actualAttributes.keys);
		for (unsigned long i = 1; i <= actualAttributeCount; ++i)
		{
			VuoText key = VuoListGetValue_VuoText(actualAttributes.keys, i);
			VuoText value = VuoListGetValue_VuoText(actualAttributes.values, i);
			QVERIFY2(attributes.contains(QString::fromUtf8(key)), key);
			QString expectedValue = attributes.take(QString::fromUtf8(key));
			QCOMPARE(QString::fromUtf8(value), expectedValue);
		}
		if (! attributes.isEmpty())
			QFAIL(QStringList(attributes.keys()).join(", ").toUtf8().constData());
		VuoDictionary_VuoText_VuoText_retain(actualAttributes);
		VuoDictionary_VuoText_VuoText_release(actualAttributes);

		VuoList_VuoTree children = VuoTree_getChildren(tree);
		QCOMPARE(VuoListGetCount_VuoTree(children), (unsigned long)childCount);
		VuoRetain(children);
		VuoRelease(children);

		VuoTree_release(tree);
	}

	void testParseAndSerialize_data()
	{
		QTest::addColumn<VuoTree>("tree");
		QTest::addColumn<QString>("xml");
		QTest::addColumn<QString>("json");
		QTest::addColumn<bool>("indent");

		VuoDictionary_VuoText_VuoText noAttributes = VuoDictionaryCreate_VuoText_VuoText();

		{
			QTest::newRow("empty tree")	<< VuoTree_makeEmpty() << "" << "" << false;
		}
		{
			const char *xml = "<a>b</a>";
			QTest::newRow("leaf node")	<< VuoTree_makeFromXmlText(xml, false) << xml << QUOTE({"a":"b"}) << false;
		}
		{
			const char *xml = "<a x=\"1\" y=\"2\">b</a>";
			QTest::newRow("leaf node with attributes")	<< VuoTree_makeFromXmlText(xml, false) << xml << QUOTE({"a":{"x":"1","y":"2","content":"b"}}) << false;
		}
		{
			const char *xml = "<c><d>e</d><f>g</f></c>";
			QTest::newRow("parent node")	<< VuoTree_makeFromXmlText(xml, false) << xml << QUOTE({"c":{"d":"e","f":"g"}}) << false;
		}
		{
			const char *xml = "<c x=\"1\"><d y=\"2\">e</d><f>g</f></c>";
			QTest::newRow("parent node with attributes")	<< VuoTree_makeFromXmlText(xml, false) << xml << QUOTE({"c":{"x":"1","d":{"y":"2","content":"e"},"f":"g"}}) << false;
		}
		{
			VuoTree d = VuoTree_make("d", noAttributes, "e", NULL);
			VuoTree f = VuoTree_make("f", noAttributes, "g", NULL);
			VuoList_VuoTree children = VuoListCreate_VuoTree();
			VuoListAppendValue_VuoTree(children, d);
			VuoListAppendValue_VuoTree(children, f);
			VuoTree c = VuoTree_make("c", noAttributes, "", children);
			QTest::newRow("parent node, constructed, children are document roots")	<< c << "<c><d>e</d><f>g</f></c>" << QUOTE({"c":{"d":"e","f":"g"}}) << false;
		}
		{
			VuoDictionary_VuoText_VuoText noAttributes = VuoDictionaryCreate_VuoText_VuoText();
			VuoTree tree = VuoTree_makeFromXmlText("<c><d>e</d><f>g</f></c>", false);
			VuoList_VuoTree children = VuoTree_getChildren(tree);
			VuoTree c = VuoTree_make("c", noAttributes, "", children);
			QTest::newRow("parent node, constructed, children are not document roots")	<< c << "<c><d>e</d><f>g</f></c>" << QUOTE({"c":{"d":"e","f":"g"}}) << false;
		}
		{
			VuoDictionary_VuoText_VuoText da = VuoDictionaryCreate_VuoText_VuoText();
			VuoDictionarySetKeyValue_VuoText_VuoText(da, VuoText_make("y"), VuoText_make("2"));
			VuoTree d = VuoTree_make("d", da, "e", NULL);
			VuoDictionary_VuoText_VuoText fa = VuoDictionaryCreate_VuoText_VuoText();
			VuoTree f = VuoTree_make("f", fa, "g", NULL);
			VuoList_VuoTree children = VuoListCreate_VuoTree();
			VuoListAppendValue_VuoTree(children, d);
			VuoListAppendValue_VuoTree(children, f);
			VuoDictionary_VuoText_VuoText ca = VuoDictionaryCreate_VuoText_VuoText();
			VuoDictionarySetKeyValue_VuoText_VuoText(ca, VuoText_make("x"), VuoText_make("1"));
			VuoTree c = VuoTree_make("c", ca, "", children);
			QTest::newRow("parent node with attributes, constructed")	<< c << "<c x=\"1\"><d y=\"2\">e</d><f>g</f></c>" << QUOTE({"c":{"x":"1","d":{"y":"2","content":"e"},"f":"g"}}) << false;
			VuoDictionary_VuoText_VuoText_retain(ca);
			VuoDictionary_VuoText_VuoText_release(ca);
			VuoDictionary_VuoText_VuoText_retain(da);
			VuoDictionary_VuoText_VuoText_release(da);
		}
		{
			const char *xml = "<H><I><J>K</J></I><L>M</L></H>";
			QTest::newRow("grandparent node")	<< VuoTree_makeFromXmlText(xml, false) << xml << QUOTE({"H":{"I":{"J":"K"},"L":"M"}}) << false;
		}
		{
			QTest::newRow("no content")	<< VuoTree_makeFromXmlText("<p></p>", false) << "<p/>" << QUOTE({"p":null}) << false;
		}
		{
			VuoTree tree = VuoTree_make("", noAttributes, "nameless", NULL);
			QTest::newRow("leaf node with no name") << tree << "<document>nameless</document>" << QUOTE({"document":"nameless"}) << false;
		}
		{
			VuoTree child = VuoTree_make("", noAttributes, "nameless", NULL);
			VuoList_VuoTree children = VuoListCreate_VuoTree();
			VuoListAppendValue_VuoTree(children, child);
			VuoTree tree = VuoTree_make("named", noAttributes, "", children);
			QTest::newRow("child node with no name") << tree << "<named><item>nameless</item></named>" << QUOTE({"named":{"item":"nameless"}}) << false;
		}
		{
			const char *xml = "<P>p1<Q>q1</Q>p2<R>r1</R></P>";
			QTest::newRow("mixed content") << VuoTree_makeFromXmlText(xml, false) << xml << QUOTE({"P":["p1",{"Q":"q1"},"p2",{"R":"r1"}]}) << false;
		}
		{
			const char *json = QUOTE({"A":"a","B":"b"});
			const char *xml = "<document><A>a</A><B>b</B></document>";
			QTest::newRow("multiple keys in top-level JSON object") << VuoTree_makeFromJsonText(json) << xml << json << false;
		}
		{
			const char *json = QUOTE(1);
			const char *xml = "<document>1</document>";
			QTest::newRow("JSON integer") << VuoTree_makeFromJsonText(json) << xml << json << false;
		}
		{
			const char *json = QUOTE([1,2]);
			const char *xml = "<document><item>1</item><item>2</item></document>";
			QTest::newRow("JSON array of integers") << VuoTree_makeFromJsonText(json) << xml << json << false;
		}
		{
			const char *xml = "<P><Q>q1</Q><R>r1</R><Q>q2</Q><R>r2</R></P>";
			QTest::newRow("children with same name")	<< VuoTree_makeFromXmlText(xml, false) << xml << QUOTE({"P":{"Q":["q1","q2"],"R":["r1","r2"]}}) << false;
		}
		{
			VuoTree q1 = VuoTree_make("Q", noAttributes, "q1", NULL);
			VuoTree q2 = VuoTree_make("Q", noAttributes, "q2", NULL);
			VuoTree r1 = VuoTree_make("R", noAttributes, "r1", NULL);
			VuoTree r2 = VuoTree_make("R", noAttributes, "r2", NULL);
			VuoList_VuoTree children = VuoListCreate_VuoTree();
			VuoListAppendValue_VuoTree(children, q1);
			VuoListAppendValue_VuoTree(children, r1);
			VuoListAppendValue_VuoTree(children, q2);
			VuoListAppendValue_VuoTree(children, r2);
			VuoTree p = VuoTree_make("P", noAttributes, "", children);
			QTest::newRow("children with same name, constructed")	<< p << "<P><Q>q1</Q><R>r1</R><Q>q2</Q><R>r2</R></P>" << QUOTE({"P":{"Q":["q1","q2"],"R":["r1","r2"]}}) << false;
		}
		{
			const char *xml = "<W><X><Y>y1</Y><Y>y2</Y></X><X><Y>y3</Y><Y>y4</Y><Z>z</Z></X></W>";
			QTest::newRow("children with same name and groups of grandchildren with same name") << VuoTree_makeFromXmlText(xml, false) << xml << QUOTE({"W":{"X":[{"Y":["y1","y2"]},{"Y":["y3","y4"],"Z":"z"}]}}) << false;
		}
		{
			const char *xml = "<W><X><Y>y1</Y></X><X><Y>y2</Y></X></W>";
			QTest::newRow("children with same name and single grandchildren with same name")	<< VuoTree_makeFromXmlText(xml, false) << xml << QUOTE({"W":{"X":[{"Y":"y1"},{"Y":"y2"}]}}) << false;
		}
		{
			const char *xml = "<W><W><W>w1</W></W><W><W>w2</W></W></W>";
			QTest::newRow("children and grandchildren with same name")	<< VuoTree_makeFromXmlText(xml, false) << xml << QUOTE({"W":{"W":[{"W":"w1"},{"W":"w2"}]}}) << false;
		}
		{
			const char *xml = "<P Q=\"q1\"><Q>q2</Q><R>r</R></P>";
			QTest::newRow("children and attributes with same name")	<< VuoTree_makeFromXmlText(xml, false) << xml << QUOTE({"P":{"Q":["q1","q2"],"R":"r"}}) << false;
		}
		{
			const char *xml = "<P>p1<Q>q1</Q>p2<Q>q2</Q>p3</P>";
			QTest::newRow("separated children with same name in mixed content") << VuoTree_makeFromXmlText(xml, false) << xml << QUOTE({"P":["p1",{"Q":"q1"},"p2",{"Q":"q2"},"p3"]}) << false;
		}
		{
			const char *xml = "<P>p1<Q>q1</Q><R>r1</R><Q>q2</Q>p2</P>";
			QTest::newRow("grouped children with same name in mixed content") << VuoTree_makeFromXmlText(xml, false) << xml << QUOTE({"P":["p1",{"Q":["q1","q2"]},{"R":"r1"},"p2"]}) << false;
			// Maybe a better translation would be {"P":["p1",{"Q":["q1","q2"],"R":"r1"},"p2"]}, but this is easier and it's a rare case.
		}
		{
			QTest::newRow("self-closing XML tag")	<< VuoTree_makeFromXmlText("<br />", false) << "<br/>" << QUOTE({"br":null}) << false;
		}
		{
			const char *xml = "<X><![CDATA[<Y>y</Y>]]><Z>z</Z></X>";
			QTest::newRow("CDATA, XML")	<< VuoTree_makeFromXmlText(xml, false) << xml << QUOTE({"X":["<Y>y<\/Y>",{"Z":"z"}]}) << false;
		}
		{
			// "libxml2 enforces the conversion of the predefined entities where necessary to prevent well-formedness problems" (http://www.xmlsoft.org/entities.html)
			const char *xml = "<p>&lt; (less than), &gt; (greater than), &apos; (apostrophe), &quot; (quote), and &amp; (ampersand)</p>";
			const char *xml2 = "<p>&lt; (less than), &gt; (greater than), ' (apostrophe), \" (quote), and &amp; (ampersand)</p>";
			const char *json = QUOTE({"p":"< (less than), > (greater than), ' (apostrophe), \" (quote), and & (ampersand)"});
			QTest::newRow("character entities, XML")	<< VuoTree_makeFromXmlText(xml, false) << xml2 << json << false;
		}
		{
			const char *xml = "<item label=\"Which word do you use: &quot;soda&quot; or &quot;pop&quot;?\">pop</item>";
			const char *json = QUOTE({"item":{"label":"Which word do you use: \"soda\" or \"pop\"?","content":"pop"}});
			QTest::newRow("quote character entity, XML")	<< VuoTree_makeFromXmlText(xml, false) << xml << json << false;
		}
		{
			// libxml2 outputs attributes with double-quotes, so doesn't need to escape single-quotes in them.
			const char *xml = "<item label='Pet&apos;s name'>Finkelfarb</item>";
			const char *xml2 = "<item label=\"Pet's name\">Finkelfarb</item>";
			const char *json = QUOTE({"item":{"label":"Pet's name","content":"Finkelfarb"}});
			QTest::newRow("apostrophe character entity, XML")	<< VuoTree_makeFromXmlText(xml, false) << xml2 << json << false;
		}
		{
			const char *json = QUOTE({"files":{"/etc/crontab":"configuration file","~/.bashrc":"shell script","My Pictures":"cat photos","passwords2.txt":"sequel to passwords1","50":"#ffebee"}});
			const char *json2 = QUOTE({"files":{"\/etc\/crontab":"configuration file","~\/.bashrc":"shell script","My Pictures":"cat photos","passwords2.txt":"sequel to passwords1","50":"#ffebee"}});
			const char *xml = "<files><item name=\"/etc/crontab\">configuration file</item><item name=\"~/.bashrc\">shell script</item><item name=\"My Pictures\">cat photos</item><passwords2.txt>sequel to passwords1</passwords2.txt><item name=\"50\">#ffebee</item></files>";
			QTest::newRow("names valid in JSON but invalid in XML") << VuoTree_makeFromJsonText(json) << xml << json2 << false;
		}
		{
			const char *xml = "<CaSe_SenSiTive>Welcome to Ohio.</CaSe_SenSiTive>";
			const char *json = QUOTE({"CaSe_SenSiTive":"Welcome to Ohio."});
			QTest::newRow("case sensitive, XML")	<< VuoTree_makeFromXmlText(xml, false) << xml << json << false;
			QTest::newRow("case sensitive, JSON")	<< VuoTree_makeFromJsonText(json) << xml << json << false;
		}
		{
			const char *xml = "<numbers>⓪①②③④⑤⑥⑦⑧⑨⑩</numbers>";
			const char *json = QUOTE({"numbers":"⓪①②③④⑤⑥⑦⑧⑨⑩"});
			QTest::newRow("UTF-8 content, XML")		<< VuoTree_makeFromXmlText(xml, false) << xml << json << false;
			QTest::newRow("UTF-8 content, JSON")	<< VuoTree_makeFromJsonText(json) << xml << json << false;
		}
		{
			const char *xml = "<汉字> </汉字>";
			const char *json = QUOTE({"汉字":" "});
			QTest::newRow("UTF-8 name, XML")	<< VuoTree_makeFromXmlText(xml, false) << xml << json << false;
			QTest::newRow("UTF-8 name, JSON")	<< VuoTree_makeFromJsonText(json) << xml << json << false;
		}
		{
			const char *json = QUOTE({"html":{"body":{"h1":"My blog"}}});
			const char *xml = "<html><body><h1>My blog</h1></body></html>";
			QTest::newRow("parsed without indentation, serialized without indentation, XML") << makeTreeFromXmlFile("resources/TestVuoTree.html", false) << QString(xml) << json << false;
		}
		{
			const char *json = QUOTE({"html":["",{"body":["",{"h1":"My blog"},""]},""]});
			const char *xml = "<html>\n\t<body>\n\t\t<h1>My blog</h1>\n\t</body>\n</html>";
			QTest::newRow("parsed with indentation, serialized without indentation, XML") << makeTreeFromXmlFile("resources/TestVuoTree.html", true) << QString(xml) << json << false;
		}
		{
			const char *json = "{\n  \"html\":{\n    \"body\":{\n      \"h1\":\"My blog\"\n    }\n  }\n}";
			const char *xml = "<html>\n  <body>\n    <h1>My blog</h1>\n  </body>\n</html>";
			QTest::newRow("parsed without indentation, serialized with indentation, XML") << makeTreeFromXmlFile("resources/TestVuoTree.html", false) << QString(xml) << json << true;
		}
		{
			const char *json = "{\n  \"html\":[\n    \"\",\n    {\n      \"body\":[\n        \"\",\n        {\n          \"h1\":\"My blog\"\n        },\n        \"\"\n      ]\n    },\n    \"\"\n  ]\n}";
			const char *xml = "<html>\n\t<body>\n\t\t<h1>My blog</h1>\n\t</body>\n</html>";  // xmlNodeDump only adds indentation if it's not there already, apparently.
			QTest::newRow("parsed with indentation, serialized with indentation, XML") << makeTreeFromXmlFile("resources/TestVuoTree.html", true) << QString(xml) << json << true;
		}
		{
			const char *json = QUOTE({"user":{"name":"sniffy04","mail":"feedme@snakes.org","hobbies":["eating firsts","eating seconds"]}});
			const char *xml = "<user><name>sniffy04</name><mail>feedme@snakes.org</mail><hobbies>eating firsts</hobbies><hobbies>eating seconds</hobbies></user>";
			QTest::newRow("serialized without indentation, JSON")	<< makeTreeFromJsonFile("resources/TestVuoTree.json") << QString(xml) << json << false;
		}
		{
			const char *json = "{\n  \"user\":{\n    \"name\":\"sniffy04\",\n    \"mail\":\"feedme@snakes.org\",\n    \"hobbies\":[\n      \"eating firsts\",\n      \"eating seconds\"\n    ]\n  }\n}";
			const char *xml = "<user>\n  <name>sniffy04</name>\n  <mail>feedme@snakes.org</mail>\n  <hobbies>eating firsts</hobbies>\n  <hobbies>eating seconds</hobbies>\n</user>";
			QTest::newRow("serialized with indentation, JSON")	<< makeTreeFromJsonFile("resources/TestVuoTree.json") << QString(xml) << json << true;
		}
		{
			VuoTree r = VuoTree_make("R", noAttributes, "r", NULL);
			VuoTree s = VuoTree_makeFromXmlText("<S><T>t</T></S>", false);
			VuoList_VuoTree grandchildren = VuoListCreate_VuoTree();
			VuoListAppendValue_VuoTree(grandchildren, r);
			VuoListAppendValue_VuoTree(grandchildren, s);

			VuoTree q = VuoTree_make("Q", noAttributes, "", grandchildren);
			VuoList_VuoTree children = VuoListCreate_VuoTree();
			VuoListAppendValue_VuoTree(children, q);

			VuoTree p = VuoTree_make("P", noAttributes, "", children);

			const char *json = "{\n  \"P\":{\n    \"Q\":{\n      \"R\":\"r\",\n      \"S\":{\n        \"T\":\"t\"\n      }\n    }\n  }\n}";
			const char *xml = "<P>\n  <Q>\n    <R>r</R>\n    <S>\n      <T>t</T>\n    </S>\n  </Q>\n</P>";
			QTest::newRow("serialized with indentation, constructed")	<< p << xml << json << true;
		}

		// Invalid XML/JSON — don't crash.
		{
			const char *xml = "<b></b><c></c>";
			QTest::newRow("no root, XML")				<< VuoTree_makeFromXmlText(xml, false) << "" << "" << false;
		}
		{
			const char *xml = "<1></1>";
			QTest::newRow("invalid element name, XML")	<< VuoTree_makeFromXmlText(xml, false) << "" << "" << false;
		}
		{
			const char *xml = "<d e=\\\"a\\\" e=\\\"b\\\"></d>";
			QTest::newRow("duplicate attribute, XML")	<< VuoTree_makeFromXmlText(xml, false) << "" << "" << false;
		}
		{
			const char *json = "{\"a\":\"b\"";
			QTest::newRow("no closing brace, JSON")		<< VuoTree_makeFromJsonText(json) << "" << "" << false;
		}
		{
			const char *json = QUOTE({"a":flase});
			QTest::newRow("misspelled literal, JSON")	<< VuoTree_makeFromJsonText(json) << "" << "" << false;
		}

		QTest::newRow("XML property list, root array, empty") << makeTreeFromXmlFile("resources/TestVuoTree-plist-rootArray-empty.plist", false)
			<< "<document/>"
			<< VUO_STRINGIFY({"document":null})
			<< false;

		QTest::newRow("XML property list, root array, simple") << makeTreeFromXmlFile("resources/TestVuoTree-plist-rootArray-simple.plist", false)
			<< "<document><item>0</item><item>1</item></document>"
			<< VUO_STRINGIFY({"document":{"item":["0","1"]}})
			<< false;

		QTest::newRow("XML property list, root dict, empty") << makeTreeFromXmlFile("resources/TestVuoTree-plist-rootDict-empty.plist", false)
			<< "<document/>"
			<< VUO_STRINGIFY({"document":null})
			<< false;

		QTest::newRow("XML property list, root dict, all types") << makeTreeFromXmlFile("resources/TestVuoTree-plist-rootDict-allTypes.plist", false)
			<< "<document><a>aa</a><b><b0>b0a</b0><b1>b1a</b1></b><c><item>0</item><item>1</item></c><d>false</d><e>true</e><f>QUJDRA==</f><g>2017-05-12T00:46:00Z</g><h>42</h><i>22.42</i></document>"
			<< VUO_STRINGIFY({"document":{"a":"aa","b":{"b0":"b0a","b1":"b1a"},"c":{"item":["0","1"]},"d":"false","e":"true","f":"QUJDRA==","g":"2017-05-12T00:46:00Z","h":"42","i":"22.42"}})
			<< false;

		QTest::newRow("XML property list, root dict, app") << makeTreeFromXmlFile("resources/TestVuoTree-plist-rootDict-app.plist", false)
			<< "<document><CFBundleIconFile>vuo.icns</CFBundleIconFile><CFBundlePackageType>APPL</CFBundlePackageType><CFBundleExecutable>Vuo</CFBundleExecutable><CFBundleIdentifier>org.vuo.editor</CFBundleIdentifier><CFBundleVersion>1.3.0</CFBundleVersion><CFBundleShortVersionString>1.3.0</CFBundleShortVersionString><LSMinimumSystemVersion>10.8</LSMinimumSystemVersion><NSPrincipalClass>NSApplication</NSPrincipalClass><NSHighResolutionCapable>true</NSHighResolutionCapable><CFBundleDocumentTypes><item><LSIsAppleDefaultForType>true</LSIsAppleDefaultForType><CFBundleTypeRole>Viewer</CFBundleTypeRole><CFBundleTypeName>Vuo Node</CFBundleTypeName><LSItemContentTypes><item>org.vuo.node</item></LSItemContentTypes><CFBundleTypeMIMETypes><item>application/x-vuo-node</item></CFBundleTypeMIMETypes><CFBundleTypeIconFile>vuo-node.icns</CFBundleTypeIconFile><CFBundleTypeExtensions><item>vuonode</item></CFBundleTypeExtensions></item><item><LSIsAppleDefaultForType>true</LSIsAppleDefaultForType><CFBundleTypeRole>Viewer</CFBundleTypeRole><CFBundleTypeName>Vuo Premium Node</CFBundleTypeName><LSItemContentTypes><item>org.vuo.node.premium</item></LSItemContentTypes><CFBundleTypeMIMETypes><item>application/x-vuo-node+</item></CFBundleTypeMIMETypes><CFBundleTypeIconFile>vuo-node.icns</CFBundleTypeIconFile><CFBundleTypeExtensions><item>vuonode+</item></CFBundleTypeExtensions></item><item><LSIsAppleDefaultForType>true</LSIsAppleDefaultForType><CFBundleTypeRole>Editor</CFBundleTypeRole><CFBundleTypeName>Vuo Composition</CFBundleTypeName><LSItemContentTypes><item>org.vuo.composition</item></LSItemContentTypes><CFBundleTypeMIMETypes><item>application/x-vuo</item></CFBundleTypeMIMETypes><CFBundleTypeIconFile>vuo-composition.icns</CFBundleTypeIconFile><CFBundleTypeExtensions><item>vuo</item></CFBundleTypeExtensions></item></CFBundleDocumentTypes><UTExportedTypeDeclarations><item><UTTypeConformsTo><item>public.data</item></UTTypeConformsTo><UTTypeDescription>Vuo Node</UTTypeDescription><UTTypeIdentifier>org.vuo.node</UTTypeIdentifier><UTTypeReferenceURL>https://vuo.org</UTTypeReferenceURL><UTTypeTagSpecification><public.filename-extension><item>vuonode</item></public.filename-extension><public.mime-type><item>application/x-vuo-node</item></public.mime-type></UTTypeTagSpecification></item><item><UTTypeConformsTo><item>public.data</item></UTTypeConformsTo><UTTypeDescription>Vuo Premium Node</UTTypeDescription><UTTypeIdentifier>org.vuo.node.premium</UTTypeIdentifier><UTTypeReferenceURL>https://vuo.org</UTTypeReferenceURL><UTTypeTagSpecification><public.filename-extension><item>vuonode+</item></public.filename-extension><public.mime-type><item>application/x-vuo-node+</item></public.mime-type></UTTypeTagSpecification></item><item><UTTypeConformsTo><item>public.data</item></UTTypeConformsTo><UTTypeDescription>Vuo Composition</UTTypeDescription><UTTypeIdentifier>org.vuo.composition</UTTypeIdentifier><UTTypeReferenceURL>https://vuo.org</UTTypeReferenceURL><UTTypeTagSpecification><public.filename-extension><item>vuo</item></public.filename-extension><public.mime-type><item>application/x-vuo</item></public.mime-type></UTTypeTagSpecification></item></UTExportedTypeDeclarations><CFBundleURLTypes><item><CFBundleURLName>Vuo License</CFBundleURLName><CFBundleURLSchemes><item>vuo-license</item></CFBundleURLSchemes></item><item><CFBundleURLName>Vuo Example Composition</CFBundleURLName><CFBundleURLSchemes><item>vuo-example</item></CFBundleURLSchemes></item></CFBundleURLTypes></document>"
			<< VUO_STRINGIFY({"document":{"CFBundleIconFile":"vuo.icns","CFBundlePackageType":"APPL","CFBundleExecutable":"Vuo","CFBundleIdentifier":"org.vuo.editor","CFBundleVersion":"1.3.0","CFBundleShortVersionString":"1.3.0","LSMinimumSystemVersion":"10.8","NSPrincipalClass":"NSApplication","NSHighResolutionCapable":"true","CFBundleDocumentTypes":{"item":[{"LSIsAppleDefaultForType":"true","CFBundleTypeRole":"Viewer","CFBundleTypeName":"Vuo Node","LSItemContentTypes":{"item":"org.vuo.node"},"CFBundleTypeMIMETypes":{"item":"application\/x-vuo-node"},"CFBundleTypeIconFile":"vuo-node.icns","CFBundleTypeExtensions":{"item":"vuonode"}},{"LSIsAppleDefaultForType":"true","CFBundleTypeRole":"Viewer","CFBundleTypeName":"Vuo Premium Node","LSItemContentTypes":{"item":"org.vuo.node.premium"},"CFBundleTypeMIMETypes":{"item":"application\/x-vuo-node+"},"CFBundleTypeIconFile":"vuo-node.icns","CFBundleTypeExtensions":{"item":"vuonode+"}},{"LSIsAppleDefaultForType":"true","CFBundleTypeRole":"Editor","CFBundleTypeName":"Vuo Composition","LSItemContentTypes":{"item":"org.vuo.composition"},"CFBundleTypeMIMETypes":{"item":"application\/x-vuo"},"CFBundleTypeIconFile":"vuo-composition.icns","CFBundleTypeExtensions":{"item":"vuo"}}]},"UTExportedTypeDeclarations":{"item":[{"UTTypeConformsTo":{"item":"public.data"},"UTTypeDescription":"Vuo Node","UTTypeIdentifier":"org.vuo.node","UTTypeReferenceURL":"https:\/\/vuo.org","UTTypeTagSpecification":{"public.filename-extension":{"item":"vuonode"},"public.mime-type":{"item":"application\/x-vuo-node"}}},{"UTTypeConformsTo":{"item":"public.data"},"UTTypeDescription":"Vuo Premium Node","UTTypeIdentifier":"org.vuo.node.premium","UTTypeReferenceURL":"https:\/\/vuo.org","UTTypeTagSpecification":{"public.filename-extension":{"item":"vuonode+"},"public.mime-type":{"item":"application\/x-vuo-node+"}}},{"UTTypeConformsTo":{"item":"public.data"},"UTTypeDescription":"Vuo Composition","UTTypeIdentifier":"org.vuo.composition","UTTypeReferenceURL":"https:\/\/vuo.org","UTTypeTagSpecification":{"public.filename-extension":{"item":"vuo"},"public.mime-type":{"item":"application\/x-vuo"}}}]},"CFBundleURLTypes":{"item":[{"CFBundleURLName":"Vuo License","CFBundleURLSchemes":{"item":"vuo-license"}},{"CFBundleURLName":"Vuo Example Composition","CFBundleURLSchemes":{"item":"vuo-example"}}]}}})
			<< false;

		// Don't crash.
		QTest::newRow("XML property list, malformed") << makeTreeFromXmlFile("resources/TestVuoTree-plist-rootDict-malformed.plist", false)
			<< ""
			<< VUO_STRINGIFY()
			<< false;

		// Don't crash.
		QTest::newRow("XML property list, malformed2") << makeTreeFromXmlFile("resources/TestVuoTree-plist-rootDict-malformed2.plist", false)
			<< ""
			<< VUO_STRINGIFY()
			<< false;

		VuoDictionary_VuoText_VuoText_retain(noAttributes);
		VuoDictionary_VuoText_VuoText_release(noAttributes);
	}
	void testParseAndSerialize()
	{
		QFETCH(VuoTree, tree);
		QFETCH(QString, xml);
		QFETCH(QString, json);
		QFETCH(bool, indent);

		VuoText actualXml = VuoTree_serializeAsXml(tree, indent);
//		VLog("expected=%s",xml.toUtf8().data());
//		VLog("actual  =%s",actualXml.toUtf8().data());
		QCOMPARE(QString::fromUtf8(actualXml), xml);
		VuoRetain(actualXml);
		VuoRelease(actualXml);

		VuoText actualJson = VuoTree_serializeAsJson(tree, indent);
//		VLog("expected=%s",json.toUtf8().data());
//		VLog("actual  =%s",actualJson.toUtf8().data());
		QCOMPARE(QString::fromUtf8(actualJson), json);
		VuoRetain(actualJson);
		VuoRelease(actualJson);

		VuoTree_retain(tree);
		VuoTree_release(tree);
	}

	void testGetAttribute_data()
	{
		QTest::addColumn<VuoTree>("tree");
		QTest::addColumn<QString>("key");
		QTest::addColumn<QString>("value");

		QTest::newRow("one of several attributes")	<< VuoTree_makeFromXmlText("<thing id=\"1\" type=\"2\" kind=\"3\"></thing>", false)
													<< "type" << "2";

		QTest::newRow("non-existent attribute")		<< VuoTree_makeFromXmlText("<thing id=\"1\" type=\"2\" kind=\"3\"></thing>", false)
													<< "ilk" << "";
	}
	void testGetAttribute()
	{
		QFETCH(VuoTree, tree);
		QFETCH(QString, key);
		QFETCH(QString, value);

		VuoText actualValue = VuoTree_getAttribute(tree, key.toUtf8().constData());
		QCOMPARE(QString::fromUtf8(actualValue), value);
		VuoRetain(actualValue);
		VuoRelease(actualValue);

		VuoTree_retain(tree);
		VuoTree_release(tree);
	}

	void testFindElementsUsingXpath_data()
	{
		QTest::addColumn<VuoTree>("tree");
		QTest::addColumn<QString>("xpath");
		QTest::addColumn<QStringList>("names");
		QTest::addColumn<QStringList>("contents");

		QStringList emptyList;

		VuoTree books = makeTreeFromXmlFile("resources/TestVuoTree_inventory.xml", false);
		VuoList_VuoTree booksChildren = VuoTree_getChildren(books);
		VuoTree bookChild = VuoListGetValue_VuoTree(booksChildren, 1);

		{
			VuoTree emptyTree = VuoTree_makeEmpty();
			QTest::newRow("empty tree") << emptyTree << "/bookstore" << emptyList << emptyList;
			VuoTree_retain(emptyTree);
		}
		{
			QTest::newRow("empty path") << books << "" << emptyList << emptyList;
			VuoTree_retain(books);
		}
		{
			QStringList names;
			names << "book" << "book" << "book";
			QStringList contents;
			contents << "" << "" << "";

			QTest::newRow("absolute path") << books << "/bookstore/book" << names << contents;
			VuoTree_retain(books);

			QTest::newRow("relative path, implicit") << books << "book" << names << contents;
			VuoTree_retain(books);

			QTest::newRow("relative path, explicit") << books << "./book" << names << contents;
			VuoTree_retain(books);
		}
		{
			QStringList names("first-name");
			QStringList contents("Joe");

			QTest::newRow("absolute path in child sharing XML doc") << bookChild << "/book/author/first-name" << names << contents;
			VuoTree_retain(bookChild);

			QTest::newRow("relative path in child sharing XML doc") << bookChild << "author/first-name" << names << contents;
			VuoTree_retain(bookChild);
		}
		{
			QStringList names("first-name");
			QStringList contents("Joe");

			VuoList_VuoTree children = VuoListCreate_VuoTree();
			VuoListAppendValue_VuoTree(children, bookChild);
			VuoDictionary_VuoText_VuoText noAttributes = VuoDictionaryCreate_VuoText_VuoText();
			VuoTree bookcase = VuoTree_make("bookcase", noAttributes, "", children);
			VuoDictionary_VuoText_VuoText_retain(noAttributes);
			VuoDictionary_VuoText_VuoText_release(noAttributes);

			QTest::newRow("absolute path in constructed tree") << bookcase << "/bookcase/book/author/first-name" << names << contents;
			VuoTree_retain(bookcase);

			QTest::newRow("relative path in constructed tree") << bookcase << "book/author/first-name" << names << contents;
			VuoTree_retain(bookcase);
		}
		{
			QStringList names;
			names << "first-name" << "first-name" << "first-name" << "first-name" << "first-name";
			QStringList contents;
			contents << "Joe" << "Mary" << "Mary" << "Britney" << "Toni";

			QTest::newRow("recursive descent, beginning of absolute path") << books << "//first-name" << names << contents;
			VuoTree_retain(books);

			QTest::newRow("recursive descent, beginning of relative path") << books << ".//first-name" << names << contents;
			VuoTree_retain(books);

			QTest::newRow("recursive descent, middle of absolute path") << books << "/bookstore//first-name" << names << contents;
			VuoTree_retain(books);
		}
		{
			QStringList names("bookstore");
			QStringList contents("");

			QTest::newRow("wildcard, root") << books << "/*" << names << contents;
			VuoTree_retain(books);
		}
		{
			QStringList names;
			names << "first-name" << "last-name";
			QStringList contents;
			contents << "Mary" << "Bob";

			QTest::newRow("wildcard, end of path") << books << "//publication/*" << names << contents;
			VuoTree_retain(books);
		}
		{
			QStringList names;
			names << "price" << "price" << "price" << "price";
			QStringList contents;
			contents << "12" << "55" << "2.50" << "6.50";

			QTest::newRow("wildcard, middle of path") << books << "/bookstore/*/price" << names << contents;
			VuoTree_retain(books);
		}
		{
			QTest::newRow("parent outside of tree") << books << ".." << emptyList << emptyList;
			VuoTree_retain(books);
		}
		{
			QTest::newRow("parent outside of child") << bookChild << ".." << emptyList << emptyList;
			VuoTree_retain(books);
		}
		{
			QStringList names("magazine");
			QStringList contents("");

			QTest::newRow("parent within tree") << books << "/bookstore/book/../magazine" << names << contents;
			VuoTree_retain(books);
		}
		{
			QStringList names("first-name");
			QStringList contents("Joe");

			QTest::newRow("index, first") << books << "book[1]/author/first-name" << names << contents;
			VuoTree_retain(books);
		}
		{
			QStringList names("first-name");
			QStringList contents("Toni");

			QTest::newRow("index, last") << books << "book[last()]/author/first-name" << names << contents;
			VuoTree_retain(books);
		}
		{
			QStringList names("degree");
			QStringList contents("B.A.");

			QTest::newRow("index, multiple levels") << books << "book[3]/author/degree[1]" << names << contents;
			VuoTree_retain(books);
		}
		{
			QStringList names;
			names << "price" << "price" << "price" << "price";
			QStringList contents;
			contents << "12" << "55" << "2.50" << "6.50";

			QTest::newRow("index, multiple results") << books << "//price[1]" << names << contents;
			VuoTree_retain(books);
		}
		{
			QStringList names("price");
			QStringList contents("55");

			QTest::newRow("index, after evaluating rest of path") << books << "(//price)[2]" << names << contents;
			VuoTree_retain(books);
		}
		{
			QTest::newRow("index, out of bounds") << books << "book[99]" << emptyList << emptyList;
			VuoTree_retain(books);
		}
		{
			QStringList names;
			names << "first-name" << "first-name";
			QStringList contents;
			contents << "Joe" << "Toni";

			QTest::newRow("predicate, child element exists") << books << "//author[award]/first-name" << names << contents;
			VuoTree_retain(books);
		}
		{
			QStringList names("price");
			QStringList contents("6.50");

			QTest::newRow("predicate, attribute equals") << books << "book[@style=\"novel\"]/price" << names << contents;
			VuoTree_retain(books);
		}
		{
			QStringList names;
			names << "price" << "price";
			QStringList contents;
			contents << "2.50" << "6.50";

			QTest::newRow("predicate, content less than") << books << "//price[. < 10]" << names << contents;
			VuoTree_retain(books);
		}
		{
			QStringList names;
			names << "first-name" << "first-name";
			QStringList contents;
			contents << "Joe" << "Toni";

			QTest::newRow("predicate, boolean operators") << books << "book/author[award or degree and publication]/first-name" << names << contents;
			VuoTree_retain(books);
		}
		{
			QStringList names("first-name");
			QStringList contents("Toni");

			QTest::newRow("predicate, boolean operators with parentheses") << books << "book/author[(award or degree) and publication]/first-name" << names << contents;
			VuoTree_retain(books);
		}
		{
			QStringList names;
			names << "degree" << "degree";
			QStringList contents;
			contents << "B.A." << "Ph.D.";

			QTest::newRow("text nodes, unique elements") << books << "book/author/degree/text()" << names << contents;
			VuoTree_retain(books);
		}
		{
			QStringList names;
			names << "p" << "p";
			QStringList contents;
			contents << "It was a dark and stormy night." << "But then all nights in Trenton seem dark and stormy to someone who has gone through what have.";

			QTest::newRow("text nodes, repeated elements") << books << "book/excerpt/p/text()" << names << contents;
			VuoTree_retain(books);
		}
		{
			VuoTree tree = VuoTree_makeFromXmlText("<facts><math><![CDATA[1 < 2]]></math><math><![CDATA[2 > 1]]></math></facts>", false);

			QStringList names;
			names << "math" << "math";
			QStringList contents;
			contents << "1 < 2" << "2 > 1";

			QTest::newRow("CDATA, unique elements") << tree << "math/text()" << names << contents;
			VuoTree_retain(tree);
		}
		{
			VuoTree tree = VuoTree_makeFromXmlText("<facts><information>The <![CDATA[<em>]]> and <![CDATA[<i>]]> tags both typically make italics.</information></facts>", false);

			QStringList names("information");
			QStringList contents("The <em> and <i> tags both typically make italics.");

			QTest::newRow("CDATA, repeated elements") << tree << "information/text()" << names << contents;
			VuoTree_retain(tree);
		}
		{
			VuoTree tree = VuoTree_makeFromXmlText("<facts><math>1 &lt; 2</math><math>true &amp;&amp; false = false</math></facts>", false);

			QStringList names;
			names << "math" << "math";
			QStringList contents;
			contents << "1 < 2" << "true && false = false";

			QTest::newRow("character entities in XML") << tree << "math[contains(., '1 < 2') or contains(., 'true && false')]" << names << contents;
			VuoTree_retain(tree);
		}
		{
			QStringList names("price");
			QStringList contents("55");

			QTest::newRow("smartquotes, double quotes") << books << "book[@style=“textbook”]/price" << names << contents;
//			QTest::newRow("smartquotes, double quotes") << books << "book[@style=\"textbook\"]/price" << names << contents;
			VuoTree_retain(books);
		}
		{
			QStringList names("first-name");
			QStringList contents("Britney");

			QTest::newRow("smartquotes, single quotes") << books << "//first-name[contains(., ‘Brit’)]" << names << contents;
//			QTest::newRow("smartquotes, single quotes") << books << "//first-name[contains(., 'Brit')]" << names << contents;
			VuoTree_retain(books);
		}

		VuoRetain(booksChildren);
		VuoRelease(booksChildren);
	}
	void testFindElementsUsingXpath()
	{
		QFETCH(VuoTree, tree);
		QFETCH(QString, xpath);
		QFETCH(QStringList, names);
		QFETCH(QStringList, contents);

		VuoList_VuoTree foundTrees = VuoTree_findItemsUsingXpath(tree, xpath.toUtf8().constData());

		unsigned long count = VuoListGetCount_VuoTree(foundTrees);
		for (unsigned long i = 1; i <= count && i-1 < names.size(); ++i)
		{
			VuoTree foundTree = VuoListGetValue_VuoTree(foundTrees, i);

			checkTreeName(foundTree, names[i-1]);
			checkTreeContent(foundTree, contents[i-1], false, true);
		}

		QCOMPARE(count, (unsigned long)names.size());

		VuoRetain(foundTrees);
		VuoRelease(foundTrees);
		VuoTree_release(tree);
	}

	void testFindAttributesUsingXpath_data()
	{
		QTest::addColumn<VuoTree>("tree");
		QTest::addColumn<QString>("xpath");
		QTest::addColumn<ElementAndAttributesList>("elements");

		VuoTree books = makeTreeFromXmlFile("resources/TestVuoTree_inventory.xml", false);

		{
			QList<ElementAndAttributes> elements;
			elements.append( (ElementAndAttributes){ QString("book"), QStringList("style"), QStringList("autobiography") } );
			elements.append( (ElementAndAttributes){ QString("book"), QStringList("style"), QStringList("textbook") } );
			elements.append( (ElementAndAttributes){ QString("book"), QStringList("style"), QStringList("novel") } );

			QTest::newRow("single attribute") << books << "book/@style" << elements;
			VuoTree_retain(books);
		}
		{
			QStringList names;
			names << "intl" << "exchange";
			QStringList values;
			values << "Canada" << "0.7";
			QList<ElementAndAttributes> elements;
			elements.append( (ElementAndAttributes){ QString("price"), names, values } );

			QTest::newRow("multiple attributes, single element") << books << "//price/@*" << elements;
			VuoTree_retain(books);
		}
		{
			QList<ElementAndAttributes> elements;
			elements.append( (ElementAndAttributes){ QString("book"), QStringList("style"), QStringList("autobiography") } );
			elements.append( (ElementAndAttributes){ QString("book"), QStringList("style"), QStringList("textbook") } );
			elements.append( (ElementAndAttributes){ QString("book"), QStringList("style"), QStringList("novel") } );
			elements[2].attributeNames.append("id");
			elements[2].attributeValues.append("myfave");

			QTest::newRow("multiple attributes, multiple elements") << books << "book/@*" << elements;
			VuoTree_retain(books);
		}
	}
	void testFindAttributesUsingXpath()
	{
		QFETCH(VuoTree, tree);
		QFETCH(QString, xpath);
		QFETCH(ElementAndAttributesList, elements);

		VuoList_VuoTree foundTrees = VuoTree_findItemsUsingXpath(tree, xpath.toUtf8().constData());

		unsigned long count = VuoListGetCount_VuoTree(foundTrees);
		for (unsigned long i = 1; i <= count && i-1 < elements.size(); ++i)
		{
			VuoTree foundTree = VuoListGetValue_VuoTree(foundTrees, i);
			ElementAndAttributes element = elements[i-1];

			checkTreeName(foundTree, element.elementName);

			VuoDictionary_VuoText_VuoText actualAttributes = VuoTree_getAttributes(foundTree);
			unsigned long attributeCount = VuoListGetCount_VuoText(actualAttributes.keys);
			for (unsigned long j = 1; j <= attributeCount && j-1 < element.attributeNames.size(); ++j)
			{
				VuoText attributeName = VuoListGetValue_VuoText(actualAttributes.keys, j);
				QCOMPARE(QString::fromUtf8(attributeName), element.attributeNames[j-1]);
				VuoText attributeValue = VuoListGetValue_VuoText(actualAttributes.values, j);
				QCOMPARE(QString::fromUtf8(attributeValue), element.attributeValues[j-1]);
			}
			VuoDictionary_VuoText_VuoText_retain(actualAttributes);
			VuoDictionary_VuoText_VuoText_release(actualAttributes);

			QCOMPARE(attributeCount, (unsigned long)element.attributeNames.size());
		}

		QCOMPARE(count, (unsigned long)elements.size());

		VuoRetain(foundTrees);
		VuoRelease(foundTrees);
		VuoTree_release(tree);
	}

	void testFindItemsWithName_data()
	{
		QTest::addColumn<VuoTree>("tree");
		QTest::addColumn<QString>("name");
		QTest::addColumn<bool>("caseSensitive");
		QTest::addColumn<bool>("includeDescendants");
		QTest::addColumn<QStringList>("names");
		QTest::addColumn<QStringList>("contents");

		QStringList emptyList;
		VuoDictionary_VuoText_VuoText noAttributes = VuoDictionaryCreate_VuoText_VuoText();

		VuoTree books = makeTreeFromXmlFile("resources/TestVuoTree_inventory.xml", false);

		{
			VuoTree tree = VuoTree_makeEmpty();
			QTest::newRow("empty tree") << tree << "hi" << true << true << emptyList << emptyList;
			VuoTree_retain(tree);
		}
		{
			QTest::newRow("empty name") << books << "" << true << true << emptyList << emptyList;
			VuoTree_retain(books);
		}
		{
			QStringList names("a");
			QStringList contents("b");

			VuoTree tree = VuoTree_makeFromXmlText("<a>b</a>", false);
			QTest::newRow("leaf node, parsed tree") << tree << "a" << true << false << names << contents;
			VuoTree_retain(tree);
		}
		{
			QStringList names("a");
			QStringList contents("b");

			VuoTree tree = VuoTree_make("a", noAttributes, "b", NULL);
			QTest::newRow("leaf node, constructed tree") << tree << "a" << true << false << names << contents;
			VuoTree_retain(tree);
		}
		{
			QStringList names("magazine");
			QStringList contents("2.50");
			QTest::newRow("child matches (case-sensitive), parsed tree") << books << "magazine" << true << false << names << contents;
			VuoTree_retain(books);

			QTest::newRow("child matches (not case-sensitive), parsed tree") << books << "MAGAZINE" << false << false << names << contents;
			VuoTree_retain(books);
		}
		{
			VuoTree tree = VuoTree_makeFromXmlText("<z><y>x</y><y>x</y>x</z>", false);
			QTest::newRow("no child matches, parsed tree") << tree << "x" << true << false << emptyList << emptyList;
			VuoTree_retain(tree);
		}
		{
			VuoTree y1 = VuoTree_make("y", noAttributes, "x1", NULL);
			VuoTree y2 = VuoTree_make("y", noAttributes, "x2", NULL);
			VuoList_VuoTree yy = VuoListCreate_VuoTree();
			VuoListAppendValue_VuoTree(yy, y1);
			VuoListAppendValue_VuoTree(yy, y2);
			VuoTree z = VuoTree_make("z", noAttributes, "", yy);

			QStringList names;
			names << "y" << "y";
			QStringList contents;
			contents << "x1" << "x2";

			QTest::newRow("child matches, constructed tree") << z << "y" << true << false << names << contents;
			VuoTree_retain(z);

			QTest::newRow("no child matches, constructed tree") << z << "Y" << true << false << emptyList << emptyList;
			VuoTree_retain(z);
		}
		{
			VuoList_VuoTree bookstoreChildren = VuoTree_getChildren(books);
			VuoTree book2 = VuoListGetValue_VuoTree(bookstoreChildren, 2);
			VuoList_VuoTree bookChildren = VuoTree_getChildren(book2);
			VuoTree book2author1 = VuoListGetValue_VuoTree(bookChildren, 1);

			QStringList names("first-name");
			QStringList contents("Mary");

			QTest::newRow("multiple levels match but descendants excluded, parsed tree") << book2author1 << "First-name" << false << false << names << contents;
			VuoTree_retain(book2author1);

			names << "first-name";
			contents << "Mary";

			QTest::newRow("multiple levels match, parsed tree") << book2author1 << "First-name" << false << true << names << contents;
			VuoTree_retain(book2author1);

			VuoRetain(bookChildren);
			VuoRelease(bookChildren);
			VuoRetain(bookstoreChildren);
			VuoRelease(bookstoreChildren);
		}
		{
			// <I><J>0<J>1</J></J><K><J>2</J></K></I>
			VuoTree j1 = VuoTree_make("J", noAttributes, "1", NULL);
			VuoList_VuoTree j1List = VuoListCreate_VuoTree();
			VuoListAppendValue_VuoTree(j1List, j1);
			VuoTree j0 = VuoTree_make("J", noAttributes, "0", j1List);
			VuoTree j2 = VuoTree_make("J", noAttributes, "2", NULL);
			VuoList_VuoTree j2List = VuoListCreate_VuoTree();
			VuoListAppendValue_VuoTree(j2List, j2);
			VuoTree kj2 = VuoTree_make("K", noAttributes, "", j2List);
			VuoList_VuoTree jk = VuoListCreate_VuoTree();
			VuoListAppendValue_VuoTree(jk, j0);
			VuoListAppendValue_VuoTree(jk, kj2);
			VuoTree i = VuoTree_make("I", noAttributes, "", jk);

			QStringList names("J");
			QStringList contents("01");

			QTest::newRow("multiple levels match but descendants excluded, constructed tree") << i << "j" << false << false << names << contents;
			VuoTree_retain(i);

			names << "J" << "J";
			contents << "1" << "2";

			QTest::newRow("multiple levels match, constructed tree") << i << "j" << false << true << names << contents;
			VuoTree_retain(i);
		}
		{
			VuoList_VuoTree bookstoreChildren = VuoTree_getChildren(books);
			VuoTree bookChild = VuoListGetValue_VuoTree(bookstoreChildren, 1);

			QTest::newRow("match in parent tree but not child") << bookChild << "bookstore" << true << true << emptyList << emptyList;
			VuoTree_retain(bookChild);

			VuoRetain(bookstoreChildren);
			VuoRelease(bookstoreChildren);
		}

		VuoDictionary_VuoText_VuoText_retain(noAttributes);
		VuoDictionary_VuoText_VuoText_release(noAttributes);
	}
	void testFindItemsWithName()
	{
		QFETCH(VuoTree, tree);
		QFETCH(QString, name);
		QFETCH(bool, caseSensitive);
		QFETCH(bool, includeDescendants);
		QFETCH(QStringList, names);
		QFETCH(QStringList, contents);

		VuoTextComparison comparison = {VuoTextComparison_Equals, caseSensitive};
		VuoList_VuoTree foundTrees = VuoTree_findItemsWithName(tree, name.toUtf8().constData(), comparison, includeDescendants);

		unsigned long count = VuoListGetCount_VuoTree(foundTrees);
		for (unsigned long i = 1; i <= count && i < names.size(); ++i)
		{
			VuoTree foundTree = VuoListGetValue_VuoTree(foundTrees, i);
			checkTreeName(foundTree, names[i-1]);
			checkTreeContent(foundTree, contents[i-1], true, true);
		}

		QCOMPARE(count, (unsigned long)names.size());

		VuoRetain(foundTrees);
		VuoRelease(foundTrees);
		VuoTree_release(tree);
	}

	void testFindItemsWithAttribute_data()
	{
		QTest::addColumn<VuoTree>("tree");
		QTest::addColumn<QString>("attribute");
		QTest::addColumn<QString>("value");
		QTest::addColumn<bool>("includeDescendants");
		QTest::addColumn<QStringList>("names");

		const char *xml = "<root type=\"a root\"><child type=\"a child\" id=\"kiddo\"><grand id=\"grand\" type=\"a grand\" /></child></root>";
		VuoTree tree = VuoTree_makeFromXmlText(xml, false);

		{
			QTest::newRow("attribute doesn't exist") << tree << "root" << "a" << true << QStringList();
			VuoTree_retain(tree);
		}
		{
			QTest::newRow("value doesn't match") << tree << "type" << "a b" << true << QStringList();
			VuoTree_retain(tree);
		}
		{
			QStringList names("child");
			QTest::newRow("value matches") << tree << "id" << "kiddo" << false << names;
			VuoTree_retain(tree);
		}
		{
			QStringList names;
			names << "root" << "child" << "grand";
			QTest::newRow("value matches in descendants") << tree << "type" << "a" << true << names;
			VuoTree_retain(tree);
		}
		{
			QStringList names;
			names << "root" << "child";
			QTest::newRow("empty value") << tree << "id" << "" << false << names;
			VuoTree_retain(tree);
		}
	}
	void testFindItemsWithAttribute()
	{
		QFETCH(VuoTree, tree);
		QFETCH(QString, attribute);
		QFETCH(QString, value);
		QFETCH(bool, includeDescendants);
		QFETCH(QStringList, names);

		VuoTextComparison comparison = {VuoTextComparison_BeginsWith, true};
		VuoList_VuoTree foundTrees = VuoTree_findItemsWithAttribute(tree, attribute.toUtf8().constData(), value.toUtf8().constData(),
																	comparison, includeDescendants);

		unsigned long count = VuoListGetCount_VuoTree(foundTrees);
		for (unsigned long i = 1; i <= count && i < names.size(); ++i)
		{
			VuoTree foundTree = VuoListGetValue_VuoTree(foundTrees, i);
			checkTreeName(foundTree, names[i-1]);
		}

		QCOMPARE(count, (unsigned long)names.size());

		VuoRetain(foundTrees);
		VuoRelease(foundTrees);
		VuoTree_release(tree);
	}

	void testFindItemsWithContent_data()
	{
		QTest::addColumn<VuoTree>("tree");
		QTest::addColumn<QString>("content");
		QTest::addColumn<bool>("includeDescendants");
		QTest::addColumn<QStringList>("names");

		const char *json = QUOTE({"root":{"child1":"123","child2":{"grand1":"456","grand2":"123"},"child3":"789789"}});
		VuoTree tree = VuoTree_makeFromJsonText(json);

		{
			QTest::newRow("no match") << tree << "0123" << false << QStringList();
			VuoTree_retain(tree);
		}
		{
			QStringList names("child1");
			QTest::newRow("child matches") << tree << "123" << false << names;
			VuoTree_retain(tree);
		}
		{
			QStringList names("child3");
			QTest::newRow("child matches multiple times") << tree << "789" << false << names;
			VuoTree_retain(tree);
		}
		{
			QTest::newRow("grandchild matches but descendants excluded") << tree << "456" << false << QStringList();
			VuoTree_retain(tree);
		}
		{
			QStringList names("grand1");
			QTest::newRow("grandchild matches and descendants included") << tree << "456" << true << names;
			VuoTree_retain(tree);
		}
		{
			const char *xml = "<p>Perhaps <i>you</i> should mind <i>your</i> own business, saucy youth.</p>";
			VuoTree p = VuoTree_makeFromXmlText(xml, false);

			QStringList names;
			names << "p" << "i" << "i";

			QTest::newRow("match in mixed content") << p << "you" << false << names;
			VuoTree_retain(p);
		}
		{
			const char *xml = "<root><child1>123</child1><child2><![CDATA[456]]></child2></root>";
			VuoTree tree = VuoTree_makeFromXmlText(xml, false);

			QStringList names("child2");

			QTest::newRow("match in CDATA") << tree << "456" << true << names;
			VuoTree_retain(tree);
		}
	}
	void testFindItemsWithContent()
	{
		QFETCH(VuoTree, tree);
		QFETCH(QString, content);
		QFETCH(bool, includeDescendants);
		QFETCH(QStringList, names);

		VuoTextComparison comparison = {VuoTextComparison_Contains, true};
		VuoList_VuoTree foundTrees = VuoTree_findItemsWithContent(tree, content.toUtf8().constData(), comparison, includeDescendants);

		unsigned long count = VuoListGetCount_VuoTree(foundTrees);
		for (unsigned long i = 1; i <= count && i < names.size(); ++i)
		{
			VuoTree foundTree = VuoListGetValue_VuoTree(foundTrees, i);
			checkTreeName(foundTree, names[i-1]);
		}

		QCOMPARE(count, (unsigned long)names.size());

		VuoRetain(foundTrees);
		VuoRelease(foundTrees);
		VuoTree_release(tree);
	}

	void testMultithreading_data()
	{
		QTest::addColumn<int>("testIndex");

		int i = 0;
		QTest::newRow("parse XML") << i++;
		QTest::newRow("serialize to XML") << i++;
		QTest::newRow("serialize to JSON") << i++;
		QTest::newRow("getters") << i++;
		QTest::newRow("find Xpath") << i++;
		QTest::newRow("find content") << i++;
	}
	void testMultithreading()
	{
		QFETCH(int, testIndex);

		void *data = NULL;
		unsigned int dataLength = 0;
		VuoText filePath = VuoText_make("resources/TestVuoTree_inventory.xml");
		VuoLocal(filePath);
		VuoUrl_fetch(filePath, &data, &dataLength);
		VuoDefer(^{ free(data); });

		dispatch_queue_t concurrentQueue = dispatch_queue_create("org.vuo.TestVuoTree.concurrent", DISPATCH_QUEUE_CONCURRENT);
		dispatch_queue_t serialQueue = dispatch_queue_create("org.vuo.TestVuoTree.serial", DISPATCH_QUEUE_SERIAL);
		dispatch_group_t group = dispatch_group_create();

		const int numIterations = 200;

		// Create some trees, some with a parent-child relationship (same xmlDoc) and some unrelated.
		const int numTrees = 8;
		VuoTree trees[numTrees];
		VuoTree *treesPtr = trees;
		for (int i = 0; i < numTrees/2; ++i)
		{
			trees[i] = VuoTree_makeFromXmlText((const char *)data, false);

			VuoTree_retain(trees[i]);

			int j = numTrees/2 + i;
			VuoList_VuoTree children = VuoTree_getChildren(trees[i]);
			trees[j] = VuoListGetValue_VuoTree(children, 1);

			VuoTree_retain(trees[j]);
			VuoRetain(children);
			VuoRelease(children);
		}

		// Assign a randomly chosen tree to each iteration.
		size_t treeForIteration[numIterations];
		size_t *treeForIterationPtr = treeForIteration;
		for (int i = 0; i < numIterations; ++i)
			treeForIteration[i] = VuoInteger_random(0, numTrees-1);

		if (testIndex == 0)  // parse XML
		{
			VuoTree parsedTrees[numIterations];
			VuoTree *parsedTreesPtr = parsedTrees;

			for (int i = 0; i < numIterations; ++i)
			{
				dispatch_group_async(group, concurrentQueue, ^{
										 parsedTreesPtr[i] = VuoTree_makeFromXmlText((const char *)data, false);
									 });
			}
			dispatch_group_wait(group, DISPATCH_TIME_FOREVER);

			VuoTree expectedTree = VuoTree_makeFromXmlText((const char *)data, false);
			VuoText expectedXml = VuoTree_serializeAsXml(expectedTree, false);

			for (int i = 0; i < numIterations; ++i)
			{
				VuoTree tree = parsedTrees[i];
				VuoText xml = VuoTree_serializeAsXml(tree, false);
				QCOMPARE(QString::fromUtf8(xml), QString::fromUtf8(expectedXml));

				VuoRetain(xml);
				VuoRelease(xml);
				VuoTree_retain(tree);
				VuoTree_release(tree);
			}

			VuoTree_retain(expectedTree);
			VuoTree_release(expectedTree);
			VuoRetain(expectedXml);
			VuoRelease(expectedXml);
		}
		else if (testIndex == 1)  // serialize to XML
		{
			VuoText results[2][numIterations];

			for (int h = 0; h < 2; ++h)
			{
				dispatch_queue_t queue = (h == 0 ? concurrentQueue : serialQueue);
				VuoText *resultsPtr = results[h];

				for (int i = 0; i < numIterations; ++i)
				{
					dispatch_group_async(group, queue, ^{
											 VuoTree tree = treesPtr[ treeForIterationPtr[i] ];
											 resultsPtr[i] = VuoTree_serializeAsXml(tree, false);
										 });
				}
				dispatch_group_wait(group, DISPATCH_TIME_FOREVER);
			}

			for (int i = 0; i < numIterations; ++i)
			{
				QCOMPARE(QString::fromUtf8(results[0][i]), QString::fromUtf8(results[1][i]));

				for (int h = 0; h < 2; ++h)
				{
					VuoRetain(results[h][i]);
					VuoRelease(results[h][i]);
				}
			}
		}
		else if (testIndex == 2)  // serialize to JSON
		{
			VuoText results[2][numIterations];

			for (int h = 0; h < 2; ++h)
			{
				dispatch_queue_t queue = (h == 0 ? concurrentQueue : serialQueue);
				VuoText *resultsPtr = results[h];

				for (int i = 0; i < numIterations; ++i)
				{
					dispatch_group_async(group, queue, ^{
											 VuoTree tree = treesPtr[ treeForIterationPtr[i] ];
											 resultsPtr[i] = VuoTree_serializeAsJson(tree, false);
										 });
				}
				dispatch_group_wait(group, DISPATCH_TIME_FOREVER);
			}

			for (int i = 0; i < numIterations; ++i)
			{
				QCOMPARE(QString::fromUtf8(results[0][i]), QString::fromUtf8(results[1][i]));

				for (int h = 0; h < 2; ++h)
				{
					VuoRetain(results[h][i]);
					VuoRelease(results[h][i]);
				}
			}
		}
		else if (testIndex == 3)  // getters
		{
			char *summaries[2][numIterations];
			VuoText contents[2][numIterations];

			for (int h = 0; h < 2; ++h)
			{
				dispatch_queue_t queue = (h == 0 ? concurrentQueue : serialQueue);
				char **summariesPtr = summaries[h];
				VuoText *contentsPtr = contents[h];

				for (int i = 0; i < numIterations; ++i)
				{
					dispatch_group_async(group, queue, ^{
											 VuoTree tree = treesPtr[ treeForIterationPtr[i] ];
											 summariesPtr[i] = VuoTree_getSummary(tree);
											 contentsPtr[i] = VuoTree_getContent(tree, true);
										 });
				}
				dispatch_group_wait(group, DISPATCH_TIME_FOREVER);
			}

			for (int i = 0; i < numIterations; ++i)
			{
				QCOMPARE(QString::fromUtf8(summaries[0][i]), QString::fromUtf8(summaries[1][i]));
				QCOMPARE(QString::fromUtf8(contents[0][i]), QString::fromUtf8(contents[1][i]));

				for (int h = 0; h < 2; ++h)
				{
					free(summaries[h][i]);
					VuoRetain(contents[h][i]);
					VuoRelease(contents[h][i]);
				}
			}
		}
		else if (testIndex == 4)  // find XPath
		{
			size_t results[2][numIterations];
			const char *xpath = "//first-name";

			for (int h = 0; h < 2; ++h)
			{
				dispatch_queue_t queue = (h == 0 ? concurrentQueue : serialQueue);
				size_t *resultsPtr = results[h];

				for (int i = 0; i < numIterations; ++i)
				{
					dispatch_group_async(group, queue, ^{
											 VuoTree tree = treesPtr[ treeForIterationPtr[i] ];
											 VuoList_VuoTree foundTrees = VuoTree_findItemsUsingXpath(tree, xpath);
											 resultsPtr[i] = VuoListGetCount_VuoTree(foundTrees);

											 VuoRetain(foundTrees);
											 VuoRelease(foundTrees);
										 });
				}
				dispatch_group_wait(group, DISPATCH_TIME_FOREVER);
			}

			for (int i = 0; i < numIterations; ++i)
				QCOMPARE(results[0][i], results[1][i]);
		}
		else if (testIndex == 5)  // find content
		{
			size_t results[2][numIterations];
			const char *content = "Bob";
			VuoTextComparison comparison = {VuoTextComparison_Equals, true};

			for (int h = 0; h < 2; ++h)
			{
				dispatch_queue_t queue = (h == 0 ? concurrentQueue : serialQueue);
				size_t *resultsPtr = results[h];

				for (int i = 0; i < numIterations; ++i)
				{
					dispatch_group_async(group, queue, ^{
											 VuoTree tree = treesPtr[ treeForIterationPtr[i] ];
											 VuoList_VuoTree foundTrees = VuoTree_findItemsWithContent(tree, content, comparison, true);
											 resultsPtr[i] = VuoListGetCount_VuoTree(foundTrees);

											 VuoRetain(foundTrees);
											 VuoRelease(foundTrees);
										 });
				}
				dispatch_group_wait(group, DISPATCH_TIME_FOREVER);
			}

			for (int i = 0; i < numIterations; ++i)
				QCOMPARE(results[0][i], results[1][i]);
		}
		else
			QFAIL("testIndex out of bounds");

		for (int i = 0; i < numTrees; ++i)
			VuoTree_release(trees[i]);

		dispatch_release(concurrentQueue);
		dispatch_release(serialQueue);
		dispatch_release(group);
	}
};

QTEST_APPLESS_MAIN(TestVuoTree)

#include "TestVuoTree.moc"

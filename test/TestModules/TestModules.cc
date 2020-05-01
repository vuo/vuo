/**
 * @file
 * TestModules interface and implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Winvalid-constexpr"
#include <QtTest/QtTest>
#pragma clang diagnostic pop

#include <Vuo/Vuo.h>

#include <wjelement/wjelement.h>
#include <wjelement/wjreader.h>

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(VuoCompilerType *);
Q_DECLARE_METATYPE(VuoCompilerModule *);
Q_DECLARE_METATYPE(VuoCompilerNodeClass *);

/**
 * Tests each module for common mistakes.
 */
class TestModules : public QObject
{
	Q_OBJECT

private:
	VuoCompiler *compiler;
	WJElement VuoType;
	WJElement VuoLibrary;
	WJElement VuoNode;
	WJElement VuoInputData;
	WJElement VuoInputEvent;
	WJElement VuoOutputData;
	WJElement VuoOutputEvent;
	WJElement VuoOutputTrigger;

	void loadSchema(const char *filename, WJElement &schema)
	{
		FILE *schemafile = fopen(filename, "r");
		if (!schemafile)
			QFAIL((QString("Couldn't read ") + filename).toUtf8().constData());

		WJReader readschema = WJROpenFILEDocument(schemafile, NULL, 0);
		if (!readschema)
			QFAIL((QString("Couldn't create JSON reader for ") + filename).toUtf8().constData());

		schema = WJEOpenDocument(readschema, NULL, NULL, NULL);
		if (!schema)
			QFAIL((QString("Couldn't open ") + filename).toUtf8().constData());
		fclose(schemafile);
		WJRCloseDocument(readschema);

		// WJEDump(schema);

		QVERIFY2(!readschema->depth, (QString("Couldn't parse ") + filename).toUtf8().constData());
	}

	static void logError(void *client, const char *format, ...)
	{
		va_list ap;
		va_start(ap, format);
		QFAIL((QString((char *)client) + ": " + QString::vasprintf(format, ap)).toUtf8().constData());
		va_end(ap);
	}

	#define validate(schema, jsonString, explanation) validateF(schema, jsonString, #schema, explanation)
	bool validateF(WJElement &schema, const char *jsonString, QString schemaString, QString explanation)
	{
		// It's OK for port details to be null or empty.
		if (QString(jsonString) == "null"
		 || QString(jsonString) == "{}"
		 || QString(jsonString) == "{ }")
		{
			if (schemaString == "VuoInputEvent"
			 || schemaString == "VuoInputData"
			 || schemaString == "VuoOutputEvent"
			 || schemaString == "VuoOutputData"
			 || schemaString == "VuoOutputTrigger")
				return true;
		}


		WJReader readjson = WJROpenMemDocument((char *)jsonString, NULL, 0);
		if (!readjson)
		{
			VUserLog("Couldn't create JSON reader");
			return false;
		}
		if (readjson->depth)
		{
			VUserLog("Couldn't parse JSON");
			return false;
		}

		WJElement json = WJEOpenDocument(readjson, NULL, NULL, NULL);
		if (!json)
		{
			VUserLog("Couldn't open JSON");
			return false;
		}

		// WJEDump(json);

		bool ret = WJESchemaValidate(schema, json, logError, NULL, NULL, (void *)(schemaString + ": " + explanation).toUtf8().constData());

		WJECloseDocument(json);
		WJRCloseDocument(readjson);

		return ret;
	}


public:
	TestModules()
	{
		compiler = new VuoCompiler();

		loadSchema("schema/VuoType.jsonschema",          VuoType);
		loadSchema("schema/VuoLibrary.jsonschema",       VuoLibrary);
		loadSchema("schema/VuoNode.jsonschema",          VuoNode);
		loadSchema("schema/VuoInputData.jsonschema",     VuoInputData);
		loadSchema("schema/VuoInputEvent.jsonschema",    VuoInputEvent);
		loadSchema("schema/VuoOutputData.jsonschema",    VuoOutputData);
		loadSchema("schema/VuoOutputEvent.jsonschema",   VuoOutputEvent);
		loadSchema("schema/VuoOutputTrigger.jsonschema", VuoOutputTrigger);
	}

	~TestModules()
	{
		WJECloseDocument(VuoType);
		WJECloseDocument(VuoLibrary);
		WJECloseDocument(VuoNode);
		WJECloseDocument(VuoInputData);
		WJECloseDocument(VuoInputEvent);
		WJECloseDocument(VuoOutputData);
		WJECloseDocument(VuoOutputEvent);
		WJECloseDocument(VuoOutputTrigger);

		delete compiler;
	}


private slots:
	void testType_data()
	{
		QTest::addColumn<VuoCompilerType *>("type");

		map<string, VuoCompilerType *> types = compiler->getTypes();
		for (map<string, VuoCompilerType *>::iterator i = types.begin(); i != types.end(); ++i)
			QTest::newRow(i->first.c_str()) << i->second;
	}
	void testType()
	{
		QFETCH(VuoCompilerType *, type);
		QVERIFY(validate(VuoType, json_object_to_json_string(type->moduleDetails), type->getBase()->getModuleKey().c_str()));
	}


	void testLibrary_data()
	{
		QTest::addColumn<VuoCompilerModule *>("library");

		map<string, VuoCompilerModule *> libraries = compiler->getLibraryModules();
		for (map<string, VuoCompilerModule *>::iterator i = libraries.begin(); i != libraries.end(); ++i)
			QTest::newRow(i->first.c_str()) << i->second;
	}
	void testLibrary()
	{
		QFETCH(VuoCompilerModule *, library);
		QVERIFY(validate(VuoLibrary, json_object_to_json_string(library->moduleDetails), library->getPseudoBase()->getModuleKey().c_str()));
	}


	void testNode_data()
	{
		QTest::addColumn<VuoCompilerNodeClass *>("nodeClass");

		map<string, VuoCompilerNodeClass *> nodeClasses = compiler->getNodeClasses();
		for (map<string, VuoCompilerNodeClass *>::iterator i = nodeClasses.begin(); i != nodeClasses.end(); ++i)
		{
			if (VuoStringUtilities::beginsWith(i->first, "vuo.test."))
				continue;

			QTest::newRow(i->first.c_str()) << i->second;
		}
	}
	void testNode()
	{
		QFETCH(VuoCompilerNodeClass *, nodeClass);

		QVERIFY(validate(VuoNode, json_object_to_json_string(nodeClass->moduleDetails), nodeClass->getBase()->getClassName().c_str()));

		foreach (VuoPortClass *portClass, nodeClass->getBase()->getInputPortClasses())
		{
			const char *details = json_object_to_json_string(dynamic_cast<VuoCompilerPortClass *>(portClass->getCompiler())->getDetails());
			VuoPortClass::PortType type = portClass->getPortType();
			if (type == VuoPortClass::dataAndEventPort)
			{
				// For data-and-event ports, the port itself is the event port…
				QVERIFY(validate(VuoInputEvent, details, portClass->getName().c_str()));

				// …and it has a data attachment:
				VuoCompilerDataClass *dataClass = dynamic_cast<VuoCompilerEventPortClass *>(portClass->getCompiler())->getDataClass();

				const char *dataDetails = json_object_to_json_string(dataClass->getDetails());
				QVERIFY(validate(VuoInputData, dataDetails, portClass->getName().c_str()));
			}
			else // if (type == VuoPortClass::eventOnlyPort)
				QVERIFY(validate(VuoInputEvent, details, portClass->getName().c_str()));
		}

		foreach (VuoPortClass *portClass, nodeClass->getBase()->getOutputPortClasses())
		{
			const char *details = json_object_to_json_string(dynamic_cast<VuoCompilerPortClass *>(portClass->getCompiler())->getDetails());
			VuoPortClass::PortType type = portClass->getPortType();
			if (type == VuoPortClass::dataAndEventPort)
			{
				// For data-and-event ports, the port itself is the event port…
				QVERIFY(validate(VuoOutputEvent, details, portClass->getName().c_str()));

				// …and it has a data attachment:
				VuoCompilerDataClass *dataClass = dynamic_cast<VuoCompilerEventPortClass *>(portClass->getCompiler())->getDataClass();

				const char *dataDetails = json_object_to_json_string(dataClass->getDetails());
				QVERIFY(validate(VuoOutputData, dataDetails, portClass->getName().c_str()));
			}
			else if (type == VuoPortClass::eventOnlyPort)
				QVERIFY(validate(VuoOutputEvent, details, portClass->getName().c_str()));
			else // if (type == VuoPortClass::triggerPort)
				QVERIFY(validate(VuoOutputTrigger, details, portClass->getName().c_str()));
		}
	}
};

QTEST_APPLESS_MAIN(TestModules)
#include "TestModules.moc"

/**
 * @file
 * TestInputEditors implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoInputEditorManager.hh"
#include "VuoInputEditorWithDialog.hh"
#include "TestCompositionExecution.hh"

#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <objc/objc-runtime.h>
#include <dlfcn.h>

extern "C" {
void *VuoApp_mainThread = NULL;	///< A reference to the main thread
}

/**
 * Get a reference to the main thread, so we can perform runtime thread assertions.
 */
static void __attribute__((constructor)) TestInputEditors_init(void)
{
	VuoApp_mainThread = (void *)pthread_self();

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	// Calls _TSGetMainThread().
	// https://b33p.net/kosada/node/12944
	YieldToAnyThread();
#pragma clang diagnostic pop
}

/**
 * Tests for the VuoInputEditor* classes.
 */
class TestInputEditors: public QObject
{
	Q_OBJECT

private:
	VuoInputEditorManager *iem;

	static QString keypressesToSend;

	static void breakFromNSApplicationRun()
	{
		// [NSApp stop:nil];
		id *nsAppGlobal = (id *)dlsym(RTLD_DEFAULT, "NSApp");
		((void (*)(id, SEL, id))objc_msgSend)(*nsAppGlobal, sel_getUid("stop:"), nil);

		// After sending the `stop:` message, the event loop needs to wake up and process another event in order to finish.
		VuoEventLoop_break();
	}

	/**
	 * Simulate key events.
	 *
	 * I first tried using `QTest::keyClicks`, `QApplication::sendEvent`, and `QApplication::postEvent`,
	 * but I couldn't get any of those to work.
	 */
	static void sendKeypresses()
	{
//		VLog("Sending \"%s\"…", QString(keypressesToSend).replace('\t', "<tab>").replace('\n', "<return>").toUtf8().constData());
		int keyCount = keypressesToSend.length();
		for (int i = 0; i < keyCount; ++i)
		{
			UniChar key = keypressesToSend.at(i).unicode();

			CGEventRef e = CGEventCreateKeyboardEvent(NULL, 0, true);
			CGEventKeyboardSetUnicodeString(e, 1, &key);
			CGEventPost(kCGHIDEventTap, e);

			e = CGEventCreateKeyboardEvent(NULL, 0, false);
			CGEventKeyboardSetUnicodeString(e, 1, &key);
			CGEventPost(kCGHIDEventTap, e);

			// Typing too fast causes the keystrokes to sometimes get out of order (!).
			usleep(USEC_PER_SEC/30);
			QApplication::processEvents();
		}

		breakFromNSApplicationRun();
	}

	/**
	 * Simulate return key.
	 */
	static void sendReturn()
	{
//		VLog("Sending \"<return>\"…");

		CGEventRef e = CGEventCreateKeyboardEvent(NULL, kVK_Return, true);
		CGEventPost(kCGHIDEventTap, e);

		e = CGEventCreateKeyboardEvent(NULL, kVK_Return, false);
		CGEventPost(kCGHIDEventTap, e);

		breakFromNSApplicationRun();
	}


private slots:
	void initTestCase()
	{
		QList<QDir> pluginDirectories{ QString(BINARY_DIR) + "/lib" };
		iem = new VuoInputEditorManager(pluginDirectories);
		iem->waitForInitiailization();
	}

	/**
	 * Shows an input editor with a given locale,
	 * simulates typing into the input editor (followed by Return),
	 * and checks the input editor's accepted value.
	 *
	 * Ensures input editors with real number fields work both when the decimal is a dot (en_US) and comma (fr_FR and de_DE).
	 * Ensures input editors with numeric fields work when the thousands separator is a comma (en_US), space (fr_FR), and dot (de_DE).
	 * https://b33p.net/kosada/node/6422
	 * https://b33p.net/kosada/node/12813
	 */
	void testTyping_data()
	{
		QTest::addColumn<QString>("inputEditorType");
		QTest::addColumn<QLocale>("locale");
		QTest::addColumn<QString>("keypressesToSend");
		QTest::addColumn<QString>("expectedValue");

		QLocale en_US(QLocale::English, QLocale::UnitedStates);
		QLocale fr_FR(QLocale::French,  QLocale::France);
		QLocale de_DE(QLocale::German,  QLocale::Germany);

		QTest::newRow("VuoEdgeBlend en_US") << "VuoEdgeBlend" << en_US << "1,234.56\t7,890.12\t3,456.78\n" << "{\"crop\":1234.56006,\"cutoff\":7890.12012,\"gamma\":3456.78003}";
		QTest::newRow("VuoEdgeBlend fr_FR") << "VuoEdgeBlend" << fr_FR << "1 234,56\t7 890,12\t3 456,78\n" << "{\"crop\":1234.56006,\"cutoff\":7890.12012,\"gamma\":3456.78003}";
		QTest::newRow("VuoEdgeBlend de_DE") << "VuoEdgeBlend" << de_DE << "1.234,56\t7.890,12\t3.456,78\n" << "{\"crop\":1234.56006,\"cutoff\":7890.12012,\"gamma\":3456.78003}";

		QTest::newRow("VuoInteger en_US") << "VuoInteger" << en_US << "1,234\n" << "1234";
		QTest::newRow("VuoInteger fr_FR") << "VuoInteger" << fr_FR << "1 234\n" << "1234";
		QTest::newRow("VuoInteger de_DE") << "VuoInteger" << de_DE << "1.234\n" << "1234";

		QTest::newRow("VuoIntegerRange en_US") << "VuoIntegerRange" << en_US << "1,234\t5,678\n" << "{\"minimum\":1234,\"maximum\":5678}";
		QTest::newRow("VuoIntegerRange fr_FR") << "VuoIntegerRange" << fr_FR << "1 234\t5 678\n" << "{\"minimum\":1234,\"maximum\":5678}";
		QTest::newRow("VuoIntegerRange de_DE") << "VuoIntegerRange" << de_DE << "1.234\t5.678\n" << "{\"minimum\":1234,\"maximum\":5678}";

		QTest::newRow("VuoMovieFormat en_US") << "VuoMovieFormat" << en_US << "1,234.56\t7,890.12\n" << "{\"imageEncoding\":\"h264\",\"imageQuality\":1234.56,\"audioEncoding\":\"AAC\",\"audioQuality\":7890.12}";
		QTest::newRow("VuoMovieFormat fr_FR") << "VuoMovieFormat" << fr_FR << "1 234,56\t7 890,12\n" << "{\"imageEncoding\":\"h264\",\"imageQuality\":1234.56,\"audioEncoding\":\"AAC\",\"audioQuality\":7890.12}";
		QTest::newRow("VuoMovieFormat de_DE") << "VuoMovieFormat" << de_DE << "1.234,56\t7.890,12\n" << "{\"imageEncoding\":\"h264\",\"imageQuality\":1234.56,\"audioEncoding\":\"AAC\",\"audioQuality\":7890.12}";

		QTest::newRow("VuoPoint2d en_US") << "VuoPoint2d" << en_US << "1,234.56\t7,890.12\n" << "{\"x\":1234.56006,\"y\":7890.12012}";
		QTest::newRow("VuoPoint2d fr_FR") << "VuoPoint2d" << fr_FR << "1 234,56\t7 890,12\n" << "{\"x\":1234.56006,\"y\":7890.12012}";
		QTest::newRow("VuoPoint2d de_DE") << "VuoPoint2d" << de_DE << "1.234,56\t7.890,12\n" << "{\"x\":1234.56006,\"y\":7890.12012}";

		QTest::newRow("VuoPoint3d en_US") << "VuoPoint3d" << en_US << "1,234.56\t7,890.12\t3,456.78\n" << "{\"x\":1234.56006,\"y\":7890.12012,\"z\":3456.78003}";
		QTest::newRow("VuoPoint3d fr_FR") << "VuoPoint3d" << fr_FR << "1 234,56\t7 890,12\t3 456,78\n" << "{\"x\":1234.56006,\"y\":7890.12012,\"z\":3456.78003}";
		QTest::newRow("VuoPoint3d de_DE") << "VuoPoint3d" << de_DE << "1.234,56\t7.890,12\t3.456,78\n" << "{\"x\":1234.56006,\"y\":7890.12012,\"z\":3456.78003}";

		QTest::newRow("VuoPoint4d en_US") << "VuoPoint4d" << en_US << "1,234.56\t7,890.12\t3,456.78\t9,012.34\n" << "{\"x\":1234.56006,\"y\":7890.12012,\"z\":3456.78003,\"w\":9012.33984}";
		QTest::newRow("VuoPoint4d fr_FR") << "VuoPoint4d" << fr_FR << "1 234,56\t7 890,12\t3 456,78\t9 012,34\n" << "{\"x\":1234.56006,\"y\":7890.12012,\"z\":3456.78003,\"w\":9012.33984}";
		QTest::newRow("VuoPoint4d de_DE") << "VuoPoint4d" << de_DE << "1.234,56\t7.890,12\t3.456,78\t9.012,34\n" << "{\"x\":1234.56006,\"y\":7890.12012,\"z\":3456.78003,\"w\":9012.33984}";

		QTest::newRow("VuoRange en_US") << "VuoRange" << en_US << "1,234.56\t7,890.12\n" << "{\"minimum\":1234.56,\"maximum\":7890.12}";
		QTest::newRow("VuoRange fr_FR") << "VuoRange" << fr_FR << "1 234,56\t7 890,12\n" << "{\"minimum\":1234.56,\"maximum\":7890.12}";
		QTest::newRow("VuoRange de_DE") << "VuoRange" << de_DE << "1.234,56\t7.890,12\n" << "{\"minimum\":1234.56,\"maximum\":7890.12}";

		QTest::newRow("VuoReal en_US") << "VuoReal" << en_US << "1,234.56\n" << "1234.56";
		QTest::newRow("VuoReal fr_FR") << "VuoReal" << fr_FR << "1 234,56\n" << "1234.56";
		QTest::newRow("VuoReal de_DE") << "VuoReal" << de_DE << "1.234,56\n" << "1234.56";

		QTest::newRow("VuoReal en_US precise") << "VuoReal" << en_US << "1.0000000001\n" << "1.0000000001";

		QTest::newRow("VuoRealRegulation en_US") << "VuoRealRegulation" << en_US << "test\t1,234.56\t7,890.12\t3,456.78\t9,012.34\n" << "{\"name\":\"test\",\"minimumValue\":1234.56,\"maximumValue\":7890.12,\"defaultValue\":3456.78,\"smoothDuration\":9012.34}";
		QTest::newRow("VuoRealRegulation fr_FR") << "VuoRealRegulation" << fr_FR << "test\t1 234,56\t7 890,12\t3 456,78\t9 012,34\n" << "{\"name\":\"test\",\"minimumValue\":1234.56,\"maximumValue\":7890.12,\"defaultValue\":3456.78,\"smoothDuration\":9012.34}";
		QTest::newRow("VuoRealRegulation de_DE") << "VuoRealRegulation" << de_DE << "test\t1.234,56\t7.890,12\t3.456,78\t9.012,34\n" << "{\"name\":\"test\",\"minimumValue\":1234.56,\"maximumValue\":7890.12,\"defaultValue\":3456.78,\"smoothDuration\":9012.34}";

		QTest::newRow("VuoTransform en_US") << "VuoTransform" << en_US << "1,234.56\t7,890.12\t3,456.78\t9,012.34\t5,678.90\t1,234.56\t7,890.12\t3,456.78\t9,012.34\n" << "{\"translation\":[1234.56006,7890.12012,3456.78003],\"eulerRotation\":[157.29500,99.11550,21.54714],\"scale\":[7890.12012,3456.78003,9012.33984]}";
		QTest::newRow("VuoTransform fr_FR") << "VuoTransform" << fr_FR << "1 234,56\t7 890,12\t3 456,78\t9 012,34\t5 678,90\t1 234,56\t7 890,12\t3 456,78\t9 012,34\n" << "{\"translation\":[1234.56006,7890.12012,3456.78003],\"eulerRotation\":[157.29500,99.11550,21.54714],\"scale\":[7890.12012,3456.78003,9012.33984]}";
		QTest::newRow("VuoTransform de_DE") << "VuoTransform" << de_DE << "1.234,56\t7.890,12\t3.456,78\t9.012,34\t5.678,90\t1.234,56\t7.890,12\t3.456,78\t9.012,34\n" << "{\"translation\":[1234.56006,7890.12012,3456.78003],\"eulerRotation\":[157.29500,99.11550,21.54714],\"scale\":[7890.12012,3456.78003,9012.33984]}";

		QTest::newRow("VuoTransform2d en_US") << "VuoTransform2d" << en_US << "1,234.56\t7,890.12\t3,456.78\t9,012.34\t5,678.90\n" << "{\"translation\":[1234.56006,7890.12012],\"rotation\":60.33219,\"scale\":[9012.33984,5678.89990]}";
		QTest::newRow("VuoTransform2d fr_FR") << "VuoTransform2d" << fr_FR << "1 234,56\t7 890,12\t3 456,78\t9 012,34\t5 678,90\n" << "{\"translation\":[1234.56006,7890.12012],\"rotation\":60.33219,\"scale\":[9012.33984,5678.89990]}";
		QTest::newRow("VuoTransform2d de_DE") << "VuoTransform2d" << de_DE << "1.234,56\t7.890,12\t3.456,78\t9.012,34\t5.678,90\n" << "{\"translation\":[1234.56006,7890.12012],\"rotation\":60.33219,\"scale\":[9012.33984,5678.89990]}";
	}
	void testTyping()
	{
		QFETCH(QString, inputEditorType);
		QFETCH(QLocale, locale);
		QFETCH(QString, keypressesToSend);
		QFETCH(QString, expectedValue);

		QLocale::setDefault(locale);

		VuoType *t = new VuoType(inputEditorType.toUtf8().constData());
		QVERIFY(t);
		VuoInputEditorWithDialog *ie = dynamic_cast<VuoInputEditorWithDialog *>(iem->newInputEditor(t));
		QVERIFY(ie);

		// Invoke the input editor with default values.
		json_object *originalValue = NULL;
		// Some input editors have numeric fields that are disabled by default.  Enable them.
		if (inputEditorType == "VuoRange" || inputEditorType == "VuoIntegerRange")
			originalValue = json_tokener_parse("{\"minimum\":0,\"maximum\":1}");
		else if (inputEditorType == "VuoMovieFormat")
			originalValue = json_tokener_parse("{\"imageEncoding\":\"H264\",\"imageQuality\":1,\"audioEncoding\":\"AAC\",\"audioQuality\":1}");

		// Enqueue the keypresses so they happen during `show`.
		TestInputEditors::keypressesToSend = keypressesToSend;
		QTimer::singleShot(100, this, sendKeypresses);

		json_object *acceptedValue = ie->show(QPoint(200, 200), originalValue, NULL, map<QString, json_object *>());

		TestCompositionExecution::checkEqual(QTest::currentDataTag(), inputEditorType.toUtf8().constData(), acceptedValue, json_tokener_parse(expectedValue.toUtf8().constData()));

		delete ie;


		// Invoke the input editor again, this time starting with the value resulting from the first invocation,
		// and accept the existing value (just press return).
		// This ensures the input editor correctly unserializes its starting value and correctly populates its fields with it.
		ie = dynamic_cast<VuoInputEditorWithDialog *>(iem->newInputEditor(t));
		QVERIFY(ie);

		QTimer::singleShot(100, this, sendReturn);

		json_object *acceptedValue2 = ie->show(QPoint(200,200), acceptedValue, NULL, map<QString, json_object *>());

		TestCompositionExecution::checkEqual(QTest::currentDataTag(), inputEditorType.toUtf8().constData(), acceptedValue2, acceptedValue);

		delete ie;


		delete t;
	}
};

QString TestInputEditors::keypressesToSend;

int main(int argc, char *argv[])
{
	// Tell Qt where to find its plugins.
	QApplication::setLibraryPaths(QStringList((VuoFileUtilities::getVuoFrameworkPath() + "/../QtPlugins").c_str()));

	QApplication app(argc, argv);
	TestInputEditors t;
	return QTest::qExec(&t, argc, argv);
}

#include "TestInputEditors.moc"

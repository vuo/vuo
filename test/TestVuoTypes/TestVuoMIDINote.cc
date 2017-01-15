/**
 * @file
 * TestVuoMidiNote implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoMidiNote.h"
}

/**
 * Tests the VuoMidiNote type.
 */
class TestVuoMidiNote : public QObject
{
	Q_OBJECT

private slots:

	void testSerializationAndSummary_data()
	{
		QTest::addColumn<QString>("value");
		QTest::addColumn<bool>("valid"); // Is @c value expected to produce a valid result?
		QTest::addColumn<QString>("summary");

		QTest::newRow("emptystring")	<< ""
										<< false
										<< "";

		QTest::newRow("note on, middle C")	<< "{\"channel\":1,\"isNoteOn\":true,\"velocity\":127,\"noteNumber\":60}"
											<< true
											<< "Channel 1: note C4 (#60) on, velocity 127";

		QTest::newRow("note off, A♯0")		<< "{\"channel\":16,\"isNoteOn\":false,\"velocity\":127,\"noteNumber\":22}"
											<< true
											<< "Channel 16: note A♯0 (#22) off, velocity 127";
	}
	void testSerializationAndSummary()
	{
		QFETCH(QString, value);
		QFETCH(bool, valid);
		QFETCH(QString, summary);

		VuoMidiNote v = VuoMidiNote_makeFromString(value.toUtf8().data());
		if (valid)
		{
			QCOMPARE(QString::fromUtf8(VuoMidiNote_getString(v)), value);
			QCOMPARE(QString::fromUtf8(VuoMidiNote_getSummary(v)), summary);
		}
	}
};

QTEST_APPLESS_MAIN(TestVuoMidiNote)

#include "TestVuoMidiNote.moc"

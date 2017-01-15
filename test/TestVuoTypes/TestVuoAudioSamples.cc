/**
 * @file
 * TestVuoAudioSamples implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoAudioSamples.h"
}

/**
 * Tests the VuoAudioSamples type.
 */
class TestVuoAudioSamples : public QObject
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
										<< "0 samples @ 0 kHz";

		QTest::newRow("3 samples")		<< "{\"samples\":[-0.014525,0.015363,0.013679],\"samplesPerSecond\":44100}"
										<< true
										<< "3 samples @ 44.1 kHz";
	}
	void testSerializationAndSummary()
	{
		QFETCH(QString, value);
		QFETCH(bool, valid);
		QFETCH(QString, summary);

		VuoAudioSamples v = VuoAudioSamples_makeFromString(value.toUtf8().data());
		if (valid)
			QCOMPARE(QString::fromUtf8(VuoAudioSamples_getString(v)), value);
		QCOMPARE(QString::fromUtf8(VuoAudioSamples_getSummary(v)), summary);
	}
};

QTEST_APPLESS_MAIN(TestVuoAudioSamples)

#include "TestVuoAudioSamples.moc"

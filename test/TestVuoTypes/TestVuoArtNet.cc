/**
 * @file
 * TestVuoArtNet implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoArtNetOutputDevice.h"
}

/**
 * Tests the VuoArtNet type.
 */
class TestVuoArtNet : public QObject
{
	Q_OBJECT

private slots:

	void testComparison()
	{
		VuoArtNetOutputDevice unicast0 = VuoArtNetOutputDevice_makeUnicast("10.0.0.1", 0, 0, 0);
		VuoArtNetOutputDevice unicast1 = VuoArtNetOutputDevice_makeUnicast("10.0.0.1", 0, 0, 1);
		QVERIFY( VuoArtNetOutputDevice_areEqual  (unicast0, unicast0));
		QVERIFY( VuoArtNetOutputDevice_areEqual  (unicast1, unicast1));
		QVERIFY(!VuoArtNetOutputDevice_areEqual  (unicast0, unicast1));
		QVERIFY( VuoArtNetOutputDevice_isLessThan(unicast0, unicast1));
		QVERIFY(!VuoArtNetOutputDevice_isLessThan(unicast1, unicast0));


		VuoArtNetInputDevice receive0 = VuoArtNetInputDevice_make(0, 0, 0);
		VuoArtNetInputDevice receive1 = VuoArtNetInputDevice_make(0, 0, 1);
		QVERIFY( VuoArtNetInputDevice_areEqual  (receive0, receive0));
		QVERIFY( VuoArtNetInputDevice_areEqual  (receive1, receive1));
		QVERIFY(!VuoArtNetInputDevice_areEqual  (receive0, receive1));
		QVERIFY( VuoArtNetInputDevice_isLessThan(receive0, receive1));
		QVERIFY(!VuoArtNetInputDevice_isLessThan(receive1, receive0));
	}
};

QTEST_APPLESS_MAIN(TestVuoArtNet)

#include "TestVuoArtNet.moc"

/**
 * @file
 * TestVuoUuid implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoUuid.h"
}

/**
 * Tests the VuoUuid type.
 */
class TestVuoUuid : public QObject
{
	Q_OBJECT

private slots:

	void testUuidComparison()
	{
		VuoUuid empty;
		VuoUuid uuid = VuoUuid_make();
		VuoUuid copy = uuid;

		QVERIFY2( VuoUuid_areEqual(uuid, copy), "Duplicate UUID is not equal." );
		QVERIFY2( !VuoUuid_areEqual(empty, uuid), "Comparison to empty UUID is true." );

		// run through a bunch of new uuids to make sure there's not something
		// seriously wrong happening in uuid generation
		QBENCHMARK
		{
			QVERIFY2( !VuoUuid_areEqual(uuid, VuoUuid_make()), "Comparison to new UUID is true." );
		}
	}
};

QTEST_APPLESS_MAIN(TestVuoUuid)

#include "TestVuoUuid.moc"

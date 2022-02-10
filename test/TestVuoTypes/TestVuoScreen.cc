/**
 * @file
 * TestVuoScreen implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoScreen.h"
#include "VuoList_VuoScreen.h"
#include "VuoScreenCommon.h"
}

/**
 * Tests the VuoScreen type.
 */
class TestVuoScreen : public QObject
{
	Q_OBJECT

private slots:

	void testScreenName()
	{
		// https://b33p.net/kosada/vuo/vuo/-/issues/18596
		VuoList_VuoScreen screens = VuoScreen_getList();
		VuoLocal(screens);
		__block bool ok = true;
		VuoListForeach_VuoScreen(screens, ^(const VuoScreen screen) {
			VUserLog("Screen: \"%s\"", screen.name);
			if (!screen.name || strlen(screen.name) < 10)
				ok = false;
			return true;
		});
		if (!ok)
			QFAIL("One of the currently-attached screens has an empty or unrealistically-short name.");
	}

};

QTEST_APPLESS_MAIN(TestVuoScreen)

#include "TestVuoScreen.moc"

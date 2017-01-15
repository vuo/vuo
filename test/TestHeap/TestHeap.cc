/**
 * @file
 * TestHeap interface and implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <QtTest/QtTest>

extern "C" {
#include "type.h"
}

class TestHeap : public QObject
{
	Q_OBJECT

private slots:

	void testRegisterPerformance()
	{
		QBENCHMARK {
			VuoRegister(malloc(1), free);
		}
	}

	void testRetainReleasePerformance()
	{
		void *p = malloc(1);
		VuoRegister(p, free);
		VuoRetain(p);
		QBENCHMARK {
			VuoRetain(p);
			VuoRelease(p);
		}
		VuoRelease(p);
	}
};

QTEST_APPLESS_MAIN(TestHeap)
#include "TestHeap.moc"

/**
 * @file
 * TestHeap interface and implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
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

	// https://b33p.net/kosada/node/12778
	void testRetainReleaseThreadedPerformance_data()
	{
		QTest::addColumn<int>("threads");

		QTest::newRow("1") << 1;
		QTest::newRow("2") << 2;
		QTest::newRow("4") << 4;
		QTest::newRow("8") << 8;
	}
	void testRetainReleaseThreadedPerformance()
	{
		QFETCH(int, threads);
		QBENCHMARK {
			void *p = malloc(1);
			VuoRegister(p, free);
			VuoRetain(p);

			dispatch_semaphore_t done = dispatch_semaphore_create(0);

			for (int i = 0; i < threads; ++i)
				dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
					for (long i = 0; i < 1000000/threads; ++i)
					{
						VuoRetain(p);
						VuoRelease(p);
					}
					dispatch_semaphore_signal(done);
				});

			for (int i = 0; i < threads; ++i)
				dispatch_semaphore_wait(done, DISPATCH_TIME_FOREVER);

			VuoRelease(p);
		}
	}
};

QTEST_APPLESS_MAIN(TestHeap)
#include "TestHeap.moc"

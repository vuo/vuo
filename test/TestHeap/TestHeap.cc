/**
 * @file
 * TestHeap interface and implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
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

	void testIsPointerReadable_data()
	{
		QTest::addColumn<void *>("pointer");
		QTest::addColumn<bool>("expectedReadable");

		QTest::newRow("NULL")                << (void *)0x0            << false;

		QTest::newRow("const string")        << (void *)"const string" << true;

		void *p = malloc(1);
		QTest::newRow("malloc 1B")           << p << true;

		p = malloc(1024);
		QTest::newRow("malloc 1KB")          << p << true;

		p = malloc(1024*1024);
		QTest::newRow("malloc 1MB")          << p << true;

		p = malloc(1024*1024*1024);
		QTest::newRow("malloc 1GB")          << p << true;

		QTest::newRow("Magic Music Visuals") << (void *)0x3f80000000fe1e70 << false;

		QTest::newRow("64bit max")           << (void *)0xffffffffffffffff << false;
	}
	void testIsPointerReadable()
	{
		QFETCH(void *, pointer);
		QFETCH(bool, expectedReadable);

		QCOMPARE(VuoHeap_isPointerReadable(pointer), expectedReadable);
	}
};

QTEST_APPLESS_MAIN(TestHeap)
#include "TestHeap.moc"

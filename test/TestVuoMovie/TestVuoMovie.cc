/**
 * @file
 * TestVuoMovie interface and implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <QtTest>

extern "C" {
#include "VuoMovie.h"
}

class TestVuoMovie : public QObject
{
	Q_OBJECT

private slots:

	void initTestCase()
	{
		VuoHeap_init();
	}

	void testFishMovieInfoPerformance()
	{
		double duration;
		QBENCHMARK {
			QVERIFY(VuoMovie_getInfo("/System/Library/Compositions/Fish.mov", &duration));
		}
	}

	void testFishMovieDecodePerformance()
	{
		VuoMovie m = VuoMovie_make("/System/Library/Compositions/Fish.mov");
		QVERIFY(m != NULL);

		unsigned int frames = 0;
		double pts;
		VuoImage i;
		QBENCHMARK {
			while (VuoMovie_getNextFrame(m, &i, &pts))
			{
				VuoRetain(i);
				VuoRelease(i);
				++frames;
			}
		}
		QVERIFY(frames == 397);
	}

	void testFishMovieFramesDecodeRandomPerformance()
	{
		VuoMovie m = VuoMovie_make("/System/Library/Compositions/Fish.mov");
		QVERIFY(m != NULL);

		double duration = VuoMovie_getDuration(m);
		// QuickTime Player says Fish.mov is 30 FPS (maybe we should provide a VuoMovie function to get average framerate).
		unsigned int frames = duration * 30;

		double pts;
		VuoImage i;
		QBENCHMARK {
			while (frames--)
			{
				double frameTime = duration * (double)rand()/(double)RAND_MAX;
				QVERIFY(VuoMovie_seekToSecond(m, frameTime));
				QVERIFY(VuoMovie_getNextFrame(m, &i, &pts));
				VuoRetain(i);
				VuoRelease(i);
			}
		}
	}
};

QTEST_APPLESS_MAIN(TestVuoMovie)
#include "TestVuoMovie.moc"

/**
 * @file
 * TestVuoVideo interface and implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <QtTest/QtTest>

extern "C" {
#include "type.h"
#include "VuoMovie.h"
}

class TestVuoVideo : public QObject
{
	Q_OBJECT

private slots:

	void initTestCase()
	{
		VuoHeap_init();
	}

	void createData(void)
	{
		QTest::addColumn<QString>("url");
		QTest::addColumn<double>("expectedDuration");	// in seconds
		QTest::addColumn<int>("expectedFrameCount");
		QTest::addColumn<int>("expectedPixelsWide");
		QTest::addColumn<int>("expectedPixelsHigh");
		QTest::addColumn<int>("expectedAudioChannels");	// negative if expected to contain some packets that are completely silent

		QTest::newRow("Animated GIF")							<< "/MovieGauntlet/count.gif"																			<<  10.1	<<   86	<<  748 <<  491	<<  0;
		QTest::newRow("DIF DV NTSC")							<< "/MovieGauntlet/Audio Codecs/Quicktime Player 7/french.dv"											<<  19.9	<<  597	<<  720 <<  480	<<  2;
		QTest::newRow("MPEG Transport Stream")					<< "/MovieGauntlet/interlaced/SD_NTSC_29.97_640x480.ts"													<<  28.7	<<  859	<<  640 <<  480	<< -2;
		QTest::newRow("MPEG v2")								<< "/MovieGauntlet/interlaced/interlace_test2.mpeg"														<<  39.9	<<  998	<<  720 <<  576	<< -2;
		QTest::newRow("MPEG v4 1")								<< "/MovieGauntlet/Audio Codecs/Miro Video Converter/french.large1080p.mp4"								<<  24.0	<<  577	<<  320 <<  240	<<  0;
//		QTest::newRow("MPEG v4 AVC")							<< "/MovieGauntlet/audio+video synchronization/Lip Sync Test Markers.m4v"								<<  67.9	<< 2035	<<  640 <<  480	<<  0;
//hang	QTest::newRow("Ogg - Theora - Ogg LR")					<< "/MovieGauntlet/Audio Codecs/Miro Video Converter/french.oggtheora.ogv"								<<  24.0	<<    0	<<  320 <<  240	<<  0;
		QTest::newRow("Ogg - Theora - Ogg 5.1")					<< "/MovieGauntlet/Audio Codecs/Quicktime Player 7/french.ogg"											<<  19.5	<<  467	<<  320 <<  240	<< -6;
		QTest::newRow("QuickTime - Animation - None")			<< "/MovieGauntlet/demo to mov3.mov"																	<<   1.0	<<   29	<<  640 <<  480	<<  0;
		QTest::newRow("QuickTime - H.264 - Lossless 5.1 - #1")	<< "/MovieGauntlet/Audio Codecs/Compressor 4.1.3/french — H.264 — Apple Lossless 5.1.mov"				<<  19.5	<<  470	<< 1920 << 3240	<< -6;
		QTest::newRow("QuickTime - H.264 - Lossless 5.1 - #2")	<< "/MovieGauntlet/Audio Codecs/Compressor 4.1.3/typewriter-barnyard — H.264 — Apple Lossless 5.1.mov"	<<  15.7	<<  377	<< 1440 <<  900	<< -6;
		QTest::newRow("QuickTime - H.264 - Lossless LR")		<< "/MovieGauntlet/out-sd.mov"																			<< 275.2	<< 6598	<<  640 <<  360	<< -2;
		QTest::newRow("QuickTime - Photo JPEG - None")			<< "/System/Library/Compositions/Fish.mov"																<<  13.2	<<  397	<<  640 <<  480	<<  0;
		QTest::newRow("QuickTime - Photo JPEG - PCM LR")		<< "/MovieGauntlet/pbourke/2.mov"																		<<   3.3	<<  101	<< 4096 << 2048	<<  2;
		QTest::newRow("WebM")									<< "/MovieGauntlet/Audio Codecs/Miro Video Converter/french.webmhd.webm"								<<  19.9	<<  475	<<  320 <<  240	<<  2;
	}

	void testInfoPerformance_data()
	{
		printf("	testInfoPerformance()\n"); fflush(stdout);
		createData();
	}
	void testInfoPerformance()
	{
		QFETCH(QString, url);
		QFETCH(double, expectedDuration);

		if (!QFile(url).exists())
			QSKIP(QString("Test movie '%1' not found").arg(url).toUtf8().data(), SkipOne);

		double duration;
		QBENCHMARK {
			QVERIFY(VuoMovie_getInfo(url.toUtf8().data(), &duration));
		}
		QVERIFY2(fabs(duration - expectedDuration) < 1, QString("expected %1, got %2").arg(expectedDuration).arg(duration).toUtf8().data());
	}

	void testDecodePerformance_data()
	{
		printf("	testDecodePerformance()\n"); fflush(stdout);
		createData();
	}
	void testDecodePerformance()
	{
		QFETCH(QString, url);
		QFETCH(double, expectedDuration);
		QFETCH(int, expectedFrameCount);
		QFETCH(int, expectedPixelsWide);
		QFETCH(int, expectedPixelsHigh);
		QFETCH(int, expectedAudioChannels);

		VuoMovie m = VuoMovie_make(strdup(url.toUtf8().data()));
		QVERIFY(m != NULL);

		bool containsAudio = VuoMovie_containsAudio(m);
		QCOMPARE(containsAudio, abs(expectedAudioChannels) > 0);

		int frames = 0;
		double pts;
		VuoImage lastImage = NULL;
		VuoList_VuoAudioSamples audioSamples;
		double frameTimestampInSecs;
		bool gotVideo;
		do
		{
			QBENCHMARK {
				VuoImage image;
				gotVideo = VuoMovie_getNextVideoFrame(m, &image, &pts);
				if (containsAudio)
				{
					audioSamples = VuoListCreate_VuoAudioSamples();
					VuoRetain(audioSamples);
					bool gotAudio = VuoMovie_getNextAudioSample(m, audioSamples, &frameTimestampInSecs);
					QVERIFY(gotAudio);
					int channels = VuoListGetCount_VuoAudioSamples(audioSamples);
					QCOMPARE(channels, abs(expectedAudioChannels));
					if (expectedAudioChannels > 0 && frameTimestampInSecs < expectedDuration)
						for (int i = 1; i <= channels; ++i)
							QVERIFY(!VuoAudioSamples_isEmpty(VuoListGetValue_VuoAudioSamples(audioSamples, i)));
					VuoRelease(audioSamples);
				}

				if (gotVideo)
				{
					VuoRelease(lastImage);
					VuoRetain(image);
					lastImage = image;
					++frames;
				}
			}
		} while (gotVideo);

		QVERIFY2(fabs(frames - expectedFrameCount) < 10, QString("expected %1, got %2").arg(expectedFrameCount).arg(frames).toUtf8().data());

		QCOMPARE(lastImage->pixelsWide, (unsigned long)expectedPixelsWide);
		QCOMPARE(lastImage->pixelsHigh, (unsigned long)expectedPixelsHigh);
		QVERIFY(!VuoImage_isEmpty(lastImage));
		VuoRelease(lastImage);
	}

	void testDecodeRandomPerformance_data()
	{
		printf("	testDecodeRandomPerformance()\n"); fflush(stdout);
		createData();
	}
	void testDecodeRandomPerformance()
	{
		QFETCH(QString, url);

		VuoMovie m = VuoMovie_make(strdup(url.toUtf8().data()));
		QVERIFY(m != NULL);

		double duration = VuoMovie_getDuration(m);

		double pts;
		VuoImage lastImage = NULL;
		int frames = 10;
		while (frames--)
		{
			QBENCHMARK {
				double frameTime = (duration - 2) * (double)rand()/(double)RAND_MAX;
				QVERIFY(VuoMovie_seekToSecond(m, frameTime));
				VuoImage image;
				QVERIFY(VuoMovie_getNextVideoFrame(m, &image, &pts));
				VuoRelease(lastImage);
				VuoRetain(image);
				lastImage = image;
			}
		}

		QVERIFY(!VuoImage_isEmpty(lastImage));
		VuoRelease(lastImage);
	}
};

QTEST_APPLESS_MAIN(TestVuoVideo)
#include "TestVuoVideo.moc"

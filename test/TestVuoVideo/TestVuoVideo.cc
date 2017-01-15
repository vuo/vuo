/**
 * @file
 * TestVuoVideo interface and implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <QtTest/QtTest>

extern "C" {
#include "type.h"
#include "VuoVideo.h"
#include "VuoVideoOptimization.h"

void *VuoApp_mainThread = NULL;	///< A reference to the main thread
}

/**
 * Get a reference to the main thread, so we can perform runtime thread-sanity assertions.
 */
static void __attribute__((constructor)) TestVuoVideo_init(void)
{
	VuoApp_mainThread = (void *)pthread_self();
}

class TestVuoVideo : public QObject
{
	const int AVFOUNDATION_OPTIMIZED = 0;

	/// AvFoundation loads asynchronously, this is the longest time to allow (in ms) prior to failing.
	const unsigned int MAX_VIDEO_LOAD_TIME = 4000;

	/// Video / audio timestamp delta in either direction shouldn't ever exceed .05 (video -.045 behind audio or video .022 ahead is noticeable)
	// const double MAX_AUDIO_DRIFT = .05;

	Q_OBJECT
	// Q_DECLARE_METATYPE(VuoVideoOptimization)

private slots:

	void createData(int optimize)
	{
		QTest::addColumn<QString>("url");
		QTest::addColumn<double>("expectedDuration");	// in seconds
		QTest::addColumn<int>("expectedFrameCount");
		QTest::addColumn<int>("expectedPixelsWide");
		QTest::addColumn<int>("expectedPixelsHigh");
		QTest::addColumn<int>("expectedAudioChannels");	// negative if expected to contain some packets that are completely silent
		QTest::addColumn<int>("optimize");	// test both AvFoundation and Ffmpeg (either can fall back on the other - this tests that too)

		QTest::newRow(VuoText_format("Animated GIF opt=%d",optimize))							<< "/MovieGauntlet/count.gif"																			<<  10.1	<<  85	<<  748 <<  491	<<  0 	<< optimize;
		QTest::newRow(VuoText_format("DIF DV NTSC opt=%d",optimize))							<< "/MovieGauntlet/Audio Codecs/Quicktime Player 7/french.dv"											<<  19.9	<<  596	<<  720 <<  480	<<  2 	<< optimize;
		QTest::newRow(VuoText_format("MPEG Transport Stream opt=%d",optimize))					<< "/MovieGauntlet/interlaced/SD_NTSC_29.97_640x480.ts"													<<  28.7	<<  860	<<  640 <<  480	<< -2 	<< optimize;
		QTest::newRow(VuoText_format("MPEG v2 opt=%d",optimize))								<< "/MovieGauntlet/interlaced/interlace_test2.mpeg"														<<  39.9	<<  997	<<  720 <<  576	<< -2 	<< optimize;
		QTest::newRow(VuoText_format("MPEG v4 1 opt=%d",optimize))								<< "/MovieGauntlet/Audio Codecs/Miro Video Converter/french.large1080p.mp4"								<<  24.0	<<  585	<<  320 <<  240	<<  2 	<< optimize;
		QTest::newRow(VuoText_format("MPEG v4 AVC opt=%d",optimize))							<< "/MovieGauntlet/audio+video synchronization/Lip Sync Test Markers.m4v"								<<  67.9	<< 2044	<<  640 <<  480	<<  2 	<< optimize;
		QTest::newRow(VuoText_format("Ogg - Theora - Ogg LR opt=%d",optimize))					<< "/MovieGauntlet/Audio Codecs/Miro Video Converter/french.oggtheora.ogv"								<<  24.0	<< 	503	<<  320 <<  240	<<  2 	<< optimize;
		QTest::newRow(VuoText_format("Ogg - Theora - Ogg 5.1 opt=%d",optimize))					<< "/MovieGauntlet/Audio Codecs/Quicktime Player 7/french.ogg"											<<  19.5	<<  466	<<  320 <<  240	<< -6 	<< optimize;
		QTest::newRow(VuoText_format("QuickTime - Animation - None opt=%d",optimize))			<< "/MovieGauntlet/demo to mov3.mov"																	<<   1.0	<<   28	<<  640 <<  480	<<  0 	<< optimize;
		QTest::newRow(VuoText_format("QuickTime - H.264 - Lossless 5.1 - #1 opt=%d",optimize))	<< "/MovieGauntlet/Audio Codecs/Compressor 4.1.3/french — H.264 — Apple Lossless 5.1.mov"				<<  19.5	<<  469	<< 1920 << 3240	<< -6 	<< optimize;
		QTest::newRow(VuoText_format("QuickTime - H.264 - Lossless 5.1 - #2 opt=%d",optimize))	<< "/MovieGauntlet/Audio Codecs/Compressor 4.1.3/typewriter-barnyard — H.264 — Apple Lossless 5.1.mov"	<<  15.7	<<  376	<< 1440 <<  900	<< -6 	<< optimize;
		QTest::newRow(VuoText_format("QuickTime - H.264 - Lossless LR opt=%d",optimize))		<< "/MovieGauntlet/out-sd.mov"																			<< 275.2	<< 6608	<<  640 <<  360	<< -2 	<< optimize;
		QTest::newRow(VuoText_format("QuickTime - Photo JPEG - None opt=%d",optimize))			<< "/System/Library/Compositions/Fish.mov"																<<  13.2	<<  397	<<  640 <<  480	<<  0 	<< optimize;
		// avfoundation seems to lock up and wreck subsequent instances of AVAssetReader when trying to load/unload this file
		// QTest::newRow(VuoText_format("QuickTime - Photo JPEG - PCM LR opt=%d",optimize))		<< "/MovieGauntlet/pbourke/2.mov"																		<<   3.3	<<  100	<< 4096 << 2048	<<  2 	<< optimize;
		QTest::newRow(VuoText_format("WebM opt=%d",optimize))									<< "/MovieGauntlet/Audio Codecs/Miro Video Converter/french.webmhd.webm"								<<  19.9	<<  474	<<  320 <<  240	<<  2 	<< optimize;
	}

	void testInfoPerformance_data()
	{
		printf("	testInfoPerformance()\n"); fflush(stdout);

		createData(0);
		createData(1);
	}

	void testInfoPerformance()
	{
		QFETCH(QString, url);
		QFETCH(double, expectedDuration);
		QFETCH(int, optimize);

		if (!QFile(url).exists())
			QSKIP(QString("Test movie '%1' not found").arg(url).toUtf8().data(), SkipOne);

		// QBENCHMARK
		{
		double duration;
		VuoVideo decoder = VuoVideo_make(url.toUtf8().data(), optimize == AVFOUNDATION_OPTIMIZED ? VuoVideoOptimization_Forward : VuoVideoOptimization_Random);
		QVERIFY(decoder != NULL);
		VuoRetain(decoder);
		QTRY_COMPARE_WITH_TIMEOUT(VuoVideo_isReady(decoder), true, MAX_VIDEO_LOAD_TIME);
		duration = VuoVideo_getDuration(decoder);
		QVERIFY(duration);
		VuoRelease(decoder);
		QVERIFY2(fabs(duration - expectedDuration) < 1, QString("expected %1, got %2").arg(expectedDuration).arg(duration).toUtf8().data());
		}
	}

	void testDecodePerformance_data()
	{
		printf("	testDecodePerformance()\n"); fflush(stdout);

		createData(0);
		createData(1);
	}

	void testDecodePerformance()
	{
		QFETCH(QString, url);
		QFETCH(int, expectedFrameCount);
		QFETCH(int, expectedPixelsWide);
		QFETCH(int, expectedPixelsHigh);
		QFETCH(int, expectedAudioChannels);
		QFETCH(int, optimize);

		if (!QFile(url).exists())
			QSKIP(QString("Test movie '%1' not found").arg(url).toUtf8().data(), SkipOne);

		VuoVideo m = VuoVideo_make(strdup(url.toUtf8().data()), optimize == AVFOUNDATION_OPTIMIZED ? VuoVideoOptimization_Forward : VuoVideoOptimization_Random);
		QVERIFY(m != NULL);
		VuoRetain(m);
		QTRY_COMPARE_WITH_TIMEOUT( VuoVideo_isReady(m), true, MAX_VIDEO_LOAD_TIME);

		/// if the video has an audio channel that contains nothing, don't try to read it since
		/// otherwise the decoder will try to sync video frames with it which can be slower and
		/// potentially throw off the frame count (dup or skipped frames while syncing)
		VuoVideo_setPlaybackRate(m, expectedAudioChannels > 0 ? 1 : 0);

		/// make sure audio channels are correct
		int audioChannels = (int) VuoVideo_getAudioChannels(m);
		QCOMPARE(audioChannels, abs(expectedAudioChannels));

		int videoFrameCount = 0,
			audioFrameCount = 0;

		// some videos don't have audio until a few seconds into the clip, which can register
		// as a false positive for 'VuoAudioSamples_isEmpty' test
		int emptyAudioFrames = 0;

		bool gotVideo = false;

		/// run through once with playback rate 1 to check audio
		if(expectedAudioChannels > 0)
		{
			QBENCHMARK
			{
				VuoVideo_seekToSecond(m, 0);
				videoFrameCount = 0;
				audioFrameCount = 0;
				emptyAudioFrames = 0;

				do
				{
					VuoVideoFrame videoFrame;

					gotVideo = VuoVideo_nextVideoFrame(m, &videoFrame);

					VuoAudioFrame audioFrame = VuoAudioFrame_make(VuoListCreate_VuoAudioSamples(), 0);
					VuoRetain(audioFrame.samples);

					bool gotAudio = VuoVideo_nextAudioFrame(m, &audioFrame);

					QVERIFY(gotAudio);

					int receivedChannels = VuoListGetCount_VuoAudioSamples(audioFrame.samples);

					QCOMPARE(receivedChannels, abs(expectedAudioChannels));

					audioFrameCount++;

					for (int i = 1; i <= receivedChannels; ++i)
					{
						if(VuoAudioSamples_isEmpty(VuoListGetValue_VuoAudioSamples(audioFrame.samples, i)))
							emptyAudioFrames++;
					}

					if(gotVideo)
					{
						videoFrameCount++;
						VuoRelease(videoFrame.image);
					}

					VuoRelease(audioFrame.samples);

				} while(gotVideo);

				QVERIFY( emptyAudioFrames < audioFrameCount * abs(expectedAudioChannels) );
			}
		}

		/// now run through alll samples again without bothering audio
		VuoVideo_setPlaybackRate(m, 0);

		VuoImage lastImage = NULL;

		QBENCHMARK
		{
			VuoVideo_seekToSecond(m, 0);
			videoFrameCount = 0;

			do
			{
				VuoVideoFrame videoFrame;

				gotVideo = VuoVideo_nextVideoFrame(m, &videoFrame);

				if(gotVideo)
				{
					videoFrameCount++;
					VuoRelease(lastImage);
					lastImage = videoFrame.image;
				}
			} while( gotVideo );

			/// AvFoundation and Ffmpeg don't sync exactly with frame counts, but they're very close.  Test that the frame count is approximately correct.
			QVERIFY2(abs(expectedFrameCount - videoFrameCount) < expectedFrameCount * .03, QString("expected ~%1 frames, got %2").arg(expectedFrameCount).arg(videoFrameCount).toUtf8().data());
		}

		QCOMPARE(lastImage->pixelsWide, (unsigned long)expectedPixelsWide);
		QCOMPARE(lastImage->pixelsHigh, (unsigned long)expectedPixelsHigh);

		QVERIFY(!VuoImage_isEmpty(lastImage));
		VuoRelease(lastImage);

		VuoRelease(m);
	}

	void testDecodeRandomPerformance_data()
	{
		printf("	testDecodeRandomPerformance()\n"); fflush(stdout);

		createData(0);
		createData(1);
	}
	void testDecodeRandomPerformance()
	{
		QFETCH(QString, url);
		QFETCH(int, optimize);
		// QFETCH(int, expectedAudioChannels);
		VUserLog("[%s] optimize=%s",url.toUtf8().data(), (optimize == AVFOUNDATION_OPTIMIZED ? "AVFoundation" : "FFMPEG"));

		if (!QFile(url).exists())
			QSKIP(QString("Test movie '%1' not found").arg(url).toUtf8().data(), SkipOne);

		VuoVideo m = VuoVideo_make(strdup(url.toUtf8().data()), AVFOUNDATION_OPTIMIZED == 0 ? VuoVideoOptimization_Forward : VuoVideoOptimization_Random);
		VuoRetain(m);

		QVERIFY(m != NULL);

		QTRY_COMPARE_WITH_TIMEOUT(VuoVideo_isReady(m), true, MAX_VIDEO_LOAD_TIME);

		double duration = VuoVideo_getDuration(m);

		VuoVideoFrame videoFrame = {NULL, 0};
		QBENCHMARK
		{
			if(videoFrame.image != NULL)
				VuoRelease(videoFrame.image);
			double frameTime = (duration - .5) * (double)rand()/(double)RAND_MAX;
			QVERIFY(VuoVideo_getFrameAtSecond(m, frameTime, &videoFrame));
			VuoRetain(videoFrame.image);
		}

		QVERIFY(!VuoImage_isEmpty(videoFrame.image));
		VuoRelease(videoFrame.image);
		VuoRelease(m);
	}
};

QTEST_MAIN(TestVuoVideo)
#include "TestVuoVideo.moc"

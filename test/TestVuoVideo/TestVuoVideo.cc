/**
 * @file
 * TestVuoVideo interface and implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <Vuo/Vuo.h>

#include <objc/objc-runtime.h>

extern "C"
{
#include "VuoVideo.h"
}

extern bool warnedAboutSlowFlip;

/**
 * C++ RAII wrapper for NSAutoreleasePool.
 */
class AutoreleasePool
{
public:
	AutoreleasePool()
	{
		// +[NSAutoreleasePool new];
		Class poolClass = (Class)objc_getClass("NSAutoreleasePool");
		pool = ((id (*)(id, SEL))objc_msgSend)((id)poolClass, sel_getUid("new"));
	}

	~AutoreleasePool()
	{
		// -[NSAutoreleasePool drain];
		((void (*)(id, SEL))objc_msgSend)(pool, sel_getUid("drain"));
	}

private:
	id pool;
};

class TestVuoVideo : public QObject
{
	const int AVFOUNDATION_OPTIMIZED = 0;

	/// AvFoundation loads asynchronously, this is the longest time to allow (in ms) prior to failing.
	const unsigned int MAX_VIDEO_LOAD_TIME = 20000;

	/// Video / audio timestamp delta in either direction shouldn't ever exceed .05 (video -.045 behind audio or video .022 ahead is noticeable)
	// const double MAX_AUDIO_DRIFT = .05;

	Q_OBJECT
	// Q_DECLARE_METATYPE(VuoVideoOptimization)

	bool isNetworkStream(QString url)
	{
		return url.startsWith("rtsp://");
	}

	bool exists(QString &url)
	{
		if (isNetworkStream(url))
			return false;

		if (QFile(url).exists())
			return true;

		QString homeDir = getenv("HOME");
		url = url.replace("/MovieGauntlet/", homeDir + "/Movies/Test Movies/");
		if (QFile(url).exists())
			return true;

		return false;
	}

private slots:

	void createData(int optimize)
	{
		QTest::addColumn<QString>("url");
		QTest::addColumn<double>("expectedDuration");  // in seconds, or 0 if the movie file isn't expected to load at all.
		QTest::addColumn<int>("expectedFrameCount");
		QTest::addColumn<int>("expectedPixelsWide");
		QTest::addColumn<int>("expectedPixelsHigh");
		QTest::addColumn<int>("expectedAudioChannels");	// negative if expected to contain some packets that are completely silent; 999 to decode audio but skip checking the number of channels
		QTest::addColumn<int>("optimize");	// test both AvFoundation and Ffmpeg (either can fall back on the other - this tests that too)

		//																																																  duration    frames  width   height  audio
		QTest::newRow(VuoText_format("Apple ProRes 4444 opt=%d",optimize))						<< "/MovieGauntlet/rugged_terrain_prores4444.mov"														<<  30.		<<  899	<< 1280 <<  720 <<  0 	<< optimize;
		QTest::newRow(VuoText_format("Animated GIF opt=%d",optimize))							<< "/MovieGauntlet/count.gif"																			<<  10.1	<<  85	<<  748 <<  491	<<  0 	<< optimize;
		QTest::newRow(VuoText_format("DIF DV NTSC opt=%d",optimize))							<< "/MovieGauntlet/Audio Codecs/Quicktime Player 7/french.dv"											<<  19.9	<<  596	<<  720 <<  480	<<  2 	<< optimize;
		QTest::newRow(VuoText_format("MPEG Transport Stream opt=%d",optimize))					<< "/MovieGauntlet/interlaced/SD_NTSC_29.97_640x480.ts"													<<  28.7	<<  860	<<  640 <<  480	<< -2 	<< optimize;
		QTest::newRow(VuoText_format("MPEG v2 opt=%d",optimize))								<< "/MovieGauntlet/interlaced/interlace_test2.mpeg"														<<  39.9	<<  997	<<  720 <<  576	<< -2 	<< optimize;
		QTest::newRow(VuoText_format("MPEG v4 1 opt=%d",optimize))								<< "/MovieGauntlet/Audio Codecs/Miro Video Converter/french.large1080p.mp4"								<<  24.0	<<  585	<<  320 <<  240	<<  2 	<< optimize;
		QTest::newRow(VuoText_format("MPEG v4 AVC opt=%d",optimize))							<< "/MovieGauntlet/audio+video synchronization/Lip Sync Test Markers.m4v"								<<  67.9	<< 2044	<<  640 <<  480	<<  2 	<< optimize;
		QTest::newRow(VuoText_format("MPEG v4 HE AAC opt=%d",optimize))							<< "/MovieGauntlet/Audio Codecs/fish-mpeg4-he-aac-mono.mp4"												<<  20.0	<<  598	<< 1280 <<  720 <<  999	<< optimize;
		QTest::newRow(VuoText_format("MPEG v4 H.264 AAC opt=%d",optimize))                      << "/MovieGauntlet/mp4/cremaschi AUP ANIMO Festiwal 2014.mp4"                                           <<   6.2    <<  156 << 1280 <<  720 <<  2   << optimize;
		QTest::newRow(VuoText_format("Ogg - Theora - Ogg LR opt=%d",optimize))					<< "/MovieGauntlet/Audio Codecs/Miro Video Converter/french.oggtheora.ogv"								<<  24.0	<< 	503	<<  320 <<  240	<<  2 	<< optimize;
		QTest::newRow(VuoText_format("Ogg - Theora - Ogg 5.1 opt=%d",optimize))					<< "/MovieGauntlet/Audio Codecs/Quicktime Player 7/french.ogg"											<<  19.5	<<  466	<<  320 <<  240	<< -6 	<< optimize;
		QTest::newRow(VuoText_format("QuickTime - Animation - None opt=%d",optimize))			<< "/MovieGauntlet/demo to mov3.mov"																	<<   1.0	<<   29	<<  640 <<  480	<<  0 	<< optimize;
		QTest::newRow(VuoText_format("QuickTime - H.264 - Lossless 5.1 - #1 opt=%d",optimize))	<< "/MovieGauntlet/Audio Codecs/Compressor 4.1.3/french — H.264 — Apple Lossless 5.1.mov"				<<  19.5	<<  469	<< 1920 << 3240	<< -6 	<< optimize;
		QTest::newRow(VuoText_format("QuickTime - H.264 - Lossless 5.1 - #2 opt=%d",optimize))	<< "/MovieGauntlet/Audio Codecs/Compressor 4.1.3/typewriter-barnyard — H.264 — Apple Lossless 5.1.mov"	<<  15.7	<<  376	<< 1440 <<  900	<< -6 	<< optimize;
		QTest::newRow(VuoText_format("QuickTime - H.264 - Lossless LR opt=%d",optimize))		<< "/MovieGauntlet/out-sd.mov"																			<< 275.2	<< 6608	<<  640 <<  360	<< -2 	<< optimize;
		QTest::newRow(VuoText_format("QuickTime - Photo JPEG - None opt=%d",optimize))			<< "/System/Library/Compositions/Fish.mov"																<<  13.2	<<  397	<<  640 <<  480	<<  0 	<< optimize;
		// avfoundation seems to lock up and wreck subsequent instances of AVAssetReader when trying to load/unload this file
		// QTest::newRow(VuoText_format("QuickTime - Photo JPEG - PCM LR opt=%d",optimize))		<< "/MovieGauntlet/pbourke/2.mov"																		<<   3.3	<<  100	<< 4096 << 2048	<<  2 	<< optimize;
		QTest::newRow(VuoText_format("WebM opt=%d",optimize))									<< "/MovieGauntlet/Audio Codecs/Miro Video Converter/french.webmhd.webm"								<<  19.9	<<  474	<<  320 <<  240	<<  2 	<< optimize;

		QTest::newRow(VuoText_format("QuickTime - AVFBatch - HapM opt=%d",optimize))			<< "/MovieGauntlet/Hap/crawling-avfbatch-HapM.mov"														<<  10.5	<<  315	<<  320 <<  240	<<  2 	<< optimize;
		QTest::newRow(VuoText_format("QuickTime - AVFBatch - HapM 12 chunks opt=%d",optimize))	<< "/MovieGauntlet/Hap/crawling-avfbatch-HapM-12chunks.mov"												<<  10.5	<<  315	<<  320 <<  240	<<  2 	<< optimize;
		QTest::newRow(VuoText_format("QuickTime - FFmpeg - Hap1 opt=%d",optimize))				<< "/MovieGauntlet/Hap/crawling-ffmpeg-Hap1.mov"														<<  10.5	<<  315	<<  320 <<  240	<<  2 	<< optimize;
		QTest::newRow(VuoText_format("QuickTime - FFmpeg - Hap1 12 chunks opt=%d",optimize))	<< "/MovieGauntlet/Hap/crawling-ffmpeg-Hap1-12chunks.mov"												<<  10.5	<<  315	<<  320 <<  240	<<  2 	<< optimize;
		QTest::newRow(VuoText_format("QuickTime - FFmpeg - Hap5 opt=%d",optimize))				<< "/MovieGauntlet/Hap/crawling-ffmpeg-Hap5.mov"														<<  10.5	<<  315	<<  320 <<  240	<<  2 	<< optimize;
		QTest::newRow(VuoText_format("QuickTime - QT7 - HapY opt=%d",optimize))					<< "/MovieGauntlet/Hap/crawling-qt7-HapY.mov"															<<  10.5	<<  315	<<  320 <<  240	<<  0 	<< optimize;
		QTest::newRow(VuoText_format("QuickTime - Hap Sample Pack - Hap1 opt=%d",optimize))     << "/MovieGauntlet/Hap/(other)/Hap Sample Pack One 1080p/Movies/Gadane Fega Hap HD.mov"                 <<   8.0    <<  240 << 1920 << 1080 <<  2   << optimize;
		QTest::newRow(VuoText_format("no video track - AIFF opt=%d",optimize))                  << "/System/Library/Sounds/Sosumi.aiff"                                                                 <<   0.     <<    0 <<    0 <<    0 <<  0   << optimize;

		// Disabled for now since the server is often down.
//		QTest::newRow(VuoText_format("RTSP - H.264 opt=%d",optimize))                           << "rtsp://184.72.239.149/vod/mp4:BigBuckBunny_115k.mov"                                               << 6627.56  <<  0 <<  0 <<  0 <<  0   << optimize;
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

		if (!exists(url))
			QSKIP(QString("Test movie '%1' not found").arg(url).toUtf8().data());

		QBENCHMARK
		{
			double duration;
			VuoVideo decoder = VuoVideo_make(VuoText_make(url.toUtf8().data()), optimize == AVFOUNDATION_OPTIMIZED ? VuoVideoOptimization_Forward : VuoVideoOptimization_Random);
			QVERIFY(decoder != NULL);
			VuoRetain(decoder);
			if (expectedDuration > 0)
				QTRY_COMPARE_WITH_TIMEOUT(VuoVideo_isReady(decoder), true, MAX_VIDEO_LOAD_TIME);
			duration = VuoVideo_getDuration(decoder);
			if (expectedDuration > 0)
				QVERIFY(duration);
			VuoRelease(decoder);
			QVERIFY2(fabs(duration - expectedDuration) < 1, QString("expected %1, got %2").arg(expectedDuration).arg(duration).toUtf8().data());
		}
	}

	/**
	 * Tests repeatedly calling `VuoVideo_nextVideoFrame()`, like `vuo.video.play` does.
	 */
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

		if (!exists(url))
			QSKIP(QString("Test movie '%1' not found").arg(url).toUtf8().data());
		if (isNetworkStream(url))
			QSKIP("Not running this test on an RTSP stream since it would take too long");

		warnedAboutSlowFlip = false;

		VuoVideo m = VuoVideo_make(VuoText_make(url.toUtf8().data()), optimize == AVFOUNDATION_OPTIMIZED ? VuoVideoOptimization_Forward : VuoVideoOptimization_Random);
		QVERIFY(m != NULL);
		VuoRetain(m);
		if (expectedFrameCount > 0)
			QTRY_COMPARE_WITH_TIMEOUT( VuoVideo_isReady(m), true, MAX_VIDEO_LOAD_TIME);

		/// if the video has an audio channel that contains nothing, don't try to read it since
		/// otherwise the decoder will try to sync video frames with it which can be slower and
		/// potentially throw off the frame count (dup or skipped frames while syncing)
		VuoVideo_setPlaybackRate(m, expectedAudioChannels > 0 ? 1 : 0);

		/// make sure audio channels are correct
		int audioChannels = (int) VuoVideo_getAudioChannels(m);
		if (expectedAudioChannels != 999)
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
				QVERIFY(VuoVideo_seekToSecond(m, 0));
				videoFrameCount = 0;
				audioFrameCount = 0;
				emptyAudioFrames = 0;

				do
				{
					AutoreleasePool pool;

					VuoVideoFrame videoFrame;

					gotVideo = VuoVideo_nextVideoFrame(m, &videoFrame);

					VuoAudioFrame audioFrame = VuoAudioFrame_make(VuoListCreate_VuoAudioSamples(), 0);
					VuoRetain(audioFrame.channels);

					bool gotAudio = VuoVideo_nextAudioFrame(m, &audioFrame);

					QVERIFY(gotAudio);

					int receivedChannels = VuoListGetCount_VuoAudioSamples(audioFrame.channels);

					if (expectedAudioChannels != 999)
						QCOMPARE(receivedChannels, abs(expectedAudioChannels));

					audioFrameCount++;

					for (int i = 1; i <= receivedChannels; ++i)
					{
						if(VuoAudioSamples_isEmpty(VuoListGetValue_VuoAudioSamples(audioFrame.channels, i)))
							emptyAudioFrames++;
					}

					if(gotVideo)
					{
						videoFrameCount++;
						VuoRelease(videoFrame.image);
					}

					VuoRelease(audioFrame.channels);

				} while(gotVideo);

				QVERIFY( emptyAudioFrames < audioFrameCount * abs(expectedAudioChannels) );
			}
		}

		/// now run through alll samples again without bothering audio
		VuoVideo_setPlaybackRate(m, 0);

		VuoImage lastImage = NULL;

		QBENCHMARK
		{
			bool seeked = VuoVideo_seekToSecond(m, 0);
			if (expectedFrameCount > 0)
				QVERIFY(seeked);
			videoFrameCount = 0;

			do
			{
				AutoreleasePool pool;

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
			QVERIFY2(abs(expectedFrameCount - videoFrameCount) <= expectedFrameCount * .03, QString("expected ~%1 frames, got %2").arg(expectedFrameCount).arg(videoFrameCount).toUtf8().data());
		}

		QCOMPARE((bool)lastImage, (bool)expectedPixelsWide);
		if (lastImage)
		{
			QCOMPARE(lastImage->pixelsWide, (unsigned long)expectedPixelsWide);
			QCOMPARE(lastImage->pixelsHigh, (unsigned long)expectedPixelsHigh);
			QVERIFY(!VuoImage_isEmpty(lastImage));
			VuoRelease(lastImage);
		}

		VuoRelease(m);

		QVERIFY(!warnedAboutSlowFlip);
	}

	/**
	 * Tests repeatedly calling `VuoVideo_getFrameAtSecond()` with random timestamps,
	 * like `vuo.video.decodeImage` does with arbitrary `frameTime` values.
	 */
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
		QFETCH(int, expectedFrameCount);
		QFETCH(int, expectedPixelsWide);
		// QFETCH(int, expectedAudioChannels);
		VUserLog("[%s] optimize=%s",url.toUtf8().data(), (optimize == AVFOUNDATION_OPTIMIZED ? "AVFoundation" : "FFMPEG"));

		if (!exists(url))
			QSKIP(QString("Test movie '%1' not found").arg(url).toUtf8().data());
		if (isNetworkStream(url))
			QSKIP("Not running this test on an RTSP stream since it would take too long");

		warnedAboutSlowFlip = false;

		VuoVideo m = VuoVideo_make(VuoText_make(url.toUtf8().data()), optimize == AVFOUNDATION_OPTIMIZED ? VuoVideoOptimization_Forward : VuoVideoOptimization_Random);
		VuoRetain(m);
		QVERIFY(m != NULL);

		// when seeking around with VuoVideo_getFrameAtSecond it's faster to ignore audio entirely.
		VuoVideo_setPlaybackRate(m, 0);

		if (expectedFrameCount > 0)
			QTRY_COMPARE_WITH_TIMEOUT(VuoVideo_isReady(m), true, MAX_VIDEO_LOAD_TIME);

		double duration = VuoVideo_getDuration(m);

		VuoVideoFrame videoFrame = {nullptr, 0, 0};
		QBENCHMARK
		{
			AutoreleasePool pool;

			if(videoFrame.image != NULL)
				VuoRelease(videoFrame.image);
			double frameTime = (duration - .5) * (double)rand()/(double)RAND_MAX;
			bool gotFrame    = VuoVideo_getFrameAtSecond(m, frameTime, &videoFrame);
			QCOMPARE(gotFrame, (bool)expectedPixelsWide);
			if (expectedPixelsWide)
				VuoRetain(videoFrame.image);
		}

		QCOMPARE(!VuoImage_isEmpty(videoFrame.image), (bool)expectedPixelsWide);
		if (expectedPixelsWide)
			VuoRelease(videoFrame.image);
		VuoRelease(m);

		QVERIFY(!warnedAboutSlowFlip);
	}

	/**
	 * Tests repeatedly calling `VuoVideo_getFrameAtSecond()` with sequential timestamps,
	 * like `vuo.video.decodeImage` does when its `frameTime` port is connected to `requestedFrame`.
	 */
	void testSequentialDecodePerformance_data()
	{
		printf("	testSequentialDecodePerformance()\n"); fflush(stdout);
		createData(0);
	}

	void testSequentialDecodePerformance()
	{
		QFETCH(QString, url);
		QFETCH(int, expectedFrameCount);
		QFETCH(int, expectedPixelsWide);

		if (!exists(url))
			QSKIP(QString("Test movie '%1' not found").arg(url).toUtf8().data());
		if (isNetworkStream(url))
			QSKIP("Not running this test on an RTSP stream since it would take too long");

		if (url.contains("/french — H.264 — Apple Lossless 5.1.mov"))
		{
			// I think this video has a lack of chapters or whatever they're called, which makes seeking really slow.
			// @todo Look into speeding this scenario up.
			QSKIP("Skipping QuickTime - H.264 - Lossless 5.1 - #1");
		}

		warnedAboutSlowFlip = false;

		VuoVideo m = VuoVideo_make(VuoText_make(url.toUtf8().data()), VuoVideoOptimization_Random);
		VuoRetain(m);

		QVERIFY(m != NULL);

		VuoVideo_setPlaybackRate(m, 0);

		if (expectedFrameCount > 0)
			QTRY_COMPARE_WITH_TIMEOUT(VuoVideo_isReady(m), true, MAX_VIDEO_LOAD_TIME);

		double duration = VuoVideo_getDuration(m) - .5;
		VuoVideoFrame videoFrame = {nullptr, 0, 0};

		int64_t max = duration * 1000;

		double fps = 30;
		double frame = 1/fps;

		QElapsedTimer timer;
		timer.start();

		for(int i = 0; i < duration * fps; i++)
		{
			AutoreleasePool pool;

			double sec = i * frame;

			if(videoFrame.image != NULL)
				VuoRelease(videoFrame.image);

			if(VuoVideo_getFrameAtSecond(m, sec, &videoFrame))
				VuoRetain(videoFrame.image);

			int64_t elapsed = timer.elapsed();
			const double leeway = 2;
			if (elapsed > max * leeway)
				QFAIL(QString("Taking too long to decode; after %1s (more than %2x as long as the realtime duration of the movie) we're only %3% done decoding.")
					.arg(elapsed/1000.)
					.arg(leeway)
					.arg(i * 100. / (duration*fps))
					.toUtf8().data());
		}

		QCOMPARE(!VuoImage_isEmpty(videoFrame.image), (bool)expectedPixelsWide);
		if (expectedPixelsWide)
			VuoRelease(videoFrame.image);
		VuoRelease(m);

		QVERIFY(!warnedAboutSlowFlip);
	}

	/**
	 * https://b33p.net/kosada/node/12217
	 * https://b33p.net/kosada/node/12227
	 * Ensures that requesting the first frame actually returns a frame with timestamp 0.
	 */
	void testDecodeFirstFrame_data()
	{
		printf("	testDecodeFirstFrame()\n"); fflush(stdout);
		createData(0);
	}
	void testDecodeFirstFrame()
	{
		QFETCH(QString, url);
		QFETCH(int, expectedFrameCount);
		QFETCH(int, expectedPixelsWide);

		if (!exists(url))
			QSKIP(QString("Test movie '%1' not found").arg(url).toUtf8().data());

		if (url.contains("/SD_NTSC_29.97_640x480.ts"))
			QSKIP("This movie starts at PTS 600");
		if (url.contains("/interlace_test2.mpeg"))
			QSKIP("This movie starts at PTS 0.5");
		if (url.contains("/french.webmhd.webm"))
			QSKIP("This movie starts at PTS 4.125");
		if (url.contains("/out-sd.mov"))
			QSKIP("This movie starts at PTS 0.083000");

		VuoVideo video = VuoVideo_make(VuoText_make(url.toUtf8().data()), VuoVideoOptimization_Random);
		VuoRetain(video);
		QVERIFY(video);

		VuoVideo_setPlaybackRate(video, 0);

		if (expectedFrameCount > 0)
			QTRY_COMPARE_WITH_TIMEOUT(VuoVideo_isReady(video), true, MAX_VIDEO_LOAD_TIME);

		VuoVideoFrame videoFrame;
		QCOMPARE(VuoVideo_getFrameAtSecond(video, 0, &videoFrame), (bool)expectedPixelsWide);
		if (expectedPixelsWide)
			QVERIFY(videoFrame.image);

		if (!isNetworkStream(url) && expectedFrameCount)
			QVERIFY2(VuoReal_areEqual(videoFrame.timestamp, 0),
				QString("Requested timestamp 0 but actually got %1").arg(videoFrame.timestamp).toUtf8().data());

		VuoRelease(video);
	}

	/**
	 * Tests decoding out of bounds frame times.  VuoVideo_getFrameAtSecond should always return an
	 * image, clamping to first & last frames.
	 */
	void testDecodeOutOfBoundsFrame_data()
	{
		printf("	testDecodeOutOfBoundsFrame()\n"); fflush(stdout);
		createData(0);
	}

	void testDecodeOutOfBoundsFrame()
	{
		QFETCH(QString, url);
		QFETCH(int, expectedFrameCount);

		if (!exists(url))
			QSKIP(QString("Test movie '%1' not found").arg(url).toUtf8().data());
		if (isNetworkStream(url))
			QSKIP("Not running this test on an RTSP stream since it would take too long");

		VuoVideo m = VuoVideo_make(VuoText_make(url.toUtf8().data()), VuoVideoOptimization_Random);
		VuoRetain(m);
		QVERIFY(m != NULL);

		VuoVideo_setPlaybackRate(m, 0);

		if (expectedFrameCount > 0)
			QTRY_COMPARE_WITH_TIMEOUT(VuoVideo_isReady(m), true, MAX_VIDEO_LOAD_TIME);

		double duration = VuoVideo_getDuration(m);

		VuoVideoFrame videoFrame = {nullptr, 0, 0};

		if (expectedFrameCount)
		{
			QVERIFY2(VuoVideo_getFrameAtSecond(m, -1, &videoFrame), "VuoVideo_getFrameAtSecond(-1) returned false.");
			QVERIFY2(videoFrame.image != NULL, "VuoVideo_getFrameAtSecond(-1) returned null image");

			QVERIFY2(VuoVideo_getFrameAtSecond(m, duration + 10, &videoFrame), "VuoVideo_getFrameAtSecond(duration + 10) returned false.");
			QVERIFY2(videoFrame.image != NULL, "VuoVideo_getFrameAtSecond(duration + 10) returned null image");
		}

		VuoRelease(m);
	}

	void testRotoscopeAndStopComposition()
	{
		// Ensure the composition runs then stops, without hanging.
		// https://b33p.net/kosada/node/15249
		VuoCompilerIssues issues;
		VuoRunner *r = VuoCompiler::newSeparateProcessRunnerFromCompositionFile("composition/RotoscopeAndStopComposition.vuo", &issues);
		r->start();
		r->waitUntilStopped();
		delete r;
	}
};

QTEST_MAIN(TestVuoVideo)
#include "TestVuoVideo.moc"

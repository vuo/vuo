/**
 * @file
 * TestControlAndTelemetry interface and implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "TestCompositionExecution.hh"

#include <Vuo/Vuo.h>
#include <sstream>
#include <mach-o/dyld.h>


// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(list<string>);
Q_DECLARE_METATYPE(set<string>);
Q_DECLARE_METATYPE(vector<VuoRunner::Port *>);


/**
 * Tests the execution of compositions.
 */
class TestControlAndTelemetry : public TestCompositionExecution
{
	Q_OBJECT

private:
	VuoCompiler *compiler;

	/**
	 * Builds the composition at @a compositionPath into an executable. Returns the executable file path.
	 */
	static string buildExecutableForNewProcess(VuoCompiler *compiler, const string &compositionPath,
											   VuoCompilerComposition **composition = NULL,
											   VuoCompiler::Optimization optimization = VuoCompiler::Optimization_FastBuild)
	{
		string dir, file, extension;
		VuoFileUtilities::splitPath(compositionPath, dir, file, extension);
		string bcPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string exePath = VuoFileUtilities::makeTmpFile(file, "");

		if (composition)
		{
			VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);
			*composition = new VuoCompilerComposition(new VuoComposition, parser);
			VuoCompilerIssues issues;
			compiler->compileComposition(*composition, bcPath, true, &issues);
			delete parser;
		}
		else
		{
			VuoCompilerIssues issues;
			compiler->compileComposition(compositionPath, bcPath, true, &issues);
		}
		compiler->linkCompositionToCreateExecutable(bcPath, exePath, optimization);
		remove(bcPath.c_str());

		return exePath;
	}

	/**
	 * Builds the composition at @a compositionPath into an executable. Returns a newly allocated `VuoRunner` for the executable.
	 */
	static VuoRunner * createRunnerInNewProcess(VuoCompiler *compiler, const string &compositionPath,
												VuoCompilerComposition **composition = NULL,
												VuoCompiler::Optimization optimization = VuoCompiler::Optimization_FastBuild)
	{
		string exePath = buildExecutableForNewProcess(compiler, compositionPath, composition, optimization);
		VuoRunner * runner = VuoRunner::newSeparateProcessRunnerFromExecutable(exePath, "", false, true);
		runner->setDelegate(new TestRunnerDelegate());
		return runner;
	}

	VuoRunner * createRunnerInNewProcess(const string &compositionPath, VuoCompilerComposition **composition = NULL,
										 VuoCompiler::Optimization optimization = VuoCompiler::Optimization_FastBuild)
	{
		return createRunnerInNewProcess(compiler, compositionPath, composition, optimization);
	}

	/**
	 * Builds the composition at @a compositionPath into a dylib.
	 */
	void buildDylibForNewProcess(const string &compositionPath, string &compositionLoaderPath, string &dylibPath,
								 VuoRunningCompositionLibraries *runningCompositionLibraries, string &compositionDir)
	{
		string file, extension;
		VuoFileUtilities::splitPath(compositionPath, compositionDir, file, extension);
		string bcPath = VuoFileUtilities::makeTmpFile(file, "bc");
		dylibPath = VuoFileUtilities::makeTmpFile(file, "dylib");

		VuoCompilerIssues issues;
		compiler->compileComposition(compositionPath, bcPath, true, &issues);

		compiler->linkCompositionToCreateDynamicLibraries(bcPath, dylibPath, runningCompositionLibraries);
		compositionLoaderPath = compiler->getCompositionLoaderPath();
		remove(bcPath.c_str());
	}

	/**
	 * Builds the composition at @a compositionPath into a dylib. Returns a newly allocated `VuoRunner` for the dylib.
	 */
	VuoRunner * createRunnerInNewProcessWithDylib(const string &compositionPath)
	{
		string compositionLoaderPath, dylibPath, compositionDir;
		std::shared_ptr<VuoRunningCompositionLibraries> runningCompositionLibraries = std::make_shared<VuoRunningCompositionLibraries>();
		buildDylibForNewProcess(compositionPath, compositionLoaderPath, dylibPath, runningCompositionLibraries.get(), compositionDir);

		VuoRunner *runner = VuoRunner::newSeparateProcessRunnerFromDynamicLibrary(compositionLoaderPath, dylibPath, runningCompositionLibraries, compositionDir, false, true);
		// runner->setDelegate(new TestRunnerDelegate());  /// @todo https://b33p.net/kosada/node/6021
		return runner;
	}

	/**
	 * Builds the composition at @a compositionPath into a dylib. Returns the dylib file path.
	 */
	string buildDylibForCurrentProcess(const string &compositionPath,
									   VuoCompiler::Optimization optimization = VuoCompiler::Optimization_FastBuild)
	{
		string compositionDir, file, extension;
		VuoFileUtilities::splitPath(compositionPath, compositionDir, file, extension);
		string bcPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string dylibPath = VuoFileUtilities::makeTmpFile(file, "dylib");

		VuoCompilerIssues issues;
		compiler->compileComposition(compositionPath, bcPath, true, &issues);
		compiler->linkCompositionToCreateDynamicLibrary(bcPath, dylibPath, optimization);
		remove(bcPath.c_str());

		return dylibPath;
	}

	/**
	 * Builds the composition at @a compositionPath into a dylib. Returns a newly allocated `VuoRunner` for the dylib.
	 */
	VuoRunner * createRunnerInCurrentProcess(const string &compositionPath,
											 VuoCompiler::Optimization optimization = VuoCompiler::Optimization_FastBuild)
	{
		string dylibPath = buildDylibForCurrentProcess(compositionPath, optimization);

		string compositionDir, file, extension;
		VuoFileUtilities::splitPath(compositionPath, compositionDir, file, extension);

		VuoRunner * runner = VuoRunner::newCurrentProcessRunnerFromDynamicLibrary(dylibPath, compositionDir, true);
		// runner->setDelegate(new TestRunnerDelegate());  /// @todo https://b33p.net/kosada/node/6021
		return runner;
	}

	/**
	 * Builds the composition at @a compositionPath into a dylib plus resource dylibs for live coding. Returns a newly
	 * allocated `VuoRunner` for the dylibs.
	 */
	VuoRunner * createRunnerForLiveCoding(const string &compositionPath, VuoCompilerComposition *&composition,
										  std::shared_ptr<VuoRunningCompositionLibraries> &runningCompositionLibraries)
	{
		string dir, file, ext;
		VuoFileUtilities::splitPath(compositionPath, dir, file, ext);
		string bcPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string dylibPath = VuoFileUtilities::makeTmpFile(file, "dylib");

		VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(compositionPath, compiler);
		VuoComposition *baseComposition = new VuoComposition();
		composition = new VuoCompilerComposition(baseComposition, parser);
		delete parser;

		VuoCompilerIssues issues;
		compiler->compileComposition(composition, bcPath, true, &issues);
		runningCompositionLibraries = std::make_shared<VuoRunningCompositionLibraries>();
		compiler->linkCompositionToCreateDynamicLibraries(bcPath, dylibPath, runningCompositionLibraries.get());
		remove(bcPath.c_str());

		return VuoRunner::newSeparateProcessRunnerFromDynamicLibrary(compiler->getCompositionLoaderPath(), dylibPath, runningCompositionLibraries, dir, false, true);
	}

	/**
	 * Builds @a composition into a dylib plus resource dylib for live coding, and replaces the running composition with it.
	 */
	void replaceCompositionForLiveCoding(const string &compositionPath, VuoCompilerComposition *composition,
										 VuoRunningCompositionLibraries *runningCompositionLibraries,
										 const string &oldCompositionSource, VuoCompilerCompositionDiff *diffInfo,
										 VuoRunner *runner)
	{
		string dir, file, ext;
		VuoFileUtilities::splitPath(compositionPath, dir, file, ext);
		string bcPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string dylibPath = VuoFileUtilities::makeTmpFile(file, "dylib");

		VuoCompilerIssues issues;
		compiler->compileComposition(composition, bcPath, true, &issues);
		compiler->linkCompositionToCreateDynamicLibraries(bcPath, dylibPath, runningCompositionLibraries);
		remove(bcPath.c_str());

		if (! diffInfo)
			diffInfo = new VuoCompilerCompositionDiff();
		string compositionDiff = diffInfo->diff(oldCompositionSource, composition, compiler);
		runner->replaceComposition(dylibPath, compositionDiff);
		delete diffInfo;
	}

	/**
	 * Helper class for running WriteTimesToFile.vuo.
	 */
	class WriteTimesToFileHelper
	{
	private:
		string path;

	public:
		void deleteFile(void) const
		{
			remove(path.c_str());
			QVERIFY(! VuoFileUtilities::fileExists(path));
		}

		WriteTimesToFileHelper(void)
		{
			path = VuoFileUtilities::makeTmpFile("WriteTimesToFile", "txt");
			setenv("TESTCONTROLANDTELEMETRY_WRITETIMESTOFILE", path.c_str(), 1);  // Pass to vuo.test.writeTimeToFile without using a published input port
			deleteFile();
		}

		~WriteTimesToFileHelper(void)
		{
			deleteFile();
		}

		/**
		 * Waits until a file exists at the path.
		 */
		void waitUntilFileExists(void) const
		{
			bool isExistent = false;
			int MAX_NUM_TRIES = 200;
			for (int i = 0; i < MAX_NUM_TRIES; ++i)
			{
				if (VuoFileUtilities::fileExists(path))
				{
					isExistent = true;
					break;
				}
				usleep(USEC_PER_SEC / 100);
			}
			QVERIFY(isExistent);
		}

		/**
		 * Checks that the file contains at least one timestamp, and all timestamps are between the two given timestamps.
		 */
		void checkTimesInFile(double beforeTime, double afterTime) const
		{
			ifstream fin(path.c_str());
			bool isEmpty = true;
			double readTime;
			while (fin >> readTime)
			{
				ostringstream sout;
				sout << std::fixed;  // not scientific notation
				sout << beforeTime << " < " << readTime << " < " << afterTime;
				QVERIFY2(beforeTime < readTime && readTime < afterTime, sout.str().c_str());
				isEmpty = false;
			}
			QVERIFY(! isEmpty);
			fin.close();
		}
	};

private slots:

	void initTestCase()
	{
		compiler = initCompiler();
	}

	void cleanupTestCase()
	{
		delete compiler;
	}

	void testStartingAndStoppingComposition_data()
	{
		QTest::addColumn<int>("testNum");

		int testNum = 0;
		QTest::newRow("New process, executable, no cache") << testNum++;
		QTest::newRow("New process, executable, cache") << testNum++;
		QTest::newRow("New process, dylib, cache") << testNum++;
		QTest::newRow("Current process, runOnMainThread(), no cache") << testNum++;
		QTest::newRow("Current process, runOnMainThread(), cache") << testNum++;
		QTest::newRow("Current process, drainMainDispatchQueue(), cache") << testNum++;
		QTest::newRow("Error handling: New process, runOnMainThread()") << testNum++;
		QTest::newRow("Error handling: Current process, runOnMainThread() on non-main thread") << testNum++;
		QTest::newRow("Error handling: New process, non-existent executable") << testNum++;
		QTest::newRow("Error handling: New process, non-existent dylib") << testNum++;
		QTest::newRow("Error handling: Current process, non-existent dylib") << testNum++;
	}
	void testStartingAndStoppingComposition()
	{
		QFETCH(int, testNum);

		string compositionPath = getCompositionPath("WriteTimesToFile.vuo");

		string nonExistentFile = "nonexistent";
		QVERIFY(! VuoFileUtilities::fileExists(nonExistentFile));

		if (testNum == 0 || testNum == 1)  // New process, executable
		{
			VuoCompiler::Optimization optimization = (testNum == 0 ?
														  VuoCompiler::Optimization_SmallBinary : VuoCompiler::Optimization_FastBuild);

			WriteTimesToFileHelper helper;

			VuoRunner *runner = createRunnerInNewProcess(compositionPath, NULL, optimization);
			double beforeStartTime = VuoTimeUtilities::getCurrentTimeInSeconds();
			runner->start();
			helper.waitUntilFileExists();
			runner->stop();
			double afterStopTime = VuoTimeUtilities::getCurrentTimeInSeconds();
			delete runner;

			helper.checkTimesInFile(beforeStartTime, afterStopTime);
		}
		else if (testNum == 2)  // New process, dylib
		{
			WriteTimesToFileHelper helper;

			VuoRunner *runner = createRunnerInNewProcessWithDylib(compositionPath);
			double beforeStartTime = VuoTimeUtilities::getCurrentTimeInSeconds();
			runner->start();
			helper.waitUntilFileExists();
			runner->stop();
			double afterStopTime = VuoTimeUtilities::getCurrentTimeInSeconds();
			delete runner;

			helper.checkTimesInFile(beforeStartTime, afterStopTime);
		}
		else if (testNum == 3 || testNum == 4)  // Current process, runOnMainThread()
		{
			VuoCompiler::Optimization optimization = (testNum == 3 ?
														  VuoCompiler::Optimization_SmallBinary : VuoCompiler::Optimization_FastBuild);

			WriteTimesToFileHelper *helper = new WriteTimesToFileHelper;

			VuoRunner *runner = createRunnerInCurrentProcess(compositionPath, optimization);
			double beforeStartTime = VuoTimeUtilities::getCurrentTimeInSeconds();
			runner->start();
			dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
							   helper->waitUntilFileExists();
							   runner->stop();
						   });
			runner->runOnMainThread();
			double afterStopTime = VuoTimeUtilities::getCurrentTimeInSeconds();
			delete runner;

			helper->checkTimesInFile(beforeStartTime, afterStopTime);
			delete helper;
		}
		else if (testNum == 5)  // Current process, drainMainDispatchQueue()
		{
			WriteTimesToFileHelper *helper = new WriteTimesToFileHelper;

			VuoRunner *runner = createRunnerInCurrentProcess(compositionPath);
			double beforeStartTime = VuoTimeUtilities::getCurrentTimeInSeconds();
			runner->start();
			dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
							   helper->waitUntilFileExists();
							   runner->stop();
			});
			while (! runner->isStopped())
			{
				runner->drainMainDispatchQueue();
				usleep(USEC_PER_SEC / 1000);
			}
			double afterStopTime = VuoTimeUtilities::getCurrentTimeInSeconds();
			delete runner;

			helper->checkTimesInFile(beforeStartTime, afterStopTime);
			delete helper;
		}
		else if (testNum == 6)  // Error handling: New process, runOnMainThread()
		{
			VuoRunner *runner = createRunnerInNewProcess(compositionPath);
			runner->start();
			try
			{
				runner->runOnMainThread();
				QFAIL("Exception not thrown for runOnMainThread() with new process.");
			}
			catch (VuoException) { }
			runner->stop();
			delete runner;
		}
		else if (testNum == 7)  // Error handling: Current process, runOnMainThread() on non-main thread
		{
			VuoRunner *runner = createRunnerInCurrentProcess(compositionPath);
			runner->start();
			dispatch_group_t group = dispatch_group_create();
			dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
			dispatch_group_async(group, queue, ^{
									 try
									 {
										 runner->runOnMainThread();
										 QFAIL("Exception not thrown for runOnMainThread() run on non-main thread.");
									 }
									 catch (VuoException) { }
								 });
			dispatch_group_wait(group, DISPATCH_TIME_FOREVER);
			dispatch_async(queue, ^{
							   runner->stop();
						   });
			runner->runOnMainThread();
			delete runner;
		}
		else if (testNum == 8)  // Error handling: New process, non-existent executable
		{
			VuoRunner *runner = VuoRunner::newSeparateProcessRunnerFromExecutable(nonExistentFile, "", false, false);
			runner->start();
			runner->waitUntilStopped();
			delete runner;
		}
		else if (testNum == 9)  // Error handling: New process, non-existent dylib
		{
			std::shared_ptr<VuoRunningCompositionLibraries> runningCompositionLibraries = std::make_shared<VuoRunningCompositionLibraries>();
			VuoRunner *runner = VuoRunner::newSeparateProcessRunnerFromDynamicLibrary(compiler->getCompositionLoaderPath(),
																					  nonExistentFile, runningCompositionLibraries,
																					  "", false, false);
			runner->start();
			runner->waitUntilStopped();
			delete runner;
		}
		else if (testNum == 10)  // Error handling: Current process, non-existent dylib
		{
			VuoRunner *runner = VuoRunner::newCurrentProcessRunnerFromDynamicLibrary(nonExistentFile, "", false);
			runner->start();
			while (! runner->isStopped())
			{
				runner->drainMainDispatchQueue();
				usleep(USEC_PER_SEC / 1000);
			}
			delete runner;
		}
		else
		{
			QFAIL("");
		}
	}

	void testRunningMultipleCompositionInstancesSequentially_data()
	{
		QTest::addColumn<int>("testNum");

		int testNum = 0;
		QTest::newRow("New process, executable") << testNum++;
		QTest::newRow("New process, dylib") << testNum++;
		QTest::newRow("Current process") << testNum++;
	}
	void testRunningMultipleCompositionInstancesSequentially()
	{
		QFETCH(int, testNum);

		string compositionPath = getCompositionPath("WriteTimesToFile.vuo");
		string exePath, compositionLoaderPath, dylibPath, compositionDir;
		std::shared_ptr<VuoRunningCompositionLibraries> runningCompositionLibraries[2] = { nullptr, nullptr };

		if (testNum == 0)  // New process, executable
		{
			exePath = buildExecutableForNewProcess(compiler, compositionPath);
		}
		else if (testNum == 1)  // New process, dylib
		{
			runningCompositionLibraries[0] = std::make_shared<VuoRunningCompositionLibraries>();
			buildDylibForNewProcess(compositionPath, compositionLoaderPath, dylibPath, runningCompositionLibraries[0].get(), compositionDir);
			runningCompositionLibraries[1] = std::make_shared<VuoRunningCompositionLibraries>(*runningCompositionLibraries[0].get());
		}
		else if (testNum == 2)  // Current process
		{
			dylibPath = buildDylibForCurrentProcess(compositionPath);
		}

		for (int i = 0; i < 2; ++i)
		{
			WriteTimesToFileHelper *helper = new WriteTimesToFileHelper;
			double beforeStartTime, afterStopTime;

			bool deleteBinaries = (i == 1);

			if (testNum == 0 || testNum == 1)  // New process
			{
				VuoRunner *runner;
				if (testNum == 0)
				{
					runner = VuoRunner::newSeparateProcessRunnerFromExecutable(exePath, "", false, deleteBinaries);
					runner->setDelegate(new TestRunnerDelegate());
				}
				else
				{
					runner = VuoRunner::newSeparateProcessRunnerFromDynamicLibrary(compositionLoaderPath, dylibPath, runningCompositionLibraries[i], compositionDir, false, deleteBinaries);
					// runner->setDelegate(new TestRunnerDelegate());  /// @todo https://b33p.net/kosada/node/6021
				}

				beforeStartTime = VuoTimeUtilities::getCurrentTimeInSeconds();
				runner->start();
				helper->waitUntilFileExists();
				runner->stop();
				afterStopTime = VuoTimeUtilities::getCurrentTimeInSeconds();
				delete runner;
			}
			else  // Current process
			{
				VuoRunner * runner = VuoRunner::newCurrentProcessRunnerFromDynamicLibrary(dylibPath, compositionDir, deleteBinaries);
				// runner->setDelegate(new TestRunnerDelegate());  /// @todo https://b33p.net/kosada/node/6021

				beforeStartTime = VuoTimeUtilities::getCurrentTimeInSeconds();
				runner->start();
				dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
								   helper->waitUntilFileExists();
								   runner->stop();
							   });
				runner->runOnMainThread();
				afterStopTime = VuoTimeUtilities::getCurrentTimeInSeconds();
				delete runner;
			}

			helper->checkTimesInFile(beforeStartTime, afterStopTime);
			delete helper;
		}
	}

	void testRunningMultipleSeparateProcessCompositionInstancesSimultaneously()
	{
		string compositionPath = getCompositionPath("Recur_Count.vuo");
		VuoCompilerIssues issues;

		VuoRunner *runner1 = VuoCompiler::newSeparateProcessRunnerFromCompositionFile(compositionPath, &issues);
		runner1->start();

		VuoRunner *runner2 = VuoCompiler::newSeparateProcessRunnerFromCompositionFile(compositionPath, &issues);
		runner2->start();

		QVERIFY(!runner1->lostContact);
		QVERIFY(!runner2->lostContact);

		runner2->stop();
		runner1->stop();
	}

	void testRunningMultipleCurrentProcessDifferentCompositionInstancesSimultaneously()
	{
		VuoCompilerIssues issues;

		VuoRunner *runner1 = VuoCompiler::newCurrentProcessRunnerFromCompositionFile(getCompositionPath("Recur_Count.vuo"), &issues);
		runner1->start();

		VuoRunner *runner2 = VuoCompiler::newCurrentProcessRunnerFromCompositionFile(getCompositionPath("Recur_Add_published.vuo"), &issues);
		runner2->start();

		QVERIFY(!runner1->lostContact);
		QVERIFY(!runner2->lostContact);

		runner2->stop();
		runner1->stop();
	}

	void testRunningMultipleCurrentProcessSameCompositionInstancesSimultaneously()
	{
		string compositionPath = getCompositionPath("Recur_Count.vuo");
		VuoCompilerIssues issues;

		VuoRunner *runner1 = VuoCompiler::newCurrentProcessRunnerFromCompositionFile(compositionPath, &issues);
		runner1->start();

		VuoRunner *runner2 = VuoCompiler::newCurrentProcessRunnerFromCompositionFile(compositionPath, &issues);
		runner2->start();

		QVERIFY(!runner1->lostContact);
		QVERIFY(!runner2->lostContact);

		runner2->stop();
		runner1->stop();
	}

	/**
	 * Make sure we can uniquify dylibs with short names.
	 *
	 * VDMX calls its files `dylib.dylib` (5-character filename, shorter than the mkstmps() `-XXXXXX`).
	 * https://b33p.net/kosada/node/12917
	 */
	void testRunningMultipleCurrentProcessSameShortNameCompositionInstancesSimultaneously()
	{
		string compositionPath = getCompositionPath("Recur_Count.vuo");

		string directory, file, extension;
		VuoFileUtilities::splitPath(compositionPath, directory, file, extension);
		string compiledCompositionPath = VuoFileUtilities::makeTmpFile(file, "bc");
		VuoCompilerIssues issues;
		compiler->compileComposition(compositionPath, compiledCompositionPath, true, &issues);

		for (int i = 1; i <= 8; ++i)
		{
			string linkedCompositionPath = VuoFileUtilities::makeTmpDir("vdmx") + "/" + string("thedylib").substr(0,i) + ".dylib";
			compiler->linkCompositionToCreateDynamicLibrary(compiledCompositionPath, linkedCompositionPath, VuoCompiler::Optimization_FastBuild);

			VuoRunner *runner1 = VuoRunner::newCurrentProcessRunnerFromDynamicLibrary(linkedCompositionPath, directory);
			runner1->start();

			VuoRunner *runner2 = VuoRunner::newCurrentProcessRunnerFromDynamicLibrary(linkedCompositionPath, directory);
			runner2->start();

			QVERIFY(!runner1->lostContact);
			QVERIFY(!runner2->lostContact);

			runner2->stop();
			runner1->stop();
		}

		remove(compiledCompositionPath.c_str());
	}

private:

	class TestRunningMultipleRunnerDelegate : public TestRunnerDelegate
	{
	public:
		map<string, string> publishedOutputData;
		dispatch_semaphore_t gotPublishedOutputEvent;

		TestRunningMultipleRunnerDelegate(void)
		{
			gotPublishedOutputEvent = dispatch_semaphore_create(0);
		}

		~TestRunningMultipleRunnerDelegate(void)
		{
			dispatch_release(gotPublishedOutputEvent);
		}

		void receivedTelemetryPublishedOutputPortUpdated(VuoRunner::Port *port, bool sentData, string dataSummary)
		{
			publishedOutputData[port->getName()] = dataSummary;
			dispatch_semaphore_signal(gotPublishedOutputEvent);
		}
	};

private slots:

	void testRunningMultipleCurrentProcessSameDylibInstancesSimultaneously()
	{
		string compositionPath = getCompositionPath("PublishedCount.vuo");

		string directory, file, extension;
		VuoFileUtilities::splitPath(compositionPath, directory, file, extension);
		string compiledCompositionPath = VuoFileUtilities::makeTmpFile(file, "bc");
		VuoCompilerIssues issues;
		compiler->compileComposition(compositionPath, compiledCompositionPath, true, &issues);
		string linkedCompositionPath = VuoFileUtilities::makeTmpFile(file, "dylib");
		compiler->linkCompositionToCreateDynamicLibrary(compiledCompositionPath, linkedCompositionPath, VuoCompiler::Optimization_FastBuild);
		remove(compiledCompositionPath.c_str());

		VuoRunner *runner1 = VuoRunner::newCurrentProcessRunnerFromDynamicLibrary(linkedCompositionPath, directory);
		VuoRunner *runner2 = VuoRunner::newCurrentProcessRunnerFromDynamicLibrary(linkedCompositionPath, directory);

		TestRunningMultipleRunnerDelegate delegate1;
		TestRunningMultipleRunnerDelegate delegate2;
		runner1->setDelegate(&delegate1);
		runner2->setDelegate(&delegate2);

		runner1->start();
		runner2->start();

		// Ensure the compositions communicate with their respective runners independently.
		{
			runner1->pause();
			QVERIFY(runner1->paused);
			QVERIFY(!runner2->paused);

			runner2->pause();
			QVERIFY(runner1->paused);
			QVERIFY(runner2->paused);

			runner1->unpause();
			QVERIFY(!runner1->paused);
			QVERIFY(runner2->paused);

			runner2->unpause();
			QVERIFY(!runner1->paused);
			QVERIFY(!runner2->paused);
		}

		// Ensure the compositions execute independently.
		{
			VuoRunner::Port *incrementPort1 = runner1->getPublishedInputPortWithName("Increment");
			map<VuoRunner::Port *, json_object *> m1;
			m1[incrementPort1] = VuoInteger_getJson(10);
			runner1->setPublishedInputPortValues(m1);
			runner1->firePublishedInputPortEvent(incrementPort1);
			dispatch_semaphore_wait(delegate1.gotPublishedOutputEvent, DISPATCH_TIME_FOREVER);
			QCOMPARE(delegate1.publishedOutputData.size(), (size_t)1);
			QCOMPARE(QString::fromStdString(delegate1.publishedOutputData["Count"]), QString("11"));
			QCOMPARE(delegate2.publishedOutputData.size(), (size_t)0);

			VuoRunner::Port *incrementPort2 = runner2->getPublishedInputPortWithName("Increment");
			map<VuoRunner::Port *, json_object *> m2;
			m2[incrementPort2] = VuoInteger_getJson(100);
			runner2->setPublishedInputPortValues(m2);
			runner2->firePublishedInputPortEvent(incrementPort2);
			dispatch_semaphore_wait(delegate2.gotPublishedOutputEvent, DISPATCH_TIME_FOREVER);
			QCOMPARE(delegate2.publishedOutputData.size(), (size_t)1);
			QCOMPARE(QString::fromStdString(delegate2.publishedOutputData["Count"]), QString("101"));
			QCOMPARE(delegate1.publishedOutputData.size(), (size_t)1);
		}

		runner2->stop();
		runner1->stop();

		remove(linkedCompositionPath.c_str());
	}

	void testPausingAndUnpausingComposition()
	{
		string compositionPath = getCompositionPath("WriteTimesToFile.vuo");
		const int PAUSE_CHECK_USEC = USEC_PER_SEC;

		{
			printf("    New process, started paused\n"); fflush(stdout);

			WriteTimesToFileHelper helper;

			VuoRunner *runner = createRunnerInNewProcess(compositionPath);
			runner->startPaused();
			usleep(PAUSE_CHECK_USEC);

			for (int i = 0; i < 2; ++i)
			{
				printf("        iteration %d\n", i);
				double beforeUnpauseTime = VuoTimeUtilities::getCurrentTimeInSeconds();
				runner->unpause();

				helper.waitUntilFileExists();
				runner->pause();
				double afterPauseTime = VuoTimeUtilities::getCurrentTimeInSeconds();
				usleep(PAUSE_CHECK_USEC);

				helper.checkTimesInFile(beforeUnpauseTime, afterPauseTime);
				helper.deleteFile();
			}

			runner->stop();
			delete runner;
		}

		{
			printf("    New process, started unpaused\n"); fflush(stdout);

			WriteTimesToFileHelper helper;

			VuoRunner *runner = createRunnerInNewProcess(compositionPath);
			double beforeUnpauseTime = VuoTimeUtilities::getCurrentTimeInSeconds();
			runner->start();

			for (int i = 0; i < 2; ++i)
			{
				printf("        iteration %d\n", i);
				if (i > 0)
				{
					beforeUnpauseTime = VuoTimeUtilities::getCurrentTimeInSeconds();
					runner->unpause();
				}

				helper.waitUntilFileExists();
				runner->pause();
				double afterPauseTime = VuoTimeUtilities::getCurrentTimeInSeconds();
				usleep(PAUSE_CHECK_USEC);

				helper.checkTimesInFile(beforeUnpauseTime, afterPauseTime);
				helper.deleteFile();
			}

			runner->stop();
			delete runner;
		}

		{
			printf("    Current process, started paused\n"); fflush(stdout);

			WriteTimesToFileHelper *helper = new WriteTimesToFileHelper;

			VuoRunner *runner = createRunnerInCurrentProcess(compositionPath);
			runner->startPaused();
			usleep(PAUSE_CHECK_USEC);

			dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
							   for (int i = 0; i < 2; ++i)
							   {
								   printf("        iteration %d\n", i);
								   double beforeUnpauseTime = VuoTimeUtilities::getCurrentTimeInSeconds();
								   runner->unpause();

								   helper->waitUntilFileExists();
								   runner->pause();
								   double afterPauseTime = VuoTimeUtilities::getCurrentTimeInSeconds();
								   usleep(PAUSE_CHECK_USEC);

								   helper->checkTimesInFile(beforeUnpauseTime, afterPauseTime);
								   helper->deleteFile();
							   }
							   runner->stop();
						   });

			runner->runOnMainThread();
			delete runner;
			delete helper;
		}

		{
			printf("    Current process, started unpaused\n"); fflush(stdout);

			WriteTimesToFileHelper *helper = new WriteTimesToFileHelper;

			VuoRunner *runner = createRunnerInCurrentProcess(compositionPath);
			__block double beforeUnpauseTime = VuoTimeUtilities::getCurrentTimeInSeconds();
			runner->start();

			dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
							   for (int i = 0; i < 2; ++i)
							   {
								   printf("        iteration %d\n", i);
								   if (i > 0)
								   {
									   beforeUnpauseTime = VuoTimeUtilities::getCurrentTimeInSeconds();
									   runner->unpause();
								   }

								   helper->waitUntilFileExists();
								   runner->pause();
								   double afterPauseTime = VuoTimeUtilities::getCurrentTimeInSeconds();
								   usleep(PAUSE_CHECK_USEC);

								   helper->checkTimesInFile(beforeUnpauseTime, afterPauseTime);
								   helper->deleteFile();
							   }
							   runner->stop();
						   });

			runner->runOnMainThread();
			delete runner;
			delete helper;
		}
	}

private:

	class TestGettingPortSummariesRunnerDelegate : public TestRunnerDelegate
	{
	private:
		struct IdentifierAndSummary
		{
			QString identifier;
			QString summary;
		};

		VuoRunner *runner;
		bool isStopping;
		int firstEventCountSeen;
		string firedPortIdentifier;
		vector<IdentifierAndSummary> actualIdentifiersAndSummaries;

	public:
		TestGettingPortSummariesRunnerDelegate()
		{
			runner = NULL;
			isStopping = false;
			firstEventCountSeen = 0;
		}

		~TestGettingPortSummariesRunnerDelegate()
		{
			delete runner;
		}

		void runComposition(VuoCompiler *compiler)
		{
			string compositionPath = TestCompositionExecution::getCompositionPath("Recur_Count_Add.vuo");
			VuoCompilerComposition *composition = NULL;
			runner = createRunnerInNewProcess(compiler, compositionPath, &composition);

			string incrementPortIdentifier;
			string countPortIdentifier;
			string item1PortIdentifier;
			string listPortIdentifier;
			string valuesPortIdentifier;
			string sumPortIdentifier;
			foreach (VuoNode *node, composition->getBase()->getNodes())
			{
				if (node->getNodeClass()->getClassName() == "vuo.test.firePeriodicallyWithCount")
				{
					{
						VuoPort *basePort = node->getOutputPortWithName("fired");
						firedPortIdentifier = static_cast<VuoCompilerPort *>(basePort->getCompiler())->getIdentifier();
					}
				}
				else if (node->getNodeClass()->getClassName() == "vuo.math.count.VuoInteger")
				{
					{
						VuoPort *basePort = node->getInputPortWithName("increment");
						incrementPortIdentifier = static_cast<VuoCompilerPort *>(basePort->getCompiler())->getIdentifier();
					}
					{
						VuoPort *basePort = node->getOutputPortWithName("count");
						countPortIdentifier = static_cast<VuoCompilerPort *>(basePort->getCompiler())->getIdentifier();
					}
				}
				else if (node->getNodeClass()->getClassName() == "vuo.list.make.2.VuoInteger")
				{
					{
						VuoPort *basePort = node->getInputPortWithName("1");
						item1PortIdentifier = static_cast<VuoCompilerPort *>(basePort->getCompiler())->getIdentifier();
					}
					{
						VuoPort *basePort = node->getOutputPortWithName("list");
						listPortIdentifier = static_cast<VuoCompilerPort *>(basePort->getCompiler())->getIdentifier();
					}
				}
				else if (node->getNodeClass()->getClassName() == "vuo.math.add.VuoInteger")
				{
					{
						VuoPort *basePort = node->getInputPortWithName("values");
						valuesPortIdentifier = static_cast<VuoCompilerPort *>(basePort->getCompiler())->getIdentifier();
					}
					{
						VuoPort *basePort = node->getOutputPortWithName("sum");
						sumPortIdentifier = static_cast<VuoCompilerPort *>(basePort->getCompiler())->getIdentifier();
					}
				}
			}

			delete composition;

			runner->setDelegate(this);
			runner->startPaused();

			{
				QCOMPARE(QString::fromStdString(runner->subscribeToOutputPortTelemetry( "", firedPortIdentifier )), QString("0"));
				QCOMPARE(QString::fromStdString(runner->subscribeToInputPortTelemetry( "", incrementPortIdentifier )), QString("1"));
				QCOMPARE(QString::fromStdString(runner->subscribeToOutputPortTelemetry( "", countPortIdentifier )), QString("0"));
				QCOMPARE(QString::fromStdString(runner->subscribeToInputPortTelemetry( "", item1PortIdentifier )), QString("0"));
				QCOMPARE(QString::fromStdString(runner->subscribeToOutputPortTelemetry( "", listPortIdentifier )), QString("List containing 2 items: <ul><li>0</li><li>10</li></ul>"));
				QCOMPARE(QString::fromStdString(runner->subscribeToInputPortTelemetry( "", valuesPortIdentifier )), QString("List containing 2 items: <ul><li>0</li><li>10</li></ul>"));
				QCOMPARE(QString::fromStdString(runner->subscribeToOutputPortTelemetry( "", sumPortIdentifier )), QString("0"));
			}

			runner->unpause();
			runner->waitUntilStopped();

			vector<IdentifierAndSummary> expectedIdentifiersAndSummaries;
			int count = 0;
			for (int eventCount = 1; expectedIdentifiersAndSummaries.size() < actualIdentifiersAndSummaries.size(); ++eventCount)
			{
				count += eventCount;
				if (eventCount >= firstEventCountSeen)
				{
					IdentifierAndSummary firedPair = { firedPortIdentifier.c_str(), QString("%1").arg(eventCount) };
					expectedIdentifiersAndSummaries.push_back(firedPair);
					IdentifierAndSummary incrementPair = { incrementPortIdentifier.c_str(), QString("%1").arg(eventCount) };
					expectedIdentifiersAndSummaries.push_back(incrementPair);
					IdentifierAndSummary countPair = { countPortIdentifier.c_str(), QString("%1").arg(count) };
					expectedIdentifiersAndSummaries.push_back(countPair);
					IdentifierAndSummary item1Pair = { item1PortIdentifier.c_str(), QString("%1").arg(count) };
					expectedIdentifiersAndSummaries.push_back(item1Pair);
					IdentifierAndSummary listPair = { listPortIdentifier.c_str(), QString("List containing 2 items: <ul><li>%1</li><li>10</li></ul>").arg(count) };
					expectedIdentifiersAndSummaries.push_back(listPair);
					IdentifierAndSummary valuesPair = { valuesPortIdentifier.c_str(), QString("List containing 2 items: <ul><li>%1</li><li>10</li></ul>").arg(count) };
					expectedIdentifiersAndSummaries.push_back(valuesPair);
					IdentifierAndSummary sumPair = { sumPortIdentifier.c_str(), QString("%1").arg(count + 10) };
					expectedIdentifiersAndSummaries.push_back(sumPair);
				}
			}

			for (int i = 0; i < expectedIdentifiersAndSummaries.size(); ++i)
			{
				QCOMPARE(actualIdentifiersAndSummaries[i].identifier + " " + actualIdentifiersAndSummaries[i].summary,
						 expectedIdentifiersAndSummaries[i].identifier + " " + expectedIdentifiersAndSummaries[i].summary);
			}
		}

		void appendIdentifierAndSummary(string portIdentifier, string dataSummary)
		{
			if (isStopping)
				return;

			// The composition may have started counting up before the runner was able to connect and start receiving telemetry.
			if (actualIdentifiersAndSummaries.empty())
			{
				if (portIdentifier != firedPortIdentifier)
					return;

				firstEventCountSeen = atoi(dataSummary.c_str());
			}

			IdentifierAndSummary identifierAndSummary = { portIdentifier.c_str(), dataSummary.c_str() };
			actualIdentifiersAndSummaries.push_back(identifierAndSummary);

			if (actualIdentifiersAndSummaries.size() == 70)
			{
				dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
				dispatch_async(queue, ^{
								   runner->stop();
							   });
				isStopping = true;
			}
		}

		void receivedTelemetryInputPortUpdated(string compositionIdentifier, string portIdentifier, bool receivedEvent, bool receivedData, string dataSummary)
		{
			appendIdentifierAndSummary(portIdentifier, dataSummary);
		}

		void receivedTelemetryOutputPortUpdated(string compositionIdentifier, string portIdentifier, bool sentEvent, bool sentData, string dataSummary)
		{
			appendIdentifierAndSummary(portIdentifier, dataSummary);
		}
	};

private slots:

	void testGettingPortSummaries()
	{
		TestGettingPortSummariesRunnerDelegate delegate;
		delegate.runComposition(compiler);
	}

private:

	class TestSettingAndGettingPortValuesRunnerDelegate : public TestRunnerDelegate
	{
	private:
		VuoRunner *runner;
		int timesCountSentEvent;
		int timesIncrementReceivedData;
		int timesIncrementReceivedEvent;
		VuoInteger firstCountSeen;
		string incrementPortIdentifier;
		string decrementPortIdentifier;
		string countPortIdentifier;
		VuoInteger incrementPortValue;
		bool isStopping;

	public:
		TestSettingAndGettingPortValuesRunnerDelegate()
		{
			runner = NULL;
			timesCountSentEvent = 0;
			timesIncrementReceivedData = 0;
			timesIncrementReceivedEvent = 0;
			isStopping = false;
		}

		~TestSettingAndGettingPortValuesRunnerDelegate()
		{
			delete runner;
		}

		void runComposition(VuoCompiler *compiler)
		{
			string compositionPath = TestCompositionExecution::getCompositionPath("Recur_Count.vuo");
			VuoCompilerComposition *composition = NULL;
			runner = createRunnerInNewProcess(compiler, compositionPath, &composition);

			foreach (VuoNode *node, composition->getBase()->getNodes())
			{
				if (node->getNodeClass()->getClassName() == "vuo.math.count.VuoInteger")
				{
					{
						VuoPort *basePort = node->getInputPortWithName("increment");
						VuoCompilerInputEventPort *compilerPort = static_cast<VuoCompilerInputEventPort *>(basePort->getCompiler());
						incrementPortIdentifier = compilerPort->getIdentifier();
					}
					{
						VuoPort *basePort = node->getInputPortWithName("decrement");
						VuoCompilerInputEventPort *compilerPort = static_cast<VuoCompilerInputEventPort *>(basePort->getCompiler());
						decrementPortIdentifier = compilerPort->getIdentifier();
					}
					{
						VuoPort *basePort = node->getOutputPortWithName("count");
						VuoCompilerOutputEventPort *compilerPort = static_cast<VuoCompilerOutputEventPort *>(basePort->getCompiler());
						countPortIdentifier = compilerPort->getIdentifier();
					}
				}
			}

			delete composition;

			incrementPortValue = 2;

			runner->setDelegate(this);

			runner->startPaused();
			runner->subscribeToInputPortTelemetry("", incrementPortIdentifier);
			runner->subscribeToOutputPortTelemetry("", countPortIdentifier);

			runner->unpause();
			runner->waitUntilStopped();

			QCOMPARE(timesIncrementReceivedEvent, 4);
			QCOMPARE(timesIncrementReceivedData, 2);
		}

		void receivedTelemetryInputPortUpdated(string compositionIdentifier, string portIdentifier, bool receivedEvent, bool receivedData, string dataSummary)
		{
			if (receivedEvent)
				++timesIncrementReceivedEvent;

			if (receivedData)
				++timesIncrementReceivedData;

			QCOMPARE((VuoInteger)atol(dataSummary.c_str()), incrementPortValue);
		}

		void receivedTelemetryOutputPortUpdated(string compositionIdentifier, string portIdentifier, bool sentEvent, bool sentData, string dataSummary)
		{
			if (isStopping)
				return;

			runner->pause();

			QCOMPARE(sentData, true);

			// For this value to be in sync with valueAsString, the composition needs to have been paused before the next event fired.
			// The firing rate of the composition is set slow enough that the composition is very likely to be paused in time.
			VuoInteger countFromRunner = VuoInteger_makeFromJson( runner->getOutputPortValue("", countPortIdentifier) );

			VuoInteger incrementFromRunner = VuoInteger_makeFromJson( runner->getInputPortValue("", incrementPortIdentifier) );
			VuoInteger countFromSummary = atol(dataSummary.c_str());

			if (timesCountSentEvent == 0)
			{
				// The composition may have started counting up before the runner was able to connect and start receiving telemetry.
				firstCountSeen = countFromSummary;
			}
			else if (timesCountSentEvent == 1)
			{
				QCOMPARE(incrementFromRunner, (VuoInteger)2);

				VuoInteger expectedCount = firstCountSeen + 1;
				QCOMPARE(countFromSummary, expectedCount);
				QCOMPARE(countFromRunner, expectedCount);

				incrementPortValue = 100;
				runner->setInputPortValue("", incrementPortIdentifier, VuoInteger_getJson(incrementPortValue));
				runner->setInputPortValue("", decrementPortIdentifier, VuoInteger_getJson(2));
			}
			else if (timesCountSentEvent == 2)
			{
				QCOMPARE(incrementFromRunner, (VuoInteger)100);

				VuoInteger expectedCount = firstCountSeen + 1 + 98;
				QCOMPARE(countFromSummary, expectedCount);
				QCOMPARE(countFromRunner, expectedCount);

				incrementPortValue = 1000;
				runner->setInputPortValue("", incrementPortIdentifier, VuoInteger_getJson(incrementPortValue));
			}
			else if (timesCountSentEvent == 3)
			{
				QCOMPARE(incrementFromRunner, (VuoInteger)1000);

				VuoInteger expectedCount = firstCountSeen + 1 + 98 + 998;
				QCOMPARE(countFromSummary, expectedCount);
				QCOMPARE(countFromRunner, expectedCount);

				dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
				dispatch_async(queue, ^{
								   runner->stop();
							   });
				isStopping = true;

				return;
			}
			++timesCountSentEvent;

			runner->unpause();
		}
	};

private slots:

	void testSettingAndGettingPortValues()
	{
		TestSettingAndGettingPortValuesRunnerDelegate delegate;
		delegate.runComposition(compiler);
	}

private:

	class TestGettingTriggerPortValuesRunnerDelegate : public TestRunnerDelegate
	{
	private:
		VuoRunner *runner;
		int timesCountChanged;
		VuoInteger firstCountSeen;
		string firedPortIdentifier;
		bool isStopping;

	public:
		TestGettingTriggerPortValuesRunnerDelegate()
		{
			runner = NULL;
			timesCountChanged = 0;
			isStopping = false;
		}

		~TestGettingTriggerPortValuesRunnerDelegate()
		{
			delete runner;
		}

		void runComposition(VuoCompiler *compiler)
		{
			string compositionPath = TestCompositionExecution::getCompositionPath("FirePeriodicallyWithCount.vuo");
			VuoCompilerComposition *composition = NULL;
			runner = createRunnerInNewProcess(compiler, compositionPath, &composition);

			foreach (VuoNode *node, composition->getBase()->getNodes())
			{
				if (node->getNodeClass()->getClassName() == "vuo.test.firePeriodicallyWithCount")
				{
					VuoPort *basePort = node->getOutputPortWithName("fired");
					firedPortIdentifier = static_cast<VuoCompilerPort *>(basePort->getCompiler())->getIdentifier();
				}
			}
			delete composition;

			runner->setDelegate(this);

			runner->startPaused();
			runner->subscribeToAllTelemetry("");
			runner->unpause();
			runner->waitUntilStopped();
		}

		void receivedTelemetryOutputPortUpdated(string compositionIdentifier, string portIdentifier, bool sentEvent, bool sentData, string dataSummary)
		{
			if (isStopping)
				return;

			runner->pause();

			QCOMPARE(QString(portIdentifier.c_str()), QString(firedPortIdentifier.c_str()));

			// For this value to be in sync with valueAsString, the composition needs to have been paused before the next event fired.
			// The firing rate of the composition is set slow enough that the composition is very likely to be paused in time.
			VuoInteger countFromRunner = VuoInteger_makeFromJson( runner->getOutputPortValue("", firedPortIdentifier.c_str()) );

			VuoInteger countFromSummary = atol(dataSummary.c_str());

			if (timesCountChanged == 0)
			{
				// The composition may have started counting up before the runner was able to connect and start receiving telemetry.
				firstCountSeen = countFromSummary;
			}
			else if (timesCountChanged == 1)
			{
				VuoInteger expectedCount = firstCountSeen + 1;
				QCOMPARE(countFromSummary, expectedCount);
				QCOMPARE(countFromRunner, expectedCount);

				dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
				dispatch_async(queue, ^{
								   runner->stop();
							   });
				isStopping = true;

				return;
			}
			++timesCountChanged;

			runner->unpause();
		}
	};

private slots:

	void testGettingTriggerPortValues()
	{
		TestGettingTriggerPortValuesRunnerDelegate delegate;
		delegate.runComposition(compiler);
	}

private:

	class TestFiringTriggerPortEventsRunnerDelegate : public TestRunnerDelegate
	{
	private:
		VuoRunner *runner;
		int timesSumChanged;
		string startedPortIdentifier;
		string spunOffPortIdentifier;
		string sumPortIdentifier;

	public:
		TestFiringTriggerPortEventsRunnerDelegate()
		{
			runner = NULL;
			timesSumChanged = 0;
		}

		~TestFiringTriggerPortEventsRunnerDelegate()
		{
			delete runner;
		}

		void runComposition(VuoCompiler *compiler)
		{
			string compositionPath = TestCompositionExecution::getCompositionPath("SpinOffWithCount.vuo");
			VuoCompilerComposition *composition = NULL;
			runner = createRunnerInNewProcess(compiler, compositionPath, &composition);

			foreach (VuoNode *node, composition->getBase()->getNodes())
			{
				if (node->getNodeClass()->getClassName() == "vuo.event.fireOnStart")
				{
					VuoPort *basePort = node->getOutputPortWithName("started");
					startedPortIdentifier = static_cast<VuoCompilerPort *>(basePort->getCompiler())->getIdentifier();
				}
				else if (node->getNodeClass()->getClassName() == "vuo.test.spinOffWithCount")
				{
					VuoPort *basePort = node->getOutputPortWithName("spunOff");
					spunOffPortIdentifier = static_cast<VuoCompilerPort *>(basePort->getCompiler())->getIdentifier();
				}
				else if (node->getNodeClass()->getClassName() == "vuo.math.add.VuoInteger")
				{
					VuoPort *basePort = node->getOutputPortWithName("sum");
					sumPortIdentifier = static_cast<VuoCompilerPort *>(basePort->getCompiler())->getIdentifier();
				}
			}
			delete composition;

			runner->setDelegate(this);

			runner->startPaused();
			runner->subscribeToOutputPortTelemetry("", sumPortIdentifier);
			runner->unpause();
			runner->waitUntilStopped();
		}

		void receivedTelemetryOutputPortUpdated(string compositionIdentifier, string portIdentifier, bool sentEvent, bool sentData, string dataSummary)
		{
			if (timesSumChanged == 0)
			{
				QCOMPARE(QString(dataSummary.c_str()), QString("1"));
				runner->fireTriggerPortEvent("", startedPortIdentifier);
			}
			else if (timesSumChanged == 1)
			{
				QCOMPARE(QString(dataSummary.c_str()), QString("2"));
				runner->fireTriggerPortEvent("", startedPortIdentifier);
			}
			else if (timesSumChanged == 2)
			{
				QCOMPARE(QString(dataSummary.c_str()), QString("3"));
				runner->fireTriggerPortEvent("", spunOffPortIdentifier);
			}
			else if (timesSumChanged == 3)
			{
				QCOMPARE(QString(dataSummary.c_str()), QString("3"));
				dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
				dispatch_async(queue, ^{
								   runner->stop();
							   });
			}
			++timesSumChanged;
		}
	};

private slots:

	void testFiringTriggerPortEvents()
	{
		TestFiringTriggerPortEventsRunnerDelegate delegate;
		delegate.runComposition(compiler);
	}

private:

	void compareJsonObjects(json_object *actual, json_object *expected)
	{
		{
			json_object_object_foreach(expected, key, val)
			{
				json_object *o;
				bool foundKey = json_object_object_get_ex(actual, key, &o);
				QVERIFY2(foundKey, key);
				QCOMPARE(QString(json_object_to_json_string(o)), QString(json_object_to_json_string(val)));
			}
		}
		{
			json_object_object_foreach(actual, key, val)
			{
				json_object *o;
				bool foundKey = json_object_object_get_ex(expected, key, &o);
				QVERIFY2(foundKey, key);
				QCOMPARE(QString(json_object_to_json_string(val)), QString(json_object_to_json_string(o)));
			}
		}
	}

private slots:

	void testGettingPublishedPorts_data()
	{
		QTest::addColumn< QString >("compositionFile");
		QTest::addColumn< vector<VuoRunner::Port *> >("expectedInputs");
		QTest::addColumn< vector<VuoRunner::Port *> >("expectedOutputs");

		{
			vector<VuoRunner::Port *> inputs;
			inputs.push_back( new VuoRunner::Port("publishedIn0", "VuoInteger", json_tokener_parse("{}")) );
			inputs.push_back( new VuoRunner::Port("publishedIn1", "VuoInteger", json_tokener_parse("{}")) );
			inputs.push_back( new VuoRunner::Port("publishedIn2", "VuoReal", json_tokener_parse("{\"default\":0.050000,\"suggestedMin\":0.000001,\"suggestedMax\":0.05}")) );
			vector<VuoRunner::Port *> outputs;
			outputs.push_back( new VuoRunner::Port("publishedSum", "VuoInteger", json_tokener_parse("{}")) );
			QTest::newRow("some published input and output ports") << "Recur_Add_published.vuo" << inputs << outputs;
		}

		{
			vector<VuoRunner::Port *> inputs;
			vector<VuoRunner::Port *> outputs;
			QTest::newRow("no published ports") << "Recur_Count.vuo" << inputs << outputs;
		}
	}
	void testGettingPublishedPorts()
	{
		QFETCH(QString, compositionFile);
		QFETCH(vector<VuoRunner::Port *>, expectedInputs);
		QFETCH(vector<VuoRunner::Port *>, expectedOutputs);

		string compositionPath = getCompositionPath(compositionFile.toStdString());
		VuoRunner *runner = createRunnerInNewProcess(compositionPath);
		runner->setDelegate(NULL);  /// @todo https://b33p.net/kosada/node/6021

		runner->startPaused();
		vector<VuoRunner::Port *> actualInputs = runner->getPublishedInputPorts();
		vector<VuoRunner::Port *> actualOutputs = runner->getPublishedOutputPorts();
		runner->stop();

		QCOMPARE(actualInputs.size(), expectedInputs.size());
		for (int i = 0; i < expectedInputs.size(); ++i)
		{
			QCOMPARE(actualInputs.at(i)->getName(), expectedInputs.at(i)->getName());
			QCOMPARE(actualInputs.at(i)->getType(), expectedInputs.at(i)->getType());
			compareJsonObjects(actualInputs.at(i)->getDetails(), expectedInputs.at(i)->getDetails());
		}

		QCOMPARE(actualOutputs.size(), expectedOutputs.size());
		for (int i = 0; i < expectedOutputs.size(); ++i)
		{
			QCOMPARE(actualOutputs.at(i)->getName(), expectedOutputs.at(i)->getName());
			QCOMPARE(actualOutputs.at(i)->getType(), expectedOutputs.at(i)->getType());
			compareJsonObjects(actualOutputs.at(i)->getDetails(), expectedOutputs.at(i)->getDetails());
		}

		delete runner;
	}

private:

	class TestPublishedPortCachingRunnerDelegate : public TestRunnerDelegate
	{
	private:
		VuoRunner *runner;
		bool isStopping;

	public:
		TestPublishedPortCachingRunnerDelegate(VuoRunner *runner)
		{
			this->runner = runner;
			isStopping = false;
		}

		void receivedTelemetryPublishedOutputPortUpdated(VuoRunner::Port *port, bool sentData, string dataSummary)
		{
			// This tests that:
			//   - VuoRunner sets up published ports by the time VuoRunnerDelegate methods begin being called. - https://b33p.net/kosada/node/3880
			//   - The pointers retrieved by getPublishedOutputPorts are consistent.

			if (isStopping)
				return;

			vector<VuoRunner::Port *> outputPorts1 = runner->getPublishedOutputPorts();
			QCOMPARE(outputPorts1.size(), (size_t)1);
			QCOMPARE(outputPorts1.at(0), port);

			vector<VuoRunner::Port *> outputPorts2 = runner->getPublishedOutputPorts();
			QCOMPARE(outputPorts2.size(), (size_t)1);
			QCOMPARE(outputPorts2.at(0), port);

			dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
			dispatch_async(queue, ^{
							   runner->stop();
						   });
			isStopping = true;
		}
	};

private slots:

	void testPublishedPortCaching_data()
	{
		QTest::addColumn< bool >("shouldRunInSeparateProcess");

		QTest::newRow("separate process") << true;
		QTest::newRow("current process") << false;
	}
	void testPublishedPortCaching()
	{
		QFETCH(bool, shouldRunInSeparateProcess);

		string compositionPath = getCompositionPath("Recur_Add_published.vuo");

		VuoRunner *runner = (shouldRunInSeparateProcess ?
								 createRunnerInNewProcess(compositionPath) :
								 createRunnerInCurrentProcess(compositionPath));

		TestPublishedPortCachingRunnerDelegate delegate(runner);
		runner->setDelegate(&delegate);
		runner->start();

		if (shouldRunInSeparateProcess)
		{
			runner->waitUntilStopped();
		}
		else
		{
			dispatch_group_t group = dispatch_group_create();
			dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
			dispatch_group_async(group, queue, ^{
									 runner->waitUntilStopped();
								 });
			runner->runOnMainThread();
			dispatch_group_wait(group, DISPATCH_TIME_FOREVER);
		}

		delete runner;
	}

private:

	class TestSettingAndGettingPublishedPortValuesRunnerDelegate : public TestRunnerDelegate
	{
	private:
		VuoRunner *runner;
		VuoRunner::Port *publishedIn1;
		VuoRunner::Port *publishedSum;
		int timesSumChanged;
		bool isStopping;

	public:
		TestSettingAndGettingPublishedPortValuesRunnerDelegate()
		{
			runner = NULL;
			publishedIn1 = NULL;
			publishedSum = NULL;
			timesSumChanged = 0;
			isStopping = false;
		}

		~TestSettingAndGettingPublishedPortValuesRunnerDelegate()
		{
			delete runner;
		}

		void runComposition(VuoCompiler *compiler)
		{
			string compositionPath = getCompositionPath("Recur_Add_published.vuo");
			runner = createRunnerInNewProcess(compiler, compositionPath);
			runner->setDelegate(this);

			runner->startPaused();

			VuoRunner::Port *publishedIn0 = runner->getPublishedInputPortWithName("publishedIn0");
			publishedIn1 = runner->getPublishedInputPortWithName("publishedIn1");
			publishedSum = runner->getPublishedOutputPortWithName("publishedSum");
			QVERIFY(publishedIn0 != NULL);
			QVERIFY(publishedIn1 != NULL);
			QVERIFY(publishedSum != NULL);

			QCOMPARE((VuoInteger)0, VuoInteger_makeFromJson(runner->getPublishedInputPortValue(publishedIn0)));
			map<VuoRunner::Port *, json_object *> m;
			m[publishedIn0] = VuoInteger_getJson(100);
			runner->setPublishedInputPortValues(m);
			QCOMPARE((VuoInteger)100, VuoInteger_makeFromJson(runner->getPublishedInputPortValue(publishedIn0)));

			runner->unpause();
			runner->waitUntilStopped();
		}

		void receivedTelemetryPublishedOutputPortUpdated(VuoRunner::Port *port, bool sentData, string dataSummary)
		{
			if (isStopping)
				return;

			runner->pause();

			QCOMPARE(QString(port->getName().c_str()), QString("publishedSum"));

			VuoInteger sumFromSummary = atol(dataSummary.c_str());

			++timesSumChanged;
			if (timesSumChanged == 1)
			{
				QCOMPARE(sumFromSummary, (VuoInteger)201);
				VuoInteger sumFromRunner = VuoInteger_makeFromJson( runner->getPublishedOutputPortValue(publishedSum) );
				QCOMPARE(sumFromRunner, (VuoInteger)201);

				map<VuoRunner::Port *, json_object *> m;
				m[publishedIn1] = VuoInteger_getJson(1000);
				runner->setPublishedInputPortValues(m);
			}
			else
			{
				if (sumFromSummary == 201)
				{
					// Ignore the few additional events that may have fired before the composition was paused the first time.
					QVERIFY(timesSumChanged < 10);
				}
				else
				{
					QCOMPARE(sumFromSummary, (VuoInteger)1201);
					VuoInteger sumFromRunner = VuoInteger_makeFromJson( runner->getPublishedOutputPortValue(publishedSum) );
					QCOMPARE(sumFromRunner, (VuoInteger)1201);

					dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
					dispatch_async(queue, ^{
									   runner->stop();
								   });
					isStopping = true;

					return;
				}
			}

			runner->unpause();
		}
	};

private slots:

	void testSettingAndGettingPublishedPortValues()
	{
		TestSettingAndGettingPublishedPortValuesRunnerDelegate delegate;
		delegate.runComposition(compiler);
	}

private:

	class TestFiringPublishedInputPortEventsRunnerDelegate : public TestRunnerDelegate
	{
	private:
		VuoRunner *runner;
		VuoRunner::Port *publishedIncrementOne;
		VuoRunner::Port *publishedDecrementBoth;
		int timesSumChanged;

	public:
		TestFiringPublishedInputPortEventsRunnerDelegate()
		{
			runner = NULL;
			publishedIncrementOne = NULL;
			publishedDecrementBoth = NULL;
			timesSumChanged = 0;
		}

		~TestFiringPublishedInputPortEventsRunnerDelegate()
		{
			delete runner;
		}

		void runComposition(VuoCompiler *compiler)
		{
			string compositionPath = getCompositionPath("PublishedInputsAndNoTrigger.vuo");
			runner = createRunnerInNewProcess(compiler, compositionPath);
			runner->setDelegate(this);

			runner->start();

			publishedIncrementOne = runner->getPublishedInputPortWithName("publishedIncrementOne");
			publishedDecrementBoth = runner->getPublishedInputPortWithName("publishedDecrementBoth");
			QVERIFY(publishedIncrementOne != NULL);
			QVERIFY(publishedDecrementBoth != NULL);

			runner->firePublishedInputPortEvent(publishedIncrementOne);

			runner->waitUntilStopped();
		}

		void receivedTelemetryPublishedOutputPortUpdated(VuoRunner::Port *port, bool sentData, string dataSummary)
		{
			QCOMPARE(QString(port->getName().c_str()), QString("publishedSum"));

			++timesSumChanged;
			if (timesSumChanged == 1)
			{
				QCOMPARE(QString(dataSummary.c_str()), QString("42"));  // 42 + 0

				map<VuoRunner::Port *, json_object *> m;
				m[publishedDecrementBoth] = VuoInteger_getJson(3);
				runner->setPublishedInputPortValues(m);

				runner->firePublishedInputPortEvent(publishedDecrementBoth);
			}
			else if (timesSumChanged == 2)
			{
				QCOMPARE(QString(dataSummary.c_str()), QString("36"));  // 39 + -3

				map<VuoRunner::Port *, json_object *> m;
				m[publishedIncrementOne] = VuoInteger_getJson(1);
				m[publishedDecrementBoth] = VuoInteger_getJson(5);
				runner->setPublishedInputPortValues(m);

				set<VuoRunner::Port *> firePorts;
				firePorts.insert(publishedIncrementOne);
				firePorts.insert(publishedDecrementBoth);

				runner->firePublishedInputPortEvent(firePorts);
			}
			else if (timesSumChanged == 3)
			{
				QCOMPARE(QString(dataSummary.c_str()), QString("27"));  // 35 + -8

				dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
				dispatch_async(queue, ^{
								   runner->stop();
							   });
			}
		}
	};

private slots:

	void testFiringPublishedInputPortEvents()
	{
		TestFiringPublishedInputPortEventsRunnerDelegate delegate;
		delegate.runComposition(compiler);
	}

	void testWaitingForFiredPublishedPortEvent_timingOfWaitCall()
	{
		string compositionPath = getCompositionPath("MultiplePublishedOutputs.vuo");

		// Do twice: Generate an event and start to wait before it reaches any published output.
		{
			printf("    Trigger, wait, propagate\n"); fflush(stdout);

			VuoRunner *runner = createRunnerInNewProcess(compositionPath);
			runner->start();
			runner->subscribeToEventTelemetry("");

			VuoRunner::Port *shouldDelay = runner->getPublishedInputPortWithName("shouldDelay");
			VuoRunner::Port *publishedCount1 = runner->getPublishedOutputPortWithName("publishedCount1");
			VuoRunner::Port *publishedCount2 = runner->getPublishedOutputPortWithName("publishedCount2");
			VuoRunner::Port *publishedCount3 = runner->getPublishedOutputPortWithName("publishedCount3");

			vector<VuoInteger> expectedCounts;
			expectedCounts.push_back(10);
			expectedCounts.push_back(20);
			for (vector<VuoInteger>::iterator i = expectedCounts.begin(); i != expectedCounts.end(); ++i)
			{
				map<VuoRunner::Port *, json_object *> m;
				m[shouldDelay] = VuoBoolean_getJson(true);
				runner->setPublishedInputPortValues(m);

				runner->firePublishedInputPortEvent(shouldDelay);
				runner->waitForFiredPublishedInputPortEvent();
				QCOMPARE(VuoInteger_makeFromJson( runner->getPublishedOutputPortValue(publishedCount1) ), *i);
				QCOMPARE(VuoInteger_makeFromJson( runner->getPublishedOutputPortValue(publishedCount2) ), (VuoInteger)0);
				QCOMPARE(VuoInteger_makeFromJson( runner->getPublishedOutputPortValue(publishedCount3) ), (VuoInteger)0);
			}

			runner->stop();
			delete runner;
		}

		// Do twice: Generate an event and let it reach a published output port before starting to wait.
		{
			printf("    Trigger, propagate, wait\n"); fflush(stdout);

			VuoRunner *runner = createRunnerInNewProcess(compositionPath);
			runner->start();
			runner->subscribeToEventTelemetry("");

			VuoRunner::Port *shouldDelay = runner->getPublishedInputPortWithName("shouldDelay");
			VuoRunner::Port *publishedCount1 = runner->getPublishedOutputPortWithName("publishedCount1");
			VuoRunner::Port *publishedCount2 = runner->getPublishedOutputPortWithName("publishedCount2");
			VuoRunner::Port *publishedCount3 = runner->getPublishedOutputPortWithName("publishedCount3");

			vector<VuoInteger> expectedCounts;
			expectedCounts.push_back(100);
			expectedCounts.push_back(200);
			for (vector<VuoInteger>::iterator i = expectedCounts.begin(); i != expectedCounts.end(); ++i)
			{
				map<VuoRunner::Port *, json_object *> m;
				m[shouldDelay] = VuoBoolean_getJson(false);
				runner->setPublishedInputPortValues(m);

				runner->firePublishedInputPortEvent(shouldDelay);
				sleep(1);
				runner->waitForFiredPublishedInputPortEvent();
				QCOMPARE(VuoInteger_makeFromJson( runner->getPublishedOutputPortValue(publishedCount1) ), (VuoInteger)0);
				QCOMPARE(VuoInteger_makeFromJson( runner->getPublishedOutputPortValue(publishedCount2) ), *i);
				QCOMPARE(VuoInteger_makeFromJson( runner->getPublishedOutputPortValue(publishedCount3) ), *i);
			}

			runner->stop();
			delete runner;
		}

		// Do one of each. (Generate, wait, receive published output event. Generate, receive published output event, wait.)
		{
			printf("    Do one of each\n"); fflush(stdout);

			VuoRunner *runner = createRunnerInNewProcess(compositionPath);
			runner->start();
			runner->subscribeToEventTelemetry("");

			VuoRunner::Port *shouldDelay = runner->getPublishedInputPortWithName("shouldDelay");
			VuoRunner::Port *publishedCount1 = runner->getPublishedOutputPortWithName("publishedCount1");
			VuoRunner::Port *publishedCount2 = runner->getPublishedOutputPortWithName("publishedCount2");
			VuoRunner::Port *publishedCount3 = runner->getPublishedOutputPortWithName("publishedCount3");

			map<VuoRunner::Port *, json_object *> m;
			m[shouldDelay] = VuoBoolean_getJson(true);
			runner->setPublishedInputPortValues(m);

			runner->firePublishedInputPortEvent(shouldDelay);
			runner->waitForFiredPublishedInputPortEvent();
			QCOMPARE(VuoInteger_makeFromJson( runner->getPublishedOutputPortValue(publishedCount1) ), (VuoInteger)10);
			QCOMPARE(VuoInteger_makeFromJson( runner->getPublishedOutputPortValue(publishedCount2) ), (VuoInteger)0);
			QCOMPARE(VuoInteger_makeFromJson( runner->getPublishedOutputPortValue(publishedCount3) ), (VuoInteger)0);

			m[shouldDelay] = VuoBoolean_getJson(false);
			runner->setPublishedInputPortValues(m);

			runner->firePublishedInputPortEvent(shouldDelay);
			sleep(1);
			runner->waitForFiredPublishedInputPortEvent();
			QCOMPARE(VuoInteger_makeFromJson( runner->getPublishedOutputPortValue(publishedCount1) ), (VuoInteger)10);
			QCOMPARE(VuoInteger_makeFromJson( runner->getPublishedOutputPortValue(publishedCount2) ), (VuoInteger)100);
			QCOMPARE(VuoInteger_makeFromJson( runner->getPublishedOutputPortValue(publishedCount3) ), (VuoInteger)100);

			runner->stop();
			delete runner;
		}

		// Generate more events than are waited on.
		{
			printf("    Trigger more than waited on\n"); fflush(stdout);

			VuoRunner *runner = createRunnerInNewProcess(compositionPath);
			runner->start();
			runner->subscribeToEventTelemetry("");

			VuoRunner::Port *shouldDelay = runner->getPublishedInputPortWithName("shouldDelay");

			map<VuoRunner::Port *, json_object *> m;
			m[shouldDelay] = VuoBoolean_getJson(false);
			runner->setPublishedInputPortValues(m);

			runner->firePublishedInputPortEvent(shouldDelay);
			runner->firePublishedInputPortEvent(shouldDelay);
			runner->waitForFiredPublishedInputPortEvent();

			runner->stop();
			delete runner;
		}
	}

	void testWaitingForFiredPublishedPortEvent_eventLifetime_data()
	{
		QTest::addColumn<QString>("composition");
		QTest::addColumn<bool>("pause");
		QTest::addColumn<bool>("allowLostContact");
		QTest::addColumn<int>("expectedOutput");

		QTest::newRow("published input port has no outgoing cables") << "UnconnectedPublishedPorts.vuo" << false << false << 0;
		QTest::newRow("published input event blocked") << "PublishedBlocked.vuo" << false << false << 0;
		QTest::newRow("internal trigger fired") << "PublishedBlockedInternalFired.vuo" << false << false << 0;
		QTest::newRow("published input event spun off by Spin Off Value") << "PublishedToSpinOffValue.vuo" << false << false << 1;
		QTest::newRow("published input event spun off by Spin Off Event") << "PublishedToSpinOffEvent.vuo" << false << false << 2;
		QTest::newRow("published input event spun off by Spin Off Events") << "PublishedToSpinOffEvents.vuo" << false << false << 10;
		QTest::newRow("published input event spun off by Build List") << "PublishedToBuildList.vuo" << false << false << 101;
		QTest::newRow("published input event spun off by Process List") << "PublishedToProcessList.vuo" << false << false << 12;
		QTest::newRow("published input event spun off to multiple sequentially") << "PublishedToSpinOffSeries.vuo" << false << false << 99;
		QTest::newRow("published input event spun off to multiple concurrently") << "PublishedToSpinOffScatter.vuo" << false << false << 4;
		QTest::newRow("spun off event dropped") << "PublishedToSpinOffDelayDrop.vuo" << false << false << 1;
		QTest::newRow("spun off event skipped because composition paused") << "PublishedToDelaySpinOff.vuo" << true << false << 1;
		QTest::newRow("published input event caused composition to stop itself") << "PublishedToStop.vuo" << false << true << 0;
		QTest::newRow("published input event caused composition to crash") << "PublishedToExit.vuo" << false << true << 0;
	}
	void testWaitingForFiredPublishedPortEvent_eventLifetime()
	{
		QFETCH(QString, composition);
		QFETCH(bool, pause);
		QFETCH(bool, allowLostContact);
		QFETCH(int, expectedOutput);

		string compositionPath = getCompositionPath(composition.toStdString());
		VuoRunner *runner = createRunnerInNewProcess(compositionPath);
		if (allowLostContact)
			runner->setDelegate(nullptr);  // Don't use TestRunnerDelegate, which fails if the runner loses contact with the composition.
		runner->start();

		VuoRunner::Port *inPort = runner->getPublishedInputPortWithName("in");
		VuoRunner::Port *outPort = runner->getPublishedOutputPortWithName("out");
		QVERIFY(inPort);
		QVERIFY(outPort);

		runner->firePublishedInputPortEvent(inPort);
		if (pause)
			runner->pause();
		runner->waitForFiredPublishedInputPortEvent();

		json_object *actualOutput = runner->getPublishedOutputPortValue(outPort);
		QCOMPARE(VuoInteger_makeFromJson(actualOutput), (VuoInteger)expectedOutput);
		json_object_put(actualOutput);

		runner->stop();
		delete runner;
	}

	void testUnconnectedPublishedPorts()
	{
		string compositionPath = getCompositionPath("UnconnectedPublishedPorts.vuo");
		VuoRunner *runner = createRunnerInNewProcess(compositionPath);
		runner->start();

		vector<VuoRunner::Port *> inPorts = runner->getPublishedInputPorts();
		QCOMPARE(inPorts.size(), (size_t)1);
		VuoRunner::Port *inPort = inPorts[0];

		vector<VuoRunner::Port *> outPorts = runner->getPublishedOutputPorts();
		QCOMPARE(outPorts.size(), (size_t)1);
		VuoRunner::Port *outPort = outPorts[0];

		map<VuoRunner::Port *, json_object *> m;
		m[inPort] = VuoInteger_getJson(49);
		runner->setPublishedInputPortValues(m);

		runner->firePublishedInputPortEvent(inPort);

		json_object *outPortValue = runner->getPublishedOutputPortValue(outPort);
		QCOMPARE(VuoInteger_makeFromJson(outPortValue), (VuoInteger)0);

		runner->stop();
		delete runner;
	}

	void testEventOnlyPublishedInputCables()
	{
		string compositionPath = getCompositionPath("EventOnlyPublishedInputCable.vuo");
		VuoRunner *runner = createRunnerInNewProcess(compositionPath);
		runner->start();

		vector<VuoRunner::Port *> inPorts = runner->getPublishedInputPorts();
		QCOMPARE(inPorts.size(), (size_t)1);

		vector<VuoRunner::Port *> outPorts = runner->getPublishedOutputPorts();
		QCOMPARE(outPorts.size(), (size_t)2);

		map<VuoRunner::Port *, json_object *> m;
		m[inPorts[0]] = VuoInteger_getJson(9);
		runner->setPublishedInputPortValues(m);

		runner->firePublishedInputPortEvent(inPorts[0]);
		runner->waitForFiredPublishedInputPortEvent();

		json_object *outPort1Value = runner->getPublishedOutputPortValue(outPorts[0]);
		QCOMPARE(VuoInteger_makeFromJson(outPort1Value), (VuoInteger)9);

		json_object *outPort2Value = runner->getPublishedOutputPortValue(outPorts[1]);
		QCOMPARE(VuoInteger_makeFromJson(outPort2Value), (VuoInteger)1);

		runner->stop();
		delete runner;
	}

private:

	class TestMultiplyConnectedPublishedOutputPortsRunnerDelegate : public TestRunnerDelegate
	{
	private:
		VuoRunner *runner;
		int timesCountUpdated;

	public:
		TestMultiplyConnectedPublishedOutputPortsRunnerDelegate()
		{
			runner = NULL;
			timesCountUpdated = 0;
		}

		~TestMultiplyConnectedPublishedOutputPortsRunnerDelegate()
		{
			delete runner;
		}

		void runComposition(VuoCompiler *compiler)
		{
			string compositionPath = getCompositionPath("MultiplyConnectedPublishedOutput.vuo");
			runner = createRunnerInNewProcess(compiler, compositionPath);
			runner->setDelegate(this);

			runner->start();
			VuoRunner::Port *inPort = runner->getPublishedInputPortWithName("start");
			runner->firePublishedInputPortEvent(inPort);
			runner->firePublishedInputPortEvent(inPort);
			runner->waitUntilStopped();
		}

		void receivedTelemetryPublishedOutputPortUpdated(VuoRunner::Port *port, bool sentData, string dataSummary)
		{
			++timesCountUpdated;
			if (timesCountUpdated == 1)
			{
				QVERIFY(sentData);
				QCOMPARE(QString::fromStdString(dataSummary), QString(VuoInteger_getSummary(1)));
			}
			else if (timesCountUpdated == 2)
			{
				QVERIFY(sentData);
				QCOMPARE(QString::fromStdString(dataSummary), QString(VuoInteger_getSummary(2)));

				dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
								   runner->stop();
							   });
			}
		}
	};

private slots:

	void testMultiplyConnectedPublishedOutputPorts()
	{
		TestMultiplyConnectedPublishedOutputPortsRunnerDelegate delegate;
		delegate.runComposition(compiler);
	}

private:

	class TestNoTelemetryForInternalUsePortsRunnerDelegate : public TestRunnerDelegate
	{
	private:
		VuoRunner *runner;
		vector<string> publishedOutputPortsSeen;

	public:
		TestNoTelemetryForInternalUsePortsRunnerDelegate()
		{
			runner = NULL;
		}

		~TestNoTelemetryForInternalUsePortsRunnerDelegate()
		{
			delete runner;
		}

		void runComposition(VuoCompiler *compiler)
		{
			string compositionPath = getCompositionPath("UnconnectedTriggerAndPublishedPorts.vuo");

			bool isTriggerConnectedToGatherPort = false;
			string compositionString = VuoFileUtilities::readFileToString(compositionPath);
			VuoCompilerComposition *composition = VuoCompilerComposition::newCompositionFromGraphvizDeclaration(compositionString, compiler);
			VuoCompilerGraph graph(composition, compiler);
			for (VuoCompilerTriggerPort *trigger : graph.getTriggerPorts())
				if (trigger->getBase()->getClass()->getName() == "started")
					for (VuoCompilerCable *outCable : graph.getOutgoingCables(trigger))
						if (outCable->getBase()->getToPort() == graph.getGatherPortOnPublishedOutputNode())
							isTriggerConnectedToGatherPort = true;
			QVERIFY(isTriggerConnectedToGatherPort);  // Make sure the composition actually makes use of an internal-use-only published port.

			runner = createRunnerInNewProcess(compiler, compositionPath);
			runner->setDelegate(this);

			runner->start();
			VuoRunner::Port *inPort = runner->getPublishedInputPortWithName("start");
			runner->firePublishedInputPortEvent(inPort);
			sleep(1);
			runner->firePublishedInputPortEvent(inPort);
			runner->waitUntilStopped();

			for (auto name : publishedOutputPortsSeen)
				QCOMPARE(QString::fromStdString(name), QString("testOutput"));
		}

		void receivedTelemetryPublishedOutputPortUpdated(VuoRunner::Port *port, bool sentData, string dataSummary)
		{
			publishedOutputPortsSeen.push_back(port->getName());

			if (publishedOutputPortsSeen.size() > 1)
			{
				dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
								   runner->stop();
							   });
			}
		}
	};

private slots:

	void testNoTelemetryForInternalUsePorts()
	{
		TestNoTelemetryForInternalUsePortsRunnerDelegate delegate;
		delegate.runComposition(compiler);
	}

	void testEventlessTransmission()
	{
		string compositionPath = TestCompositionExecution::getCompositionPath("CutList.vuo");
		VuoCompilerComposition *composition = NULL;
		VuoRunner *runner = createRunnerInNewProcess(compiler, compositionPath, &composition);

		string item1PortIdentifier;
		string listPortIdentifier;
		string partialListPortIdentifier;

		foreach (VuoNode *node, composition->getBase()->getNodes())
		{
			if (node->getNodeClass()->getClassName() == "vuo.list.make.3.VuoInteger")
			{
				VuoPort *basePort = node->getInputPortWithName("1");
				item1PortIdentifier = static_cast<VuoCompilerPort *>( basePort->getCompiler() )->getIdentifier();
			}
			else if (node->getNodeClass()->getClassName() == "vuo.list.cut.VuoInteger")
			{
				{
					VuoPort *basePort = node->getInputPortWithName("list");
					listPortIdentifier = static_cast<VuoCompilerPort *>( basePort->getCompiler() )->getIdentifier();
				}
				{
					VuoPort *basePort = node->getOutputPortWithName("partialList");
					partialListPortIdentifier = static_cast<VuoCompilerPort *>( basePort->getCompiler() )->getIdentifier();
				}
			}
		}

		QVERIFY(! item1PortIdentifier.empty());
		QVERIFY(! listPortIdentifier.empty());
		QVERIFY(! partialListPortIdentifier.empty());

		runner->start();
		VuoRunner::Port *inPort = runner->getPublishedInputPortWithName("Refresh");
		runner->firePublishedInputPortEvent(inPort);
		runner->waitForFiredPublishedInputPortEvent();

		{
			json_object *item1Value = runner->getInputPortValue("", item1PortIdentifier);
			QCOMPARE(QString(json_object_to_json_string_ext(item1Value, JSON_C_TO_STRING_PLAIN)), QString("11"));

			json_object *listValue = runner->getInputPortValue("", listPortIdentifier);
			QCOMPARE(QString(json_object_to_json_string_ext(listValue, JSON_C_TO_STRING_PLAIN)), QString("[11,22,33]"));

			json_object *partialListValue = runner->getInputPortValue("", partialListPortIdentifier);
			QCOMPARE(QString(json_object_to_json_string_ext(partialListValue, JSON_C_TO_STRING_PLAIN)), QString("[11,22]"));
		}

		json_object *newValue = VuoInteger_getJson(44);
		runner->setInputPortValue("", item1PortIdentifier, newValue);
		json_object_put(newValue);

		{
			json_object *item1Value = runner->getInputPortValue("", item1PortIdentifier);
			QCOMPARE(QString(json_object_to_json_string_ext(item1Value, JSON_C_TO_STRING_PLAIN)), QString("44"));

			json_object *listValue = runner->getInputPortValue("", listPortIdentifier);
			QCOMPARE(QString(json_object_to_json_string_ext(listValue, JSON_C_TO_STRING_PLAIN)), QString("[44,22,33]"));

			json_object *partialListValue = runner->getInputPortValue("", partialListPortIdentifier);
			QCOMPARE(QString(json_object_to_json_string_ext(partialListValue, JSON_C_TO_STRING_PLAIN)), QString("[11,22]"));
		}

		runner->firePublishedInputPortEvent(inPort);
		runner->waitForFiredPublishedInputPortEvent();

		{
			json_object *item1Value = runner->getInputPortValue("", item1PortIdentifier);
			QCOMPARE(QString(json_object_to_json_string_ext(item1Value, JSON_C_TO_STRING_PLAIN)), QString("44"));

			json_object *listValue = runner->getInputPortValue("", listPortIdentifier);
			QCOMPARE(QString(json_object_to_json_string_ext(listValue, JSON_C_TO_STRING_PLAIN)), QString("[44,22,33]"));

			json_object *partialListValue = runner->getInputPortValue("", partialListPortIdentifier);
			QCOMPARE(QString(json_object_to_json_string_ext(partialListValue, JSON_C_TO_STRING_PLAIN)), QString("[44,22]"));
		}

		runner->stop();

		delete composition;
		delete runner;
	}

	void testDisablingTermination_data()
	{
		QTest::addColumn<bool>("shouldDisable");

		QTest::newRow("node delays without disabling termination") << false;
		QTest::newRow("node delays and disables termination") << true;
	}
	void testDisablingTermination()
	{
		QFETCH(bool, shouldDisable);

		string compositionPath = getCompositionPath("TemporarilyDisableTermination.vuo");

		VuoRunner *runner = createRunnerInNewProcess(compositionPath);
		runner->start();

		map<VuoRunner::Port *, json_object *> m;
		VuoRunner::Port *shouldDelayPort = runner->getPublishedInputPortWithName("ShouldDelay");
		m[shouldDelayPort] = VuoBoolean_getJson(true);
		VuoRunner::Port *shouldDisablePort = runner->getPublishedInputPortWithName("ShouldDisable");
		m[shouldDisablePort] = VuoBoolean_getJson(shouldDisable);
		runner->setPublishedInputPortValues(m);

		runner->firePublishedInputPortEvent(shouldDelayPort);

		dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
		dispatch_async(queue, ^{
						   runner->stop();
					   });

		// Wait until the runtime times out waiting for the node to finish executing.
		usleep(6*USEC_PER_SEC);
		QVERIFY(runner->isStopped() == ! shouldDisable);

		if (shouldDisable)
		{
			// Wait until the node finishes executing and the composition stops.
			usleep(2*USEC_PER_SEC);
			QVERIFY(runner->isStopped());
		}

		delete runner;
	}

	void testReplacingCompositionWithoutCrashing_data()
	{
		QTest::addColumn< QString >("compositionFile");

		QTest::newRow("published trigger ports") << "PublishedInputsAndNoTrigger.vuo";
		QTest::newRow("non-repeating trigger port") << "Start.vuo";
		QTest::newRow("automatic repeating trigger port") << "Recur_Count.vuo";
		QTest::newRow("console window") << "Console.vuo";
	}
	void testReplacingCompositionWithoutCrashing()
	{
		QFETCH(QString, compositionFile);

		string compositionPath = getCompositionPath(compositionFile.toUtf8().constData());

		VuoCompilerComposition *composition = NULL;
		VuoRunner *runner = NULL;
		std::shared_ptr<VuoRunningCompositionLibraries> runningCompositionLibraries(nullptr);

		// Build and run the composition.
		{
			runner = createRunnerForLiveCoding(compositionPath, composition, runningCompositionLibraries);
			// runner->setDelegate(new TestRunnerDelegate());  /// @todo https://b33p.net/kosada/node/6021
			runner->start();
		}

		// Replace the composition with itself.
		{
			replaceCompositionForLiveCoding(compositionPath, composition, runningCompositionLibraries.get(),
											composition->getGraphvizDeclaration(), nullptr, runner);
		}

		runner->stop();

		delete runner;
		delete composition;
	}

private:

	class TestPreservingTelemetrySubscriptionsRunnerDelegate : public TestRunnerDelegate
	{
	public:
		map<string, bool> outputPortsUpdated;

		void receivedTelemetryOutputPortUpdated(string compositionIdentifier, string portIdentifier, bool sentEvent, bool sentData, string dataSummary)
		{
			outputPortsUpdated[portIdentifier] = sentData;
		}
	};

private slots:

	void testPreservingTelemetrySubscriptions_data()
	{
		QTest::addColumn< bool >("isSendingAllTelemetry");
		QTest::addColumn< bool >("isSendingEventTelemetry");
		QTest::addColumn< set<string> >("portsSendingDataTelemetry");

		set<string> noPortDataTelemetry;

		QTest::newRow("no telemetry") << false << false << noPortDataTelemetry;
		QTest::newRow("all telemetry") << true << false << noPortDataTelemetry;
		QTest::newRow("event telemetry") << false << true << noPortDataTelemetry;

		{
			set<string> portsSendingDataTelemetry;
			portsSendingDataTelemetry.insert(VuoStringUtilities::buildPortIdentifier("Count1", "count"));
			portsSendingDataTelemetry.insert(VuoStringUtilities::buildPortIdentifier("Count2", "count"));
			QTest::newRow("port data telemetry") << false << false << portsSendingDataTelemetry;
		}
	}
	void testPreservingTelemetrySubscriptions()
	{
		QFETCH(bool, isSendingAllTelemetry);
		QFETCH(bool, isSendingEventTelemetry);
		QFETCH(set<string>, portsSendingDataTelemetry);

		string compositionPath = getCompositionPath("PublishedInputsAndNoTrigger.vuo");

		VuoCompilerComposition *composition = NULL;
		VuoRunner *runner = NULL;
		std::shared_ptr<VuoRunningCompositionLibraries> runningCompositionLibraries(nullptr);

		// Build and run the composition.
		{
			runner = createRunnerForLiveCoding(compositionPath, composition, runningCompositionLibraries);
			runner->start();
		}

		// Subscribe to telemetry.
		{
			if (isSendingAllTelemetry)
				runner->subscribeToAllTelemetry("");
			if (isSendingEventTelemetry)
				runner->subscribeToEventTelemetry("");
			for (set<string>::iterator i = portsSendingDataTelemetry.begin(); i != portsSendingDataTelemetry.end(); ++i)
				runner->subscribeToOutputPortTelemetry("", *i);
		}

		// Replace the composition with itself.
		{
			replaceCompositionForLiveCoding(compositionPath, composition, runningCompositionLibraries.get(),
											composition->getGraphvizDeclaration(), nullptr, runner);
		}

		// Fire an event into the composition and collect telemetry.
		TestPreservingTelemetrySubscriptionsRunnerDelegate runnerDelegate;
		{
			runner->setDelegate(&runnerDelegate);

			VuoRunner::Port *decrementBothPort = runner->getPublishedInputPortWithName("publishedDecrementBoth");
			runner->firePublishedInputPortEvent(decrementBothPort);
		}

		runner->stop();

		delete runner;
		delete composition;

		// Check the telemetry collected.
		{
			{
				map<string, bool>::iterator portIter = runnerDelegate.outputPortsUpdated.find(VuoStringUtilities::buildPortIdentifier("Add1", "sum"));
				bool portIterFound = portIter != runnerDelegate.outputPortsUpdated.end();
				QVERIFY(portIterFound == isSendingAllTelemetry || isSendingEventTelemetry);
				if (portIterFound)
					QVERIFY(portIter->second == isSendingAllTelemetry);
			}

			for (set<string>::iterator i = portsSendingDataTelemetry.begin(); i != portsSendingDataTelemetry.end(); ++i)
			{
				map<string, bool>::iterator portIter = runnerDelegate.outputPortsUpdated.find(*i);
				QVERIFY(portIter != runnerDelegate.outputPortsUpdated.end());
				QVERIFY(portIter->second == true);
			}
		}
	}

	void testPreservingFiniCallbacks()
	{
		const char *createdFile = "/tmp/vuo.test.finiCallbackCreatesFile";
		remove(createdFile);

		string compositionPath = getCompositionPath("FiniCallbackCreatesFile.vuo");

		string bcPath;
		string dylibPath;

		VuoCompilerComposition *composition = NULL;
		VuoRunner *runner = NULL;
		std::shared_ptr<VuoRunningCompositionLibraries> runningCompositionLibraries(nullptr);

		// Build and run the composition.
		{
			runner = createRunnerForLiveCoding(compositionPath, composition, runningCompositionLibraries);
			// runner->setDelegate(new TestRunnerDelegate());  /// @todo https://b33p.net/kosada/node/6021
			runner->start();
		}

		// Replace the composition with itself.
		{
			replaceCompositionForLiveCoding(compositionPath, composition, runningCompositionLibraries.get(),
											composition->getGraphvizDeclaration(), nullptr, runner);
		}

		QVERIFY(! VuoFileUtilities::fileExists(createdFile));

		runner->stop();

		delete runner;
		delete composition;

		QVERIFY(VuoFileUtilities::fileExists(createdFile));
	}

	void testAddingResourcesToRunningComposition()
	{
		string compositionPath = getCompositionPath("PublishedInputsAndNoTrigger.vuo");

		string compositionDir, file, extension;
		VuoFileUtilities::splitPath(compositionPath, compositionDir, file, extension);
		string bcPath = VuoFileUtilities::makeTmpFile(file, "bc");
		string dylibPath = VuoFileUtilities::makeTmpFile(file, "dylib");

		VuoCompilerComposition *composition = NULL;
		VuoRunner *runner = NULL;
		std::shared_ptr<VuoRunningCompositionLibraries> runningCompositionLibraries = std::make_shared<VuoRunningCompositionLibraries>();

		// Build and run an empty composition.
		{
			composition = new VuoCompilerComposition(new VuoComposition(), NULL);

			VuoCompilerIssues issues;
			compiler->compileComposition(composition, bcPath, true, &issues);
			compiler->linkCompositionToCreateDynamicLibraries(bcPath, dylibPath, runningCompositionLibraries.get());
			remove(bcPath.c_str());

			runner = VuoRunner::newSeparateProcessRunnerFromDynamicLibrary(compiler->getCompositionLoaderPath(), dylibPath, runningCompositionLibraries, compositionDir, false, true);
			// runner->setDelegate(new TestRunnerDelegate());  /// @todo https://b33p.net/kosada/node/6021
			runner->start();
		}

		// Replace the composition with one in which a new node class has been added.
		{
			string oldCompositionGraphviz = composition->getGraphvizDeclaration();

			// Add "FireOnStart1".
			// (Unlike a stateless, non-event-firing node class, this will appear in the
			// compiled composition even without any incoming cables.)
			VuoCompilerNodeClass *fireOnStartNodeClass = compiler->getNodeClass("vuo.event.fireOnStart");
			VuoNode *fireOnStartNode = fireOnStartNodeClass->newNode("FireOnStart1");
			composition->getBase()->addNode(fireOnStartNode);

			replaceCompositionForLiveCoding(compositionPath, composition, runningCompositionLibraries.get(),
											oldCompositionGraphviz, nullptr, runner);
		}

		// Replace the composition with one in which a new node class with library dependencies has been added.
		{
			string oldCompositionGraphviz = composition->getGraphvizDeclaration();

			// Add "DisplayConsoleWindow1".
			VuoCompilerNodeClass *displayConsoleWindowNodeClass = compiler->getNodeClass("vuo.console.window");
			VuoNode *displayConsoleWindowNode = displayConsoleWindowNodeClass->newNode("DisplayConsoleWindow1");
			composition->getBase()->addNode(displayConsoleWindowNode);

			replaceCompositionForLiveCoding(compositionPath, composition, runningCompositionLibraries.get(),
											oldCompositionGraphviz, nullptr, runner);
		}

		runner->stop();

		delete runner;
		delete composition;
	}

	void testLiveCoding_data()
	{
		QTest::addColumn<int>("testNum");
		QTest::addColumn<int>("finalSum");

		int testNum = 0;
		QTest::newRow("no change") << testNum++ << 10110;
		QTest::newRow("cable endpoint changed") << testNum++ << 10220;
		QTest::newRow("stateful node added") << testNum++ << 12330;
		QTest::newRow("drawer resized") << testNum++ << 10110;
		QTest::newRow("published port added") << testNum++ << 10100;
		QTest::newRow("all published ports removed") << testNum++ << 10010;
		QTest::newRow("all published input ports removed") << testNum++ << 10010;
		QTest::newRow("published port renamed") << testNum++ << 10110;
		QTest::newRow("published port type changed") << testNum++ << 10110;
	}
	void testLiveCoding()
	{
		QFETCH(int, testNum);
		QFETCH(int, finalSum);

		string compositionPath = getCompositionPath("PublishedInputsAndNoTrigger.vuo");

		VuoCompilerComposition *composition = NULL;
		VuoRunner *runner = NULL;
		std::shared_ptr<VuoRunningCompositionLibraries> runningCompositionLibraries(nullptr);
		VuoRunner::Port *publishedIncrementOne = NULL;
		VuoRunner::Port *publishedSum = NULL;
		string originalCompositionGraphviz;

		// Build and run the original composition.
		{
			runner = createRunnerForLiveCoding(compositionPath, composition, runningCompositionLibraries);

			originalCompositionGraphviz = composition->getGraphvizDeclaration();

			// runner->setDelegate(new TestRunnerDelegate());  /// @todo https://b33p.net/kosada/node/6021
			runner->start();
			runner->subscribeToEventTelemetry("");

			publishedIncrementOne = runner->getPublishedInputPortWithName("publishedIncrementOne");
			QVERIFY(publishedIncrementOne != NULL);
			publishedSum = runner->getPublishedOutputPortWithName("publishedSum");
			QVERIFY(publishedSum != NULL);

			// "Count1:increment" becomes 10, "Count1:count" becomes 10,
			// "Add1:sum" becomes 10 + 0 = 10.
			map<VuoRunner::Port *, json_object *> m;
			m[publishedIncrementOne] = VuoInteger_getJson(10);
			runner->setPublishedInputPortValues(m);

			runner->firePublishedInputPortEvent(publishedIncrementOne);
			runner->waitForFiredPublishedInputPortEvent();
			QCOMPARE(VuoInteger_makeFromJson( runner->getPublishedOutputPortValue(publishedSum) ), (VuoInteger)10);
		}

		VuoNode *count1Node = NULL;
		VuoNode *count2Node = NULL;
		VuoNode *makeList1Node = NULL;
		foreach (VuoNode *n, composition->getBase()->getNodes())
		{
			if (n->getTitle() == "Count1")
				count1Node = n;
			else if (n->getTitle() == "Count2")
				count2Node = n;
			else if (n->getTitle() == "MakeList1")
				makeList1Node = n;
		}
		QVERIFY(count1Node != NULL);
		QVERIFY(count2Node != NULL);
		QVERIFY(makeList1Node != NULL);

		if (testNum == 0)  // no change
		{
			replaceCompositionForLiveCoding(compositionPath, composition, runningCompositionLibraries.get(),
											originalCompositionGraphviz, nullptr, runner);

			// "Count1:increment" becomes 100, "Count1:count" becomes 110,
			// "Add1:sum" becomes 110 + 0 = 110.
			map<VuoRunner::Port *, json_object *> m;
			m[publishedIncrementOne] = VuoInteger_getJson(100);
			runner->setPublishedInputPortValues(m);

			runner->firePublishedInputPortEvent(publishedIncrementOne);
			runner->waitForFiredPublishedInputPortEvent();
			QCOMPARE(VuoInteger_makeFromJson( runner->getPublishedOutputPortValue(publishedSum) ), (VuoInteger)110);
		}

		else if (testNum == 1)  // cable endpoint changed
		{
			VuoCable *cableToModify = NULL;
			foreach (VuoCable *c, composition->getBase()->getCables())
			{
				if (c->getFromNode() == count2Node && c->getToNode() == makeList1Node)
					cableToModify = c;
			}
			QVERIFY(cableToModify != NULL);

			// Change "Count2:count -> MakeList1:2" to "Count1:count -> MakeList1:2".
			VuoPort *count1NodeCountPort = count1Node->getOutputPortWithName("count");
			QVERIFY(count1NodeCountPort != NULL);
			cableToModify->setFrom(count1Node, count1NodeCountPort);

			replaceCompositionForLiveCoding(compositionPath, composition, runningCompositionLibraries.get(),
											originalCompositionGraphviz, nullptr, runner);

			// "Count1:increment" becomes 100, "Count1:count" becomes 110,
			// "Add1:sum" becomes 110 + 110 = 220.
			map<VuoRunner::Port *, json_object *> m;
			m[publishedIncrementOne] = VuoInteger_getJson(100);
			runner->setPublishedInputPortValues(m);

			runner->firePublishedInputPortEvent(publishedIncrementOne);
			runner->waitForFiredPublishedInputPortEvent();
			QCOMPARE(VuoInteger_makeFromJson( runner->getPublishedOutputPortValue(publishedSum) ), (VuoInteger)220);

			// When the composition is reverted, "MakeList1:2" remains at 110.
		}

		else if (testNum == 2)  // stateful node added
		{
			// Add "Count3".
			VuoCompilerNodeClass *countNodeClass = count1Node->getNodeClass()->getCompiler();
			VuoNode *count3Node = countNodeClass->newNode("Count3");
			composition->getBase()->addNode(count3Node);

			// Remove "Count2:count -> MakeList1:2".
			VuoCable *count2ToMakeList1Cable = NULL;
			foreach (VuoCable *c, composition->getBase()->getCables())
			{
				if (c->getFromNode() == count2Node && c->getToNode() == makeList1Node)
					count2ToMakeList1Cable = c;
			}
			QVERIFY(count2ToMakeList1Cable != NULL);
			composition->getBase()->removeCable(count2ToMakeList1Cable);

			// Add "Count1:count -> Count3:increment".
			VuoCompilerPort *count1NodeCountPort = static_cast<VuoCompilerPort *>(count1Node->getOutputPortWithName("count")->getCompiler());
			VuoCompilerPort *count3NodeIncrementPort = static_cast<VuoCompilerPort *>(count3Node->getInputPortWithName("increment")->getCompiler());
			VuoCompilerCable *count1ToCount3Cable = new VuoCompilerCable(count1Node->getCompiler(), count1NodeCountPort,
																		 count3Node->getCompiler(), count3NodeIncrementPort);
			composition->getBase()->addCable(count1ToCount3Cable->getBase());

			// Add "Count3:count -> MakeList1:2".
			VuoCompilerPort *count3NodeCountPort = static_cast<VuoCompilerPort *>(count3Node->getOutputPortWithName("count")->getCompiler());
			VuoCompilerPort *makeList1NodeItem2Port = static_cast<VuoCompilerPort *>(makeList1Node->getInputPortWithName("2")->getCompiler());
			VuoCompilerCable *count3ToAdd1Cable = new VuoCompilerCable(count3Node->getCompiler(), count3NodeCountPort,
																	   makeList1Node->getCompiler(), makeList1NodeItem2Port);
			composition->getBase()->addCable(count3ToAdd1Cable->getBase());

			replaceCompositionForLiveCoding(compositionPath, composition, runningCompositionLibraries.get(),
											originalCompositionGraphviz, nullptr, runner);

			// "Count1:increment" becomes 100, "Count1:count" becomes 110,
			// "Count3:increment" becomes 110, "Count3:count" becomes 110,
			// "Add1:sum" becomes 110 + 110 = 220.
			map<VuoRunner::Port *, json_object *> m;
			m[publishedIncrementOne] = VuoInteger_getJson(100);
			runner->setPublishedInputPortValues(m);

			runner->firePublishedInputPortEvent(publishedIncrementOne);
			runner->waitForFiredPublishedInputPortEvent();
			QCOMPARE(VuoInteger_makeFromJson( runner->getPublishedOutputPortValue(publishedSum) ), (VuoInteger)220);

			// "Count1:increment" becomes 1000, "Count1:count" becomes 1110,
			// "Count3:increment" becomes 1110, "Count3:count" becomes 1220,
			// "Add1:sum" becomes 1110 + 1220 = 2330.
			m[publishedIncrementOne] = VuoInteger_getJson(1000);
			runner->setPublishedInputPortValues(m);

			runner->firePublishedInputPortEvent(publishedIncrementOne);
			runner->waitForFiredPublishedInputPortEvent();
			QCOMPARE(VuoInteger_makeFromJson( runner->getPublishedOutputPortValue(publishedSum) ), (VuoInteger)2330);

			// When the composition is reverted, "MakeList1:2" remains at 1220.
		}

		else if (testNum == 3)  // drawer resized
		{
			// Replace 2-input "MakeList1" with 3-input "MakeList1".
			VuoCompilerNodeClass *makeListNodeClass = compiler->getNodeClass("vuo.list.make.3.VuoInteger");
			VuoNode *makeList2Node = makeListNodeClass->newNode("MakeList1");
			makeList2Node->getCompiler()->setGraphvizIdentifier( makeList1Node->getCompiler()->getGraphvizIdentifier() );
			composition->replaceNode(makeList1Node, makeList2Node);

			replaceCompositionForLiveCoding(compositionPath, composition, runningCompositionLibraries.get(),
											originalCompositionGraphviz, nullptr, runner);

			// "Count1:increment" becomes 100, "Count1:count" becomes 110,
			// "Add1:sum" becomes 110 + 0 = 110.
			map<VuoRunner::Port *, json_object *> m;
			m[publishedIncrementOne] = VuoInteger_getJson(100);
			runner->setPublishedInputPortValues(m);

			runner->firePublishedInputPortEvent(publishedIncrementOne);
			runner->waitForFiredPublishedInputPortEvent();
			QCOMPARE(VuoInteger_makeFromJson( runner->getPublishedOutputPortValue(publishedSum) ), (VuoInteger)110);
		}

		else if (testNum == 4)  // published port added
		{
			// Add "publishedSetOne".
			VuoCompilerType *type = compiler->getType("VuoInteger");
			VuoCompilerPublishedPortClass *portClass = new VuoCompilerPublishedPortClass("publishedSetOne", VuoPortClass::dataAndEventPort, type->getType());
			portClass->setDataVuoType(type->getBase());
			VuoPublishedPort *publishedPort = static_cast<VuoPublishedPort *>(portClass->newPort()->getBase());
			int index = composition->getBase()->getPublishedInputPorts().size();
			composition->getBase()->addPublishedInputPort(publishedPort, index);

			// Add "publishedSetOne -> Count1:setCount".
			VuoCompilerPort *compilerPublishedPort = static_cast<VuoCompilerPort *>(publishedPort->getCompiler());
			VuoCompilerPort *count1NodeSetPort = static_cast<VuoCompilerPort *>(count1Node->getInputPortWithName("setCount")->getCompiler());
			VuoCompilerCable *publishedCable = new VuoCompilerCable(NULL, compilerPublishedPort, count1Node->getCompiler(), count1NodeSetPort);
			composition->getBase()->addCable(publishedCable->getBase());

			replaceCompositionForLiveCoding(compositionPath, composition, runningCompositionLibraries.get(),
											originalCompositionGraphviz, nullptr, runner);

			// "Count1:setCount" becomes 100, "Count1:count" becomes 100,
			// "Add1:sum" becomes 100 + 0 = 100.
			VuoRunner::Port *publishedSetOne = runner->getPublishedInputPortWithName("publishedSetOne");
			map<VuoRunner::Port *, json_object *> m;
			m[publishedSetOne] = VuoInteger_getJson(100);
			runner->setPublishedInputPortValues(m);

			runner->firePublishedInputPortEvent(publishedSetOne);
			runner->waitForFiredPublishedInputPortEvent();
			QCOMPARE(VuoInteger_makeFromJson( runner->getPublishedOutputPortValue(publishedSum) ), (VuoInteger)100);
		}

		else if (testNum == 5)  // all published ports removed
		{
			for (int i = composition->getBase()->getPublishedInputPorts().size() - 1; i >= 0; --i)
			{
				for (VuoCable *publishedCable : composition->getBase()->getPublishedInputPorts().at(i)->getConnectedCables(true))
					composition->getBase()->removeCable(publishedCable);

				composition->getBase()->removePublishedInputPort(i);
			}

			for (int i = composition->getBase()->getPublishedOutputPorts().size() - 1; i >= 0; --i)
			{
				for (VuoCable *publishedCable : composition->getBase()->getPublishedOutputPorts().at(i)->getConnectedCables(true))
					composition->getBase()->removeCable(publishedCable);

				composition->getBase()->removePublishedOutputPort(i);
			}

			replaceCompositionForLiveCoding(compositionPath, composition, runningCompositionLibraries.get(),
											originalCompositionGraphviz, nullptr, runner);

			// Don't test anything here, but below test that the published ports get added back in.
		}

		else if (testNum == 6)  // all published input ports removed
		{
			for (int i = composition->getBase()->getPublishedInputPorts().size() - 1; i >= 0; --i)
			{
				for (VuoCable *publishedCable : composition->getBase()->getPublishedInputPorts().at(i)->getConnectedCables(true))
					composition->getBase()->removeCable(publishedCable);

				composition->getBase()->removePublishedInputPort(i);
			}

			replaceCompositionForLiveCoding(compositionPath, composition, runningCompositionLibraries.get(),
											originalCompositionGraphviz, nullptr, runner);

			// "Add1:sum" stays at 10.
			QCOMPARE(VuoInteger_makeFromJson( runner->getPublishedOutputPortValue(publishedSum) ), (VuoInteger)10);
		}

		else if (testNum == 7)  // published port renamed
		{
			VuoPublishedPort *publishedPort = composition->getBase()->getPublishedInputPortWithName("publishedIncrementOne");
			publishedPort->getClass()->setName("publishedIncrementOneRenamed");

			replaceCompositionForLiveCoding(compositionPath, composition, runningCompositionLibraries.get(),
											originalCompositionGraphviz, nullptr, runner);

			// "Count1:increment" becomes 100, "Count1:count" becomes 110,
			// "Add1:sum" becomes 110 + 0 = 110.
			VuoRunner::Port *publishedIncrementOneRenamed = runner->getPublishedInputPortWithName("publishedIncrementOneRenamed");
			map<VuoRunner::Port *, json_object *> m;
			m[publishedIncrementOneRenamed] = VuoInteger_getJson(100);
			runner->setPublishedInputPortValues(m);

			runner->firePublishedInputPortEvent(publishedIncrementOneRenamed);
			runner->waitForFiredPublishedInputPortEvent();
			QCOMPARE(VuoInteger_makeFromJson( runner->getPublishedOutputPortValue(publishedSum) ), (VuoInteger)110);
		}

		else if (testNum == 8)  // published port type changed
		{
			// Remove "publishedIncrementOne -> Count1:increment".
			VuoPublishedPort *publishedPort = composition->getBase()->getPublishedInputPortWithName("publishedIncrementOne");
			VuoCable *publishedToCount1Cable = NULL;
			foreach (VuoCable *c, composition->getBase()->getCables())
			{
				if (c->getFromPort() == publishedPort && c->getToNode() == count1Node)
					publishedToCount1Cable = c;
			}
			QVERIFY(publishedToCount1Cable != NULL);
			composition->getBase()->removeCable(publishedToCount1Cable);

			// Change "publishedIncrementOne" type to VuoReal.
			VuoCompilerPort *compilerPublishedPort = static_cast<VuoCompilerPort *>(publishedPort->getCompiler());
			VuoCompilerType *realType = compiler->getType("VuoReal");
			compilerPublishedPort->setDataVuoType(realType->getBase());

			// Add "Round".
			VuoCompilerNodeClass *roundNodeClass = compiler->getNodeClass("vuo.math.round");
			VuoNode *roundNode = roundNodeClass->newNode("Round");
			composition->getBase()->addNode(roundNode);

			// Add "publishedIncrementOne -> Round:real".
			VuoCompilerPort *roundNodeRealPort = static_cast<VuoCompilerPort *>(roundNode->getInputPortWithName("real")->getCompiler());
			VuoCompilerCable *publishedCable = new VuoCompilerCable(NULL, compilerPublishedPort, roundNode->getCompiler(), roundNodeRealPort);
			composition->getBase()->addCable(publishedCable->getBase());

			// Add "Round:rounded -> Count1:increment".
			VuoCompilerPort *roundNodeRoundedPort = static_cast<VuoCompilerPort *>(roundNode->getOutputPortWithName("rounded")->getCompiler());
			VuoCompilerPort *count1NodeIncrementPort = static_cast<VuoCompilerPort *>(count1Node->getInputPortWithName("increment")->getCompiler());
			VuoCompilerCable *roundToCount1Cable = new VuoCompilerCable(roundNode->getCompiler(), roundNodeRoundedPort,
																		count1Node->getCompiler(), count1NodeIncrementPort);
			composition->getBase()->addCable(roundToCount1Cable->getBase());

			replaceCompositionForLiveCoding(compositionPath, composition, runningCompositionLibraries.get(),
											originalCompositionGraphviz, nullptr, runner);

			// "Count1:increment" becomes 100, "Count1:count" becomes 110,
			// "Add1:sum" becomes 110 + 0 = 110.
			map<VuoRunner::Port *, json_object *> m;
			m[publishedIncrementOne] = VuoInteger_getJson(100);
			runner->setPublishedInputPortValues(m);

			runner->firePublishedInputPortEvent(publishedIncrementOne);
			runner->waitForFiredPublishedInputPortEvent();
			QCOMPARE(VuoInteger_makeFromJson( runner->getPublishedOutputPortValue(publishedSum) ), (VuoInteger)110);
		}

		// Replace the latest composition with the original composition.
		{
			VuoCompilerComposition *originalComposition = VuoCompilerComposition::newCompositionFromGraphvizDeclaration(originalCompositionGraphviz, compiler);

			replaceCompositionForLiveCoding(compositionPath, originalComposition, runningCompositionLibraries.get(),
											composition->getGraphvizDeclaration(), nullptr, runner);

			delete originalComposition;

			// "Count1:increment" becomes 10000, "Count1:count" becomes X,
			// "Add1:sum" becomes X + Y = finalSum.
			map<VuoRunner::Port *, json_object *> m;
			m[publishedIncrementOne] = VuoInteger_getJson(10000);
			runner->setPublishedInputPortValues(m);

			runner->firePublishedInputPortEvent(publishedIncrementOne);
			runner->waitForFiredPublishedInputPortEvent();
			QCOMPARE(VuoInteger_makeFromJson( runner->getPublishedOutputPortValue(publishedSum) ), (VuoInteger)finalSum);
		}

		runner->stop();

		delete runner;
		delete composition;
	}

};

QTEST_APPLESS_MAIN(TestControlAndTelemetry)
#include "TestControlAndTelemetry.moc"

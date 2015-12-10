/**
 * @file
 * VuoRunner implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoNodeClass.hh"
#include "VuoRunner.hh"
#include "VuoFileUtilities.hh"

#include <CoreFoundation/CoreFoundation.h>
#include <pthread.h>
#include <stdio.h>
#include <dlfcn.h>
#include <sstream>
#include <objc/runtime.h>
#include <objc/message.h>

static pthread_t mainThread = NULL;	///< A reference to the main thread
/**
 * Get a reference to the main thread, so we can perform runtime thread-sanity assertions.
 */
static void __attribute__((constructor)) thisFunctionIsCalledAtStartup()
{
	mainThread = pthread_self();

	// Processes using VuoRunner are also likely to use Vuo Types, which need reference counting support.
	// If the code using VuoRunner links to libVuoHeap.dylib, initialize the reference counting system.
	typedef void (*initType)(void);
	initType VuoHeap_init = (initType) dlsym(RTLD_SELF, "VuoHeap_init");
	if (VuoHeap_init)
		VuoHeap_init();
}
/**
 * Is the current thread the main thread?
 */
static bool isMainThread(void)
{
	return mainThread == pthread_self();
}

/**
 * Applies standard settings to the specified ZMQ socket.
 */
static void VuoRunner_configureSocket(void *zmqSocket)
{
	int timeout = 5 * 1000; // 5 seconds
	zmq_setsockopt(zmqSocket, ZMQ_RCVTIMEO, &timeout, sizeof timeout);
	zmq_setsockopt(zmqSocket, ZMQ_SNDTIMEO, &timeout, sizeof timeout);
}

/**
 * Creates a runner object that can run the composition in file @a compositionPath in a new process.
 */
VuoRunner * VuoRunner::newSeparateProcessRunnerFromCompositionFile(string compositionPath)
{
	string compositionDir, compositionFile, compositionExt;
	VuoFileUtilities::splitPath(compositionPath, compositionDir, compositionFile, compositionExt);
	string compiledCompositionPath = VuoFileUtilities::makeTmpFile(compositionFile, "bc");

	string compiledCompositionDir, compiledCompositionFile, compiledCompositionExt;
	VuoFileUtilities::splitPath(compiledCompositionPath, compiledCompositionDir, compiledCompositionFile, compiledCompositionExt);
	string linkedCompositionPath = compiledCompositionDir + compiledCompositionFile;

	string vuoFrameworkDir = VuoFileUtilities::getVuoFrameworkPath();
	string vuoCompilePath = vuoFrameworkDir + "/Helpers/vuo-compile";
	string vuoLinkPath = vuoFrameworkDir + "/Helpers/vuo-link";

	// vuo-compile --output /tmp/composition.bc composition.vuo
	pid_t compilePid = fork();
	if (compilePid == 0)
	{
		execl(vuoCompilePath.c_str(), "vuo-compile", "--output", compiledCompositionPath.c_str(), compositionPath.c_str(), NULL);
		VLog("Error: Couldn't run '%s'.", vuoCompilePath.c_str());
		return NULL;
	}
	else if (compilePid > 0)
	{
		int status;
		waitpid(compilePid, &status, 0);
	}
	else
	{
		VLog("Error: Couldn't fork '%s'.", vuoCompilePath.c_str());
		return NULL;
	}

	// vuo-link --output /tmp/composition composition.bc
	pid_t linkPid = fork();
	if (linkPid == 0)
	{
		execl(vuoLinkPath.c_str(), "vuo-link", "--output", linkedCompositionPath.c_str(), compiledCompositionPath.c_str(), NULL);
		VLog("Error: Couldn't run '%s'.", vuoLinkPath.c_str());
		return NULL;
	}
	else if (linkPid > 0)
	{
		int status;
		waitpid(linkPid, &status, 0);
	}
	else
	{
		VLog("Error: Couldn't fork '%s'.", vuoLinkPath.c_str());
		return NULL;
	}

	remove(compiledCompositionPath.c_str());

	return newSeparateProcessRunnerFromExecutable(linkedCompositionPath, compositionDir, true);
}

/**
 * Creates a runner that can run a composition in a new process.
 *
 * @param executablePath A linked composition executable, produced by VuoCompiler::linkCompositionToCreateExecutable().
 * @param sourceDir The directory containing the composition (.vuo) source file, used by nodes in the composition to resolve relative paths.
 * @param deleteExecutableWhenFinished True if the runner should delete @c executablePath when it's finished using the file.
 *
 * @see CompileAndRunInNewProcess.cc
 */
VuoRunner * VuoRunner::newSeparateProcessRunnerFromExecutable(string executablePath, string sourceDir,
															  bool deleteExecutableWhenFinished)
{
	VuoRunner * vr = new VuoRunner();
	vr->executablePath = executablePath;
	vr->shouldDeleteBinariesWhenFinished = deleteExecutableWhenFinished;
	vr->sourceDir = sourceDir;
	return vr;
}

/**
 * Creates a runner object that can run a composition in a new process
 * and replace the composition with a new version while it's running.
 *
 * @param compositionLoaderPath The VuoCompositionLoader executable.
 * @param compositionDylibPath A linked composition dynamic library, produced by VuoCompiler::linkCompositionToCreateDynamicLibraries().
 * @param resourceDylibPath A linked resource dynamic library, produced by the first call to VuoCompiler::linkCompositionToCreateDynamicLibraries() for this runner.
 * @param sourceDir The directory containing the composition (.vuo) source file, used by nodes in the composition to resolve relative paths.
 * @param deleteDylibsWhenFinished True if the runner should delete @c compositionDylibPath and @c resourceDylibPath when it's finished using the files.
 */
VuoRunner * VuoRunner::newSeparateProcessRunnerFromDynamicLibrary(string compositionLoaderPath, string compositionDylibPath,
																  string resourceDylibPath, string sourceDir,
																  bool deleteDylibsWhenFinished)
{
	VuoRunner * vr = new VuoRunner();
	vr->executablePath = compositionLoaderPath;
	vr->dylibPath = compositionDylibPath;
	vr->resourceDylibPaths.push_back(resourceDylibPath);
	vr->shouldDeleteBinariesWhenFinished = deleteDylibsWhenFinished;
	vr->sourceDir = sourceDir;
	return vr;
}

/**
 * Creates a runner object that can run a composition in the current process.
 *
 * @param dylibPath A linked composition dynamic library, produced by VuoCompiler::linkCompositionToCreateDynamicLibrary().
 * @param sourceDir The directory containing the composition (.vuo) source file, used by nodes in the composition to resolve relative paths.
 * @param deleteDylibWhenFinished True if the runner should delete @c dylibPath when it's finished using the file.
 *
 * @only64Bit
 *
 * @see CompileAndRunInCurrentProcess.cc
 */
VuoRunner * VuoRunner::newCurrentProcessRunnerFromDynamicLibrary(string dylibPath, string sourceDir,
																 bool deleteDylibWhenFinished)
{
	VuoRunner * vr = new VuoRunner();
	vr->dylibPath = dylibPath;
	vr->shouldDeleteBinariesWhenFinished = deleteDylibWhenFinished;
	vr->sourceDir = sourceDir;
	return vr;
}

/**
 * Destructor.
 *
 * Assumes the composition either has not been started or has been started and stopped.
 */
VuoRunner::~VuoRunner(void)
{
	dispatch_release(stoppedSemaphore);
	dispatch_release(beganListeningSemaphore);
	dispatch_release(endedListeningSemaphore);
	dispatch_release(delegateQueue);
	dispatch_release(anyPublishedPortEventSemaphore);
}

/**
 * Private constructor, used by factory methods.
 */
VuoRunner::VuoRunner(void)
{
	dylibHandle = NULL;
	shouldDeleteBinariesWhenFinished = false;
	stopped = true;
	listenCanceled = false;
	stoppedSemaphore = dispatch_semaphore_create(1);
	beganListeningSemaphore = dispatch_semaphore_create(0);
	endedListeningSemaphore = dispatch_semaphore_create(1);
	controlQueue = dispatch_queue_create("org.vuo.runner.control", NULL);
	ZMQContext = NULL;
	ZMQControl = NULL;
	ZMQTelemetry = NULL;
	ZMQLoaderControl = NULL;
	delegate = NULL;
	delegateQueue = dispatch_queue_create("org.vuo.runner.delegate", NULL);
	arePublishedInputPortsCached = false;
	arePublishedOutputPortsCached = false;
	anyPublishedPortEventSemaphore = dispatch_semaphore_create(0);
	anyPublishedPortEventSignaled = false;
}

/**
 * Starts the composition running.
 *
 * If running the composition in the current process, a call to this method must be followed by
 * either a call to runOnMainThread() or repeated calls to drainMainDispatchQueue()
 * in order to run the composition.
 *
 * If running the composition in a separate process, no further calls are needed to run the composition.
 *
 * If running the composition in the current process, the current working directory is changed to the
 * composition source directory that was passed to newCurrentProcessRunnerFromDynamicLibrary()
 * until the composition is stopped.
 *
 * Assumes the composition is not already running.
 */
void VuoRunner::start(void)
{
	start(false);
}

/**
 * Starts the composition running, but in a paused state.
 *
 * This is useful if you want to call setPublishedInputPortValue() before the composition
 * begins firing events. To unpause the composition, call unpause().
 *
 * If running the composition in the current process, a call to this method must be followed by
 * either a call to runOnMainThread() or repeated calls to drainMainDispatchQueue()
 * in order to run the composition.
 *
 * If running the composition in a separate process, no further calls are needed.
 *
 * If running the composition in the current process, the current working directory is changed to the
 * composition source directory that was passed to newCurrentProcessRunnerFromDynamicLibrary()
 * until the composition is stopped.
 *
 * Assumes the composition is not already running.
 */
void VuoRunner::startPaused(void)
{
	start(true);
}

/**
 * Internal helper method for starting the composition running.
 *
 * Assumes the composition is not already running.
 *
 * @param isPaused If true, the composition is initially in a paused state.
 */
void VuoRunner::start(bool isPaused)
{
	stopped = false;
	dispatch_semaphore_wait(stoppedSemaphore, DISPATCH_TIME_FOREVER);

	ZMQContext = zmq_init(1);

	if (isInCurrentProcess())
	{
		// Start the composition in the current process.

		dylibHandle = dlopen(dylibPath.c_str(), RTLD_NOW);
		if (!dylibHandle)
		{
			VLog("Error: Couldn't load dylib: '%s'.", dlerror());
			return;
		}

		typedef void(* initInProcessType)(void *_ZMQContext, const char *controlURL, const char *telemetryURL, bool _isPaused, pid_t _runnerPid);

		initInProcessType vuoInitInProcess = (initInProcessType)dlsym(dylibHandle, "vuoInitInProcess");
		if (! vuoInitInProcess)
		{
			VLog("Error: Couldn't find function vuoInitInProcess(): %s", dlerror());
			return;
		}

		ZMQControlURL = "inproc://" + VuoFileUtilities::makeTmpFile("vuo-control", "");
		ZMQTelemetryURL = "inproc://" + VuoFileUtilities::makeTmpFile("vuo-telemetry", "");

		// Set the current working directory to that of the source .vuo composition so that
		// relative URL paths are resolved correctly.
		char buffer[PATH_MAX];
		getcwd(buffer, PATH_MAX);
		originalWorkingDir = buffer;
		if (!sourceDir.empty())
			chdir(sourceDir.c_str());

		vuoInitInProcess(ZMQContext, ZMQControlURL.c_str(), ZMQTelemetryURL.c_str(), isPaused, getpid());
	}
	else
	{
		// Start the composition or composition loader in a new process.

		vector<string> args;

		string executableName;
		if (isUsingCompositionLoader())
		{
			// If we're using the loader, set the executable's display name to the dylib,
			// so that composition's name shows up in the process list.
			string dir, file, ext;
			VuoFileUtilities::splitPath(dylibPath, dir, file, ext);
			executableName = file;
		}
		else
		{
			string dir, file, ext;
			VuoFileUtilities::splitPath(executablePath, dir, file, ext);
			string executableName = file;
			if (! ext.empty())
				executableName += "." + ext;
		}
		args.push_back(executableName);

		ZMQControlURL = "ipc://" + VuoFileUtilities::makeTmpFile("vuo-control", "");
		ZMQTelemetryURL = "ipc://" + VuoFileUtilities::makeTmpFile("vuo-telemetry", "");
		args.push_back("--vuo-control=" + ZMQControlURL);
		args.push_back("--vuo-telemetry=" + ZMQTelemetryURL);

		pid_t pid = getpid();
		ostringstream oss;
		oss << pid;
		args.push_back("--vuo-runner=" + oss.str());

		if (isUsingCompositionLoader())
		{
			ZMQLoaderControlURL = "ipc://" + VuoFileUtilities::makeTmpFile("vuo-loader", "");
			args.push_back("--vuo-loader=" + ZMQLoaderControlURL);
		}

		if (isPaused)
		{
			args.push_back("--vuo-pause");
		}

		char * argv[7];
		for (size_t i = 0; i < args.size(); ++i)
		{
			argv[i] = (char *) malloc(args[i].length() + 1);
			strcpy(argv[i], args[i].c_str());
		}
		argv[args.size()] = NULL;

		int fd[2];
		int ret = pipe(fd);
		if (ret != 0)
		{
			VLog("Error: Couldn't open pipe to parent of composition process.");
			return;
		}

		pid_t childPid = fork();
		if (childPid == 0)
		{
			pid_t grandchildPid = fork();
			if (grandchildPid == 0)
			{
				// Set the current working directory to that of the source .vuo composition so that
				// relative URL paths are resolved correctly.
				if (!sourceDir.empty())
					chdir(sourceDir.c_str());

				execv(executablePath.c_str(), argv);
			}
			else
			{
				close(fd[0]);
				write(fd[1], &grandchildPid, sizeof(pid_t));
			}

			_exit(0);  // Can't safely call exit() in child after fork(). Instead call _exit().
		}
		else if (childPid > 0)
		{
			close(fd[1]);
			pid_t grandchildPid;
			read(fd[0], &grandchildPid, sizeof(pid_t));

			if (grandchildPid > 0)
				compositionPid = grandchildPid;
			else
			{
				VLog("Error: Couldn't fork composition process.");
				return;
			}
		}
		else
		{
			VLog("Error: Couldn't fork parent of composition process.");
			return;
		}
	}

	// Connect to the composition loader (if any) and composition.
	if (isUsingCompositionLoader())
	{
		ZMQLoaderControl = zmq_socket(ZMQContext,ZMQ_REQ);
		VuoRunner_configureSocket(ZMQLoaderControl);

		// Try to connect to the composition loader. If at first we don't succeed, give the composition loader a little more time to set up the socket.
		int numTries = 0;
		while (zmq_connect(ZMQLoaderControl,ZMQLoaderControlURL.c_str()))
		{
			if (++numTries == 1000)
			{
				VLog("Error: VuoLoaderControl connect failed.");
				return;
			}
			usleep(USEC_PER_SEC / 1000);
		}

		replaceComposition(dylibPath, resourceDylibPaths.at(0), "");
	}
	else
	{
		setUpConnections();
	}
}

/**
 * Establishes ZMQ connections with the running composition.
 */
void VuoRunner::setUpConnections(void)
{
	ZMQControl = zmq_socket(ZMQContext,ZMQ_REQ);
	VuoRunner_configureSocket(ZMQControl);

	// Try to connect to the composition. If at first we don't succeed, give the composition a little more time to set up the socket.
	int numTries = 0;
	while (zmq_connect(ZMQControl,ZMQControlURL.c_str()))
	{
		if (++numTries == 1000)
		{
			VLog("Error: VuoControl connect failed.");
			return;
		}
		usleep(USEC_PER_SEC / 1000);
	}

	// Cache published ports so they're available whenever a caller starts listening for published port value changes.
	arePublishedInputPortsCached = false;
	arePublishedOutputPortsCached = false;
	if (isInCurrentProcess())
	{
		dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
		dispatch_async(queue, ^{
						   getCachedPublishedPorts(false);
						   getCachedPublishedPorts(true);
					   });
		while (! arePublishedOutputPortsCached)
		{
			drainMainDispatchQueue();
			usleep(USEC_PER_SEC / 10);
		}
	}
	else
	{
		getCachedPublishedPorts(false);
		getCachedPublishedPorts(true);
	}

	listenCanceled = false;
	dispatch_semaphore_wait(endedListeningSemaphore, DISPATCH_TIME_FOREVER);
	dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
	dispatch_async(queue, ^{
					   listen();
				   });
	dispatch_semaphore_wait(beganListeningSemaphore, DISPATCH_TIME_FOREVER);
}

/**
 * For a composition in the current process, runs the composition on the main thread until it stops
 * (either on its own or from a call to stop() on another thread).
 *
 * Call this function after start() or startPaused().
 *
 * There's no need to call this function if your application calls @c dispatch_main(), @c NSApplicationMain(),
 * or @c UIApplicationMain(), or if it invokes a @c CFRunLoop on the main thread.
 *
 * Throws a @c std::logic_error if this runner was not constructed to run the composition in the current process
 * or if this function was not called on the main thread.
 *
 * @only64Bit
 *
 * @see drainMainDispatchQueue(), an alternative to this function.
 */
void VuoRunner::runOnMainThread(void)
{
	if (! isInCurrentProcess())
		throw std::logic_error("The composition is not running in the current process. Only use this function if the composition was constructed with newCurrentProcessRunnerFromDynamicLibrary().");

	if (! isMainThread())
		throw std::logic_error("This is not the main thread. Only call this function from the main thread.");

	while (! stopped)
	{
		id pool = objc_msgSend((id)objc_getClass("NSAutoreleasePool"), sel_getUid("new"));
		CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.01,false);
		objc_msgSend(pool, sel_getUid("drain"));
	}
}

/**
 * For a composition in the current process, briefly performs work that requires the main thread (such as Cocoa event handling).
 * Repeated calls to this function allow the composition to run without taking over the main thread like runOnMainThread() does.
 *
 * Start calling this function after start() or startPaused().
 *
 * There's no need to call this function if your application calls @c dispatch_main(), @c NSApplicationMain(),
 * or @c UIApplicationMain(), or if it invokes a @c CFRunLoop on the main thread.
 *
 * Throws a @c std::logic_error if this runner was not constructed to run the composition in the current process
 * or if this function was not called on the main thread.
 *
 * \eg{
 *   VuoRunner *runner = newCurrentProcessRunnerFromDynamicLibrary(...);
 *   runner->start();
 *   while (! runner->isStopped())
 *   {
 *      runner->drainMainDispatchQueue();
 *		// do other work on the main thread
 *   }
 * }
 *
 * @only64Bit
 *
 * @see runOnMainThread(), an alternative to this function.
 */
void VuoRunner::drainMainDispatchQueue(void)
{
	if (! isInCurrentProcess())
		throw std::logic_error("The composition is not running in the current process. Only use this function if the composition was constructed with newCurrentProcessRunnerFromDynamicLibrary().");

	if (! isMainThread())
		throw std::logic_error("This is not the main thread. Only call this function from the main thread.");

	id pool = objc_msgSend((id)objc_getClass("NSAutoreleasePool"), sel_getUid("new"));
	CFRunLoopRunInMode(kCFRunLoopDefaultMode,0,false);
	objc_msgSend(pool, sel_getUid("drain"));
}

/**
 * Sends a control request to the composition telling it to cease firing events.
 *
 * Upon return, no more events will be fired and all events will have finished propagating through the composition.
 *
 * Assumes the composition has been started, is not paused, and has not been stopped.
 */
void VuoRunner::pause(void)
{
	dispatch_sync(controlQueue, ^{
					  if (stopped) {
						  return;
					  }

					  vuoMemoryBarrier();

					  vuoControlRequestSend(VuoControlRequestCompositionPause,NULL,0);
					  vuoControlReplyReceive(VuoControlReplyCompositionPaused);
				  });
}

/**
 * Sends a control request to the composition telling it to resume firing events.
 *
 * Assumes the composition is paused.
 */
void VuoRunner::unpause(void)
{
	dispatch_sync(controlQueue, ^{
					  if (stopped) {
						  return;
					  }

					  vuoMemoryBarrier();

					  vuoControlRequestSend(VuoControlRequestCompositionUnpause,NULL,0);
					  vuoControlReplyReceive(VuoControlReplyCompositionUnpaused);
				  });
}

/**
 * Sends a control request to the composition loader telling it to load an updated version
 * of the running composition.
 *
 * Upon return, the old version of the composition will have stopped and the updated version
 * will have started.
 *
 * Assumes the composition loader has been started and has not been stopped.
 *
 * @param compositionDylibPath A linked composition dynamic library, produced by VuoCompiler::linkCompositionToCreateDynamicLibrary().
 * @param resourceDylibPath A linked resource dynamic library, produced by a call to VuoCompiler::linkCompositionToCreateDynamicLibraries() for this runner.
 *				Pass an empty string if no linked resource dynamic library was created.
 * @param compositionDiff A comparison of the old and new compositions, produced by VuoCompilerComposition::diffAgainstOlderComposition().
 */
void VuoRunner::replaceComposition(string compositionDylibPath, string resourceDylibPath, string compositionDiff)
{
	if (! isUsingCompositionLoader())
		throw std::logic_error("The runner is not using a composition loader. Only use this function if the composition was constructed with newSeparateProcessRunnerFromDynamicLibrary().");

	dispatch_sync(controlQueue, ^{
					  if (stopped) {
						  return;
					  }

					  if (dylibPath != compositionDylibPath)
					  {
						  if (shouldDeleteBinariesWhenFinished)
						  {
							  remove(dylibPath.c_str());
						  }

						  dylibPath = compositionDylibPath;
					  }

					  vuoMemoryBarrier();

					  cleanUpConnections();

					  zmq_msg_t messages[3];
					  vuoInitMessageWithString(&messages[0], dylibPath.c_str());
					  vuoInitMessageWithString(&messages[1], resourceDylibPath.c_str());
					  vuoInitMessageWithString(&messages[2], compositionDiff.c_str());

					  vuoLoaderControlRequestSend(VuoLoaderControlRequestCompositionReplace,messages,3);
					  vuoLoaderControlReplyReceive(VuoLoaderControlReplyCompositionReplaced);

					  setUpConnections();
				  });
}

/**
 * Sends a control request to the composition telling it to stop.
 *
 * Upon return, the composition will have stopped. If the composition was running in a separate process,
 * that process will have ended.
 *
 * If the composition has already stopped on its own, this function skips sending the control request.
 * It just performs some cleanup.
 *
 * This function waits for any pending VuoRunnerDelegate function calls to return.
 *
 * Assumes the composition has been started and has not been stopped.
 */
void VuoRunner::stop(void)
{
	dispatch_sync(controlQueue, ^{
					  if (stopped) {
						  return;
					  }

					  vuoMemoryBarrier();

					  // Only tell the composition to stop if it hasn't already ended on its own.
					  if (! listenCanceled)
					  {
						  int timeoutInSeconds = (isInCurrentProcess() ? -1 : 5);
						  zmq_msg_t messages[1];
						  vuoInitMessageWithInt(&messages[0], timeoutInSeconds);
						  vuoControlRequestSend(VuoControlRequestCompositionStop,messages,1);
						  vuoControlReplyReceive(VuoControlReplyCompositionStopping);
					  }

					  cleanUpConnections();

					  if (isUsingCompositionLoader())
					  {
						  zmq_close(ZMQLoaderControl);
						  ZMQLoaderControl = NULL;
					  }

					  if (isInCurrentProcess())
					  {
						  typedef void(* finiType)(void);

						  finiType vuoFini = (finiType)dlsym(dylibHandle, "vuoFini");
						  if (! vuoFini)
						  {
							  VLog("Error: Couldn't find function vuoFini(): %s", dlerror());
							  return;
						  }
						  vuoFini();

						  dlclose(dylibHandle);
						  dylibHandle = NULL;

						  chdir(originalWorkingDir.c_str());
					  }
					  else
					  {
						  // Wait for child process to end.
						  int status;
						  waitpid(compositionPid, &status, 0);

						  zmq_term(ZMQContext);
					  }

					  if (shouldDeleteBinariesWhenFinished)
					  {
						  if (isUsingCompositionLoader())
						  {
							  remove(dylibPath.c_str());
							  for (vector<string>::iterator i = resourceDylibPaths.begin(); i != resourceDylibPaths.end(); ++i)
								  remove((*i).c_str());
						  }
						  else if (isInCurrentProcess())
						  {
							  remove(dylibPath.c_str());
						  }
						  else
						  {
							  remove(executablePath.c_str());
						  }
					  }

					  stopped = true;
					  dispatch_semaphore_signal(stoppedSemaphore);
				  });
}

/**
 * Closes ZMQ connections to the running composition.
 */
void VuoRunner::cleanUpConnections(void)
{
	if (! ZMQControl)
		return;

	zmq_close(ZMQControl);
	ZMQControl = NULL;

	listenCanceled = true;
	dispatch_semaphore_wait(endedListeningSemaphore, DISPATCH_TIME_FOREVER);
	dispatch_semaphore_signal(endedListeningSemaphore);
}

/**
 * Waits until the composition is stopped.
 *
 * If the composition is already stopped, this function returns immediately.
 */
void VuoRunner::waitUntilStopped(void)
{
	dispatch_retain(stoppedSemaphore);
	dispatch_semaphore_wait(stoppedSemaphore, DISPATCH_TIME_FOREVER);
	dispatch_semaphore_signal(stoppedSemaphore);
	dispatch_release(stoppedSemaphore);
}

/**
 * Sends a control request to the composition telling it to modify an input port's value.
 *
 * Upon return, the input port value will have been modified.
 *
 * Assumes the composition has been started and has not been stopped.
 *
 * @param portIdentifier The compile-time identifier for the port (see VuoCompilerEventPort::getIdentifier()).
 * @param value JSON representation of the port's new value.
 *
 * @see VuoTypes for information about types and their JSON representations.
 */
void VuoRunner::setInputPortValue(string portIdentifier, json_object *value)
{
	const char *valueAsString = json_object_to_json_string_ext(value, JSON_C_TO_STRING_PLAIN);

	dispatch_sync(controlQueue, ^{
					  if (stopped) {
						  return;
					  }

					  vuoMemoryBarrier();

					  zmq_msg_t messages[2];
					  vuoInitMessageWithString(&messages[0], portIdentifier.c_str());
					  vuoInitMessageWithString(&messages[1], valueAsString);
					  vuoControlRequestSend(VuoControlRequestInputPortValueModify, messages, 2);
					  vuoControlReplyReceive(VuoControlReplyInputPortValueModified);
				  });
}

/**
 * Sends a control request to the composition telling it to fire an event from the trigger port.
 * If the trigger port carries data, its most recent data is fired along with the event.
 *
 * Upon return, the event will have been fired.
 *
 * Assumes the composition has been started and has not been stopped.
 *
 * @param portIdentifier The compile-time identifier for the port (see VuoCompilerEventPort::getIdentifier()).
 */
void VuoRunner::fireTriggerPortEvent(string portIdentifier)
{
	dispatch_sync(controlQueue, ^{
					  if (stopped) {
						  return;
					  }

					  vuoMemoryBarrier();

					  zmq_msg_t messages[1];
					  vuoInitMessageWithString(&messages[0], portIdentifier.c_str());
					  vuoControlRequestSend(VuoControlRequestTriggerPortFireEvent, messages, 1);
					  vuoControlReplyReceive(VuoControlReplyTriggerPortFiredEvent);
				  });
}

/**
 * Sends a control request to the composition telling it to retrieve an input port's value.
 *
 * Assumes the composition has been started and has not been stopped.
 *
 * @param portIdentifier The compile-time identifier for the port (see VuoCompilerEventPort::getIdentifier()).
 * @return JSON representation of the port's value.
 *
 * @see VuoTypes for information about types and their JSON representations.
 */
json_object * VuoRunner::getInputPortValue(string portIdentifier)
{
	__block string valueAsString;
	dispatch_sync(controlQueue, ^{
					  if (stopped) {
						  return;
					  }

					  vuoMemoryBarrier();

					  zmq_msg_t messages[2];
					  vuoInitMessageWithBool(&messages[0], !isInCurrentProcess());
					  vuoInitMessageWithString(&messages[1], portIdentifier.c_str());
					  vuoControlRequestSend(VuoControlRequestInputPortValueRetrieve, messages, 2);
					  vuoControlReplyReceive(VuoControlReplyInputPortValueRetrieved);
					  char *s = vuoReceiveAndCopyString(ZMQControl);
					  if (s)
					  {
						  valueAsString = s;
						  free(s);
					  }
					  else
						  valueAsString = "null";
				  });
	return json_tokener_parse(valueAsString.c_str());
}

/**
 * Sends a control request to the composition telling it to retrieve an output port's value.
 *
 * Assumes the composition has been started and has not been stopped.
 *
 * @param portIdentifier The compile-time identifier for the port (see VuoCompilerEventPort::getIdentifier()).
 * @return JSON representation of the port's value.
 *
 * @see VuoTypes for information about types and their JSON representations.
 */
json_object * VuoRunner::getOutputPortValue(string portIdentifier)
{
	__block string valueAsString;
	dispatch_sync(controlQueue, ^{
					  if (stopped) {
						  return;
					  }

					  vuoMemoryBarrier();

					  zmq_msg_t messages[2];
					  vuoInitMessageWithBool(&messages[0], !isInCurrentProcess());
					  vuoInitMessageWithString(&messages[1], portIdentifier.c_str());
					  vuoControlRequestSend(VuoControlRequestOutputPortValueRetrieve, messages, 2);
					  vuoControlReplyReceive(VuoControlReplyOutputPortValueRetrieved);
					  char *s = vuoReceiveAndCopyString(ZMQControl);
					  if (s)
					  {
						  valueAsString = s;
						  free(s);
					  }
					  else
						  valueAsString = "null";
				  });
	return json_tokener_parse(valueAsString.c_str());
}

/**
 * Sends a control request to the composition telling it to retrieve an input port's summary.
 *
 * Assumes the composition has been started and has not been stopped.
 *
 * @param portIdentifier The compile-time identifier for the port (see VuoCompilerEventPort::getIdentifier()).
 * @return A brief description of the port's value.
 *
 * @see VuoTypes for information about types and their summaries.
 */
string VuoRunner::getInputPortSummary(string portIdentifier)
{
	__block string summary;
	dispatch_sync(controlQueue, ^{
					  if (stopped) {
						  return;
					  }

					  vuoMemoryBarrier();

					  zmq_msg_t messages[1];
					  vuoInitMessageWithString(&messages[0], portIdentifier.c_str());
					  vuoControlRequestSend(VuoControlRequestInputPortSummaryRetrieve, messages, 1);
					  vuoControlReplyReceive(VuoControlReplyInputPortSummaryRetrieved);
					  char *s = vuoReceiveAndCopyString(ZMQControl);
					  if (s)
					  {
						  summary = s;
						  free(s);
					  }
					  else
						  summary = "";
				  });
	return summary;
}

/**
 * Sends a control request to the composition telling it to retrieve an output port's summary.
 *
 * Assumes the composition has been started and has not been stopped.
 *
 * @param portIdentifier The compile-time identifier for the port (see VuoCompilerEventPort::getIdentifier()).
 * @return A brief description of the port's value.
 *
 * @see VuoTypes for information about types and their summaries.
 */
string VuoRunner::getOutputPortSummary(string portIdentifier)
{
	__block string summary;
	dispatch_sync(controlQueue, ^{
					  if (stopped) {
						  return;
					  }

					  vuoMemoryBarrier();

					  zmq_msg_t messages[1];
					  vuoInitMessageWithString(&messages[0], portIdentifier.c_str());
					  vuoControlRequestSend(VuoControlRequestOutputPortSummaryRetrieve, messages, 1);
					  vuoControlReplyReceive(VuoControlReplyOutputPortSummaryRetrieved);
					  char *s = vuoReceiveAndCopyString(ZMQControl);
					  if (s)
					  {
						  summary = s;
						  free(s);
					  }
					  else
						  summary = "";
				  });
	return summary;
}

/**
 * Sends a control request to the composition telling it to modify a published input port's value.
 *
 * Upon return, the published input port value will have been modified.
 *
 * Assumes the composition has been started and has not been stopped.
 *
 * @param port The published input port.
 * @param value JSON representation of the port's new value.
 *
 * @see VuoTypes for information about types and their JSON representations.
 */
void VuoRunner::setPublishedInputPortValue(VuoRunner::Port *port, json_object *value)
{
	const char *valueAsString = json_object_to_json_string_ext(value, JSON_C_TO_STRING_PLAIN);

	dispatch_sync(controlQueue, ^{
					  if (stopped) {
						  return;
					  }

					  vuoMemoryBarrier();

					  zmq_msg_t messages[2];
					  vuoInitMessageWithString(&messages[0], port->getName().c_str());
					  vuoInitMessageWithString(&messages[1], valueAsString);
					  vuoControlRequestSend(VuoControlRequestPublishedInputPortValueModify, messages, 2);
					  vuoControlReplyReceive(VuoControlReplyPublishedInputPortValueModified);
				  });
}

/**
 * Sends a control request to the composition telling it to fire an event into a published input port.
 *
 * Upon return, the event will have been fired.
 *
 * Assumes the composition has been started, is not paused, and has not been stopped.
 */
void VuoRunner::firePublishedInputPortEvent(VuoRunner::Port *port)
{
	dispatch_sync(controlQueue, ^{
					  if (stopped) {
						  return;
					  }

					  vuoMemoryBarrier();

					  anyPublishedPortEventSignaled = false;

					  zmq_msg_t messages[1];
					  vuoInitMessageWithString(&messages[0], port->getName().c_str());
					  vuoControlRequestSend(VuoControlRequestPublishedInputPortFireEvent, messages, 1);
					  vuoControlReplyReceive(VuoControlReplyPublishedInputPortFiredEvent);
				  });
}

/**
 * Sends a control request to the composition telling it to fire an event into all published input ports.
 * This is a single event that goes to all published input ports simultaneously.
 *
 * Upon return, the event will have been fired.
 *
 * Assumes the composition has been started, is not paused, and has not been stopped.
 */
void VuoRunner::firePublishedInputPortEvent(void)
{
	Port port(VuoNodeClass::publishedInputNodeSimultaneousTriggerName, "", NULL);
	firePublishedInputPortEvent(&port);
}

/**
 * Waits until the first event following a call to firePublishedInputPortEvent() comes out of
 * any published output port.
 *
 * The event that comes out may not be the same as the one fired by firePublishedInputPortEvent()
 * if the composition contains trigger ports. The event also may not be finished traveling through the
 * composition if the composition contains a branch where the event travels through multiple parts of
 * the composition concurrently.
 *
 * Assumes the composition has been started and has not been stopped.
 *
 * @eg{
 *   runner.setPublishedInputPortValue(inputPort, inputValue);
 *   runner.setPublishedInputPortValue(anotherInputPort, anotherInputValue);
 *   runner.firePublishedInputPortEvent(inputPort);
 *   runner.waitForAnyPublishedOutputPortEvent();
 *   result = runner.getPublishedOutputPortValue(outputPort);
 * }
 */
void VuoRunner::waitForAnyPublishedOutputPortEvent(void)
{
	saturating_semaphore_wait(anyPublishedPortEventSemaphore, &anyPublishedPortEventSignaled);
}

/**
 * Sends a control request to the composition telling it to retrieve a published input port's value.
 *
 * Assumes the composition has been started and has not been stopped.
 *
 * @param port The published input port.
 * @return JSON representation of the port's value.
 *
 * @see VuoTypes for information about types and their JSON representations.
 */
json_object * VuoRunner::getPublishedInputPortValue(VuoRunner::Port *port)
{
	__block string valueAsString;
	dispatch_sync(controlQueue, ^{
					  if (stopped) {
						  return;
					  }

					  vuoMemoryBarrier();

					  zmq_msg_t messages[2];
					  vuoInitMessageWithBool(&messages[0], !isInCurrentProcess());
					  vuoInitMessageWithString(&messages[1], port->getName().c_str());
					  vuoControlRequestSend(VuoControlRequestPublishedInputPortValueRetrieve, messages, 2);
					  vuoControlReplyReceive(VuoControlReplyPublishedInputPortValueRetrieved);
					  char *s = vuoReceiveAndCopyString(ZMQControl);
					  if (s)
					  {
						  valueAsString = s;
						  free(s);
					  }
					  else
						  valueAsString = "null";
				  });
	return json_tokener_parse(valueAsString.c_str());
}

/**
 * Sends a control request to the composition telling it to retrieve a published output port's value.
 *
 * Assumes the composition has been started and has not been stopped.
 *
 * @param port The published output port.
 * @return JSON representation of the port's value.
 *
 * @see VuoTypes for information about types and their JSON representations.
 */
json_object * VuoRunner::getPublishedOutputPortValue(VuoRunner::Port *port)
{
	__block string valueAsString;
	dispatch_sync(controlQueue, ^{
					  if (stopped) {
						  return;
					  }

					  vuoMemoryBarrier();

					  zmq_msg_t messages[2];
					  vuoInitMessageWithBool(&messages[0], !isInCurrentProcess());
					  vuoInitMessageWithString(&messages[1], port->getName().c_str());
					  vuoControlRequestSend(VuoControlRequestPublishedOutputPortValueRetrieve, messages, 2);
					  vuoControlReplyReceive(VuoControlReplyPublishedOutputPortValueRetrieved);
					  char *s = vuoReceiveAndCopyString(ZMQControl);
					  if (s)
					  {
						  valueAsString = s;
						  free(s);
					  }
					  else
						  valueAsString = "null";
				  });
	return json_tokener_parse(valueAsString.c_str());
}

/**
 * Returns the list of published input or output ports for the running composition.
 *
 * The first time this function is called for input ports, and the first time it's called for output ports,
 * it sends a control request to the composition. On subsequent calls, it returns the list of ports cached
 * from the first call.
 *
 * Assumes the composition has been started and has not been stopped.
 *
 * If a control request is sent, this function must be synchronized with other control requests
 * (i.e. called from @ref start(bool) or @ref controlQueue).
 */
vector<VuoRunner::Port *> VuoRunner::getCachedPublishedPorts(bool input)
{
	// Caching not only provides faster access (without zmq messages),
	// but also ensures that the VuoRunner::Port pointers passed to
	// VuoRunnerDelegate::receivedTelemetryPublishedOutputPortUpdated are consistent.

	if (input)
	{
		if (! arePublishedInputPortsCached)
		{
			publishedInputPorts = refreshPublishedPorts(true);
			arePublishedInputPortsCached = true;
		}
		return publishedInputPorts;
	}
	else
	{
		if (! arePublishedOutputPortsCached)
		{
			publishedOutputPorts = refreshPublishedPorts(false);
			arePublishedOutputPortsCached = true;
		}
		return publishedOutputPorts;
	}
}

/**
 * Sends a control request to the composition telling it to retrieve a list of published input or output ports.
 *
 * Assumes the composition has been started and has not been stopped.
 *
 * This function must be synchronized with other control requests
 * (i.e. called from @ref start(bool) or @ref controlQueue).
 */
vector<VuoRunner::Port *> VuoRunner::refreshPublishedPorts(bool input)
{
	vuoMemoryBarrier();

	enum VuoControlRequest requests[4];
	enum VuoControlReply replies[4];
	if (input)
	{
		requests[0] = VuoControlRequestPublishedInputPortNamesRetrieve;
		requests[1] = VuoControlRequestPublishedInputPortTypesRetrieve;
		requests[2] = VuoControlRequestPublishedInputPortDetailsRetrieve;
		requests[3] = VuoControlRequestPublishedInputPortConnectedIdentifiersRetrieve;
		replies[0] = VuoControlReplyPublishedInputPortNamesRetrieved;
		replies[1] = VuoControlReplyPublishedInputPortTypesRetrieved;
		replies[2] = VuoControlReplyPublishedInputPortDetailsRetrieved;
		replies[3] = VuoControlReplyPublishedInputPortConnectedIdentifiersRetrieved;
	}
	else
	{
		requests[0] = VuoControlRequestPublishedOutputPortNamesRetrieve;
		requests[1] = VuoControlRequestPublishedOutputPortTypesRetrieve;
		requests[2] = VuoControlRequestPublishedOutputPortDetailsRetrieve;
		requests[3] = VuoControlRequestPublishedOutputPortConnectedIdentifiersRetrieve;
		replies[0] = VuoControlReplyPublishedOutputPortNamesRetrieved;
		replies[1] = VuoControlReplyPublishedOutputPortTypesRetrieved;
		replies[2] = VuoControlReplyPublishedOutputPortDetailsRetrieved;
		replies[3] = VuoControlReplyPublishedOutputPortConnectedIdentifiersRetrieved;
	}

	vector<string> names;
	vector<string> types;
	vector<string> details;

	for (int i = 0; i < 3; ++i)
	{
		vuoControlRequestSend(requests[i], NULL, 0);
		vuoControlReplyReceive(replies[i]);
		vector<string> messageStrings = receiveListOfStrings(ZMQControl);
		if (i == 0)
			names = messageStrings;
		else if (i == 1)
			types = messageStrings;
		else
			details = messageStrings;
	}

	vector<VuoRunner::Port *> ports;
	for (size_t i = 0; i < names.size() && i < types.size(); ++i)
	{
		zmq_msg_t messages[1];
		vuoInitMessageWithString(&messages[0], names[i].c_str());
		vuoControlRequestSend(requests[3], messages, 1);
		vuoControlReplyReceive(replies[3]);
		vector<string> messageStrings = receiveListOfStrings(ZMQControl);
		set<string> identifiers( messageStrings.begin(), messageStrings.end() );

		VuoRunner::Port *port = new Port(names[i], types[i], json_tokener_parse(details[i].c_str()));
		port->setConnectedPortIdentifiers(identifiers);
		ports.push_back(port);
	}

	return ports;
}

/**
 * Returns the list of published input ports in the composition.
 *
 * This function may either send a control request to the composition or use cached values.
 *
 * Assumes the composition has been started and has not been stopped.
 */
vector<VuoRunner::Port *> VuoRunner::getPublishedInputPorts(void)
{
	return getCachedPublishedPorts(true);
}

/**
 * Returns the list of published output ports in the composition.
 *
 * This function may either send a control request to the composition or use cached values.
 *
 * Assumes the composition has been started and has not been stopped.
 */
vector<VuoRunner::Port *> VuoRunner::getPublishedOutputPorts(void)
{
	return getCachedPublishedPorts(false);
}

/**
 * Returns the published input port with the given name, or NULL if no such port exists.
 *
 * This function may either send a control request to the composition or use cached values.
 *
 * Assumes the composition has been started and has not been stopped.
 */
VuoRunner::Port * VuoRunner::getPublishedInputPortWithName(string name)
{
	vector<VuoRunner::Port *> inputPorts = getPublishedInputPorts();
	for (vector<VuoRunner::Port *>::iterator i = inputPorts.begin(); i != inputPorts.end(); ++i)
		if ((*i)->getName() == name)
			return *i;

	return NULL;
}

/**
 * Returns the published output port with the given name, or NULL if no such port exists.
 *
 * This function may either send a control request to the composition or use cached values.
 *
 * Assumes the composition has been started and has not been stopped.
 */
VuoRunner::Port * VuoRunner::getPublishedOutputPortWithName(string name)
{
	vector<VuoRunner::Port *> outputPorts = getPublishedOutputPorts();
	for (vector<VuoRunner::Port *>::iterator i = outputPorts.begin(); i != outputPorts.end(); ++i)
		if ((*i)->getName() == name)
			return *i;

	return NULL;
}

/**
 * Listens for telemetry data from the composition until the composition stops.
 *
 * Assumes the composition has been started and has not been stopped.
 */
void VuoRunner::listen(void)
{
	{
		ZMQTelemetry = zmq_socket(ZMQContext,ZMQ_SUB);
		VuoRunner_configureSocket(ZMQTelemetry);
		if(zmq_connect(ZMQTelemetry,ZMQTelemetryURL.c_str()))
			VLog("Error: VuoTelemetry connect failed.");
	}

	{
		// subscribe to all types of telemetry
		char type = VuoTelemetryStats;
		zmq_setsockopt(ZMQTelemetry, ZMQ_SUBSCRIBE, &type, sizeof type);
		type = VuoTelemetryNodeExecutionStarted;
		zmq_setsockopt(ZMQTelemetry, ZMQ_SUBSCRIBE, &type, sizeof type);
		type = VuoTelemetryNodeExecutionFinished;
		zmq_setsockopt(ZMQTelemetry, ZMQ_SUBSCRIBE, &type, sizeof type);
		type = VuoTelemetryInputPortsUpdated;
		zmq_setsockopt(ZMQTelemetry, ZMQ_SUBSCRIBE, &type, sizeof type);
		type = VuoTelemetryOutputPortsUpdated;
		zmq_setsockopt(ZMQTelemetry, ZMQ_SUBSCRIBE, &type, sizeof type);
		type = VuoTelemetryEventDropped;
		zmq_setsockopt(ZMQTelemetry, ZMQ_SUBSCRIBE, &type, sizeof type);
		type = VuoTelemetryError;
		zmq_setsockopt(ZMQTelemetry, ZMQ_SUBSCRIBE, &type, sizeof type);
		type = VuoTelemetryStopRequested;
		zmq_setsockopt(ZMQTelemetry, ZMQ_SUBSCRIBE, &type, sizeof type);
	}

	// Wait for the first VuoTelemetryStats message to arrive, then confirm that we're able to receive messages
	// (if we don't wait for a message to arrive, important one-time-only messages such as VuoTelemetryOutputPortsUpdated might be lost).
	// As http://zguide.zeromq.org/page:all says, "the subscriber will always miss the first messages that the publisher sends".
	zmq_pollitem_t items[]=
	{
		{ZMQTelemetry,0,ZMQ_POLLIN,0},
	};
	int itemCount = 1;
	long timeout = -1;
	zmq_poll(items,itemCount,timeout);
	dispatch_semaphore_signal(beganListeningSemaphore);

	while(! listenCanceled)
	{
		long timeout = USEC_PER_SEC;  // wait up to 1 second (can't wait forever, since we need to check listenCanceled)
		zmq_poll(items,itemCount,timeout);
		if(items[0].revents & ZMQ_POLLIN)
		{
			// Receive telemetry type.
			char type = vuoReceiveInt(ZMQTelemetry);

			// Receive telemetry arguments and forward to VuoRunnerDelegate.
			switch (type)
			{
				case VuoTelemetryStats:
				{
					unsigned long utime = vuoReceiveUnsignedInt64(ZMQTelemetry);
					unsigned long stime = vuoReceiveUnsignedInt64(ZMQTelemetry);
					dispatch_sync(delegateQueue, ^{
									  if (delegate)
										  delegate->receivedTelemetryStats(utime, stime);
								  });
					break;
				}
				case VuoTelemetryNodeExecutionStarted:
				{
					char *nodeIdentifier = vuoReceiveAndCopyString(ZMQTelemetry);
					dispatch_sync(delegateQueue, ^{
									  if (delegate)
										  delegate->receivedTelemetryNodeExecutionStarted( string(nodeIdentifier) );
								  });
					free(nodeIdentifier);
					break;
				}
				case VuoTelemetryNodeExecutionFinished:
				{
					char *nodeIdentifier = vuoReceiveAndCopyString(ZMQTelemetry);
					dispatch_sync(delegateQueue, ^{
									  if (delegate)
										  delegate->receivedTelemetryNodeExecutionFinished( string(nodeIdentifier) );
								  });
					free(nodeIdentifier);
					break;
				}
				case VuoTelemetryInputPortsUpdated:
				{
					while (hasMoreToReceive(ZMQTelemetry))
					{
						char *portIdentifier = vuoReceiveAndCopyString(ZMQTelemetry);
						if (hasMoreToReceive(ZMQTelemetry))
						{
							bool receivedEvent = vuoReceiveBool(ZMQTelemetry);
							if (hasMoreToReceive(ZMQTelemetry))
							{
								bool receivedData = vuoReceiveBool(ZMQTelemetry);
								if (hasMoreToReceive(ZMQTelemetry))
								{
									string portDataSummary;
									char *s = vuoReceiveAndCopyString(ZMQTelemetry);
									if (s)
									{
										portDataSummary = s;
										free(s);
									}
									else
										portDataSummary = "";

									dispatch_sync(delegateQueue, ^{
													  if (delegate)
														  delegate->receivedTelemetryInputPortUpdated(portIdentifier, receivedEvent, receivedData, portDataSummary);
												  });
								}
							}
						}
						free(portIdentifier);
					}
					break;
				}
				case VuoTelemetryOutputPortsUpdated:
				{
					while (hasMoreToReceive(ZMQTelemetry))
					{
						char *portIdentifier = vuoReceiveAndCopyString(ZMQTelemetry);
						if (hasMoreToReceive(ZMQTelemetry))
						{
							bool sentData = vuoReceiveBool(ZMQTelemetry);
							if (hasMoreToReceive(ZMQTelemetry))
							{
								string portDataSummary;
								char *s = vuoReceiveAndCopyString(ZMQTelemetry);
								if (s)
								{
									portDataSummary = s;
									free(s);
								}
								else
									portDataSummary = "";

								dispatch_sync(delegateQueue, ^{
												  if (delegate)
													  delegate->receivedTelemetryOutputPortUpdated(portIdentifier, sentData, portDataSummary);
											  });

								if (sentData)
								{
									for (vector<VuoRunner::Port *>::iterator i = publishedOutputPorts.begin(); i != publishedOutputPorts.end(); ++i)
									{
										VuoRunner::Port *port = *i;
										set<string> connectedIdentifiers = port->getConnectedPortIdentifiers();
										if (connectedIdentifiers.find(portIdentifier) != connectedIdentifiers.end())
										{
											dispatch_sync(delegateQueue, ^{
															  if (delegate)
															  delegate->receivedTelemetryPublishedOutputPortUpdated(port, sentData, portDataSummary);
														  });

											saturating_semaphore_signal(anyPublishedPortEventSemaphore, &anyPublishedPortEventSignaled);
										}
									}
								}
							}
						}
						free(portIdentifier);
					}
					break;
				}
				case VuoTelemetryEventDropped:
				{
					char *portIdentifier = vuoReceiveAndCopyString(ZMQTelemetry);
					dispatch_sync(delegateQueue, ^{
									  if (delegate)
										  delegate->receivedTelemetryEventDropped(portIdentifier);
								  });
					free(portIdentifier);
					break;
				}
				case VuoTelemetryError:
				{
					char *message = vuoReceiveAndCopyString(ZMQTelemetry);
					dispatch_sync(delegateQueue, ^{
									  if (delegate)
										  delegate->receivedTelemetryError( string(message) );
								  });
					free(message);
					break;
				}
				case VuoTelemetryStopRequested:
				{
					dispatch_sync(delegateQueue, ^{
									   if (delegate)
										   delegate->lostContactWithComposition();
								   });
					dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
									   stop();
								   });
					break;
				}
			}
		}
		else if (! listenCanceled)
		{
			listenCanceled = true;
			dispatch_sync(delegateQueue, ^{
							   if (delegate)
								   delegate->lostContactWithComposition();
						   });
			dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
							   stop();
						   });
		}
	}

	zmq_close(ZMQTelemetry);
	ZMQTelemetry = NULL;

	dispatch_semaphore_signal(endedListeningSemaphore);
}

/**
 * Sends a control request to the composition via the control socket.
 *
 * Assumes the composition has been started and has not been stopped.
 *
 * @param request The type of control request.
 * @param messages Arguments passed with the control request.
 * @param messageCount The number of arguments.
 */
void VuoRunner::vuoControlRequestSend(enum VuoControlRequest request, zmq_msg_t *messages, unsigned int messageCount)
{
	vuoSend("runner VuoControl",ZMQControl,request,messages,messageCount,false);
}

/**
 * Sends a control request to the composition loader via the composition loader's control socket.
 *
 * Assumes the composition has been started and has not been stopped.
 *
 * @param request The type of control request.
 * @param messages Arguments passed with the control request.
 * @param messageCount The number of arguments.
 */
void VuoRunner::vuoLoaderControlRequestSend(enum VuoLoaderControlRequest request, zmq_msg_t *messages, unsigned int messageCount)
{
	vuoSend("runner VuoLoaderControl",ZMQLoaderControl,request,messages,messageCount,false);
}

/**
 * Receives a control reply from the composition via the control socket, and checks that it
 * is the expected reply.
 *
 * Assumes the reply is the next message to be received on the control socket.
 *
 * @param expectedReply The expected reply value.
 */
void VuoRunner::vuoControlReplyReceive(enum VuoControlReply expectedReply)
{
	int reply = vuoReceiveInt(ZMQControl);
	if (reply != expectedReply)
		VLog("Error: VuoControl message received unexpected reply (received %d, expected %d).", reply, expectedReply);
}

/**
 * Receives a control reply from the composition loader via the composition loader's control socket,
 * and checks that it is the expected reply.
 *
 * Assumes the reply is the next message to be received on the composition loader's control socket.
 *
 * @param expectedReply The expected reply value.
 */
void VuoRunner::vuoLoaderControlReplyReceive(enum VuoLoaderControlReply expectedReply)
{
	int reply = vuoReceiveInt(ZMQLoaderControl);
	if (reply != expectedReply)
		VLog("Error: VuoLoaderControl message received unexpected reply (received %d, expected %d).", reply, expectedReply);
}

/**
 * Returns true if there are more messages to receive on the socket currently.
 */
bool VuoRunner::hasMoreToReceive(void *socket)
{
	int64_t more;
	size_t moreSize = sizeof more;
	zmq_getsockopt(socket, ZMQ_RCVMORE, &more, &moreSize);
	return more;
}

/**
 * Receives a list of 0 or more strings from the composition.
 *
 * @return The list of strings.
 */
vector<string> VuoRunner::receiveListOfStrings(void *socket)
{
	vector<string> messageStrings;
	while (hasMoreToReceive(socket))
	{
		char *s = vuoReceiveAndCopyString(socket);
		messageStrings.push_back(string(s));
		free(s);
	}
	return messageStrings;
}

/**
 * Signals the semaphore only if it's being waited on. Use with saturating_semaphore_wait().
 *
 * Copied from https://developer.apple.com/library/mac/#documentation/Darwin/Reference/ManPages/man3/dispatch_semaphore_create.3.html
 */
void VuoRunner::saturating_semaphore_signal(dispatch_semaphore_t dsema, bool *signaled)
{
	if (__sync_bool_compare_and_swap(signaled, false, true)) {
		dispatch_semaphore_signal(dsema);
	}
}

/**
 * Waits on the semaphore. Use with saturating_semaphore_signal().
 *
 * Copied from https://developer.apple.com/library/mac/#documentation/Darwin/Reference/ManPages/man3/dispatch_semaphore_create.3.html
 */
void VuoRunner::saturating_semaphore_wait(dispatch_semaphore_t dsema, bool *signaled)
{
	*signaled = false;
	dispatch_semaphore_wait(dsema, DISPATCH_TIME_FOREVER);
}

/**
 * Returns true if the composition either has not been started or has been started and stopped.
 */
bool VuoRunner::isStopped(void)
{
	return stopped;
}

/**
 * Returns true if the composition was constructed to run in the current process (even if the composition is not running right now).
 */
bool VuoRunner::isInCurrentProcess(void)
{
	return executablePath.empty();
}

/**
 * Returns true if the composition was constructed to run in a separate process with a composition loader
 * (even if the composition is not running right now).
 */
bool VuoRunner::isUsingCompositionLoader(void)
{
	return ! executablePath.empty() && ! dylibPath.empty();
}

/**
 * Sets the delegate that receives telemetry messages from the running composition. May be null.
 */
void VuoRunner::setDelegate(VuoRunnerDelegate *delegate)
{
	dispatch_sync(delegateQueue, ^{
					  this->delegate = delegate;
				  });
}

/**
 * Creates a dummy published port that is not yet connected to any port in a running composition.
 *
 * @param name The published port's name.
 * @param type The published port's data type name, or an empty string if the port is event-only.
 * @param details The published port's details (see @ref VuoInputData).
 */
VuoRunner::Port::Port(string name, string type, json_object *details)
{
	this->name = name;
	this->type = type;
	this->details = details;
}

/**
 * Returns the published port's name.
 */
string VuoRunner::Port::getName(void)
{
	return name;
}

/**
 * Returns the published port's type name, or an empty string if the port is event-only.
 */
string VuoRunner::Port::getType(void)
{
	return type;
}

/**
 * Returns the published port's details.
 */
json_object *VuoRunner::Port::getDetails(void)
{
	return details;
}

/**
 * Makes this published port an alias for the ports with the given identifiers.
 */
void VuoRunner::Port::setConnectedPortIdentifiers(set<string> connectedPortIdentifiers)
{
	this->connectedPortIdentifiers = connectedPortIdentifiers;
}

/**
 * Returns the identifiers of the ports for which this published port is an alias.
 */
set<string> VuoRunner::Port::getConnectedPortIdentifiers(void)
{
	return connectedPortIdentifiers;
}

VuoRunnerDelegate::~VuoRunnerDelegate() { }  // Fixes "undefined symbols" error

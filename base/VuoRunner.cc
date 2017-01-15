/**
 * @file
 * VuoRunner implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoNodeClass.hh"
#include "VuoRunner.hh"
#include "VuoFileUtilities.hh"
#include "VuoEventLoop.h"
#include "VuoRuntime.h"

#include <CoreFoundation/CoreFoundation.h>
#include <pthread.h>
#include <stdio.h>
#include <dlfcn.h>
#include <sstream>
#include <objc/runtime.h>
#include <objc/message.h>

void *VuoApp_mainThread = NULL;	///< A reference to the main thread
static int compositionReadRunnerWritePipe[2];  ///< A pipe used by the runtime to check if the runner process has ended.

/**
 * Get a reference to the main thread, so we can perform runtime thread-sanity assertions.
 */
static void __attribute__((constructor)) VuoRunner_init()
{
	VuoApp_mainThread = (void *)pthread_self();
	pipe(compositionReadRunnerWritePipe);
}

/**
 * Is the current thread the main thread?
 */
static bool isMainThread(void)
{
	return VuoApp_mainThread == (void *)pthread_self();
}

/**
 * Applies standard settings to the specified ZMQ socket.
 */
static void VuoRunner_configureSocket(void *zmqSocket, int timeoutInSeconds)
{
	if (timeoutInSeconds >= 0)
	{
		int timeoutInMilliseconds = timeoutInSeconds * 1000;
		zmq_setsockopt(zmqSocket, ZMQ_RCVTIMEO, &timeoutInMilliseconds, sizeof timeoutInMilliseconds);
		zmq_setsockopt(zmqSocket, ZMQ_SNDTIMEO, &timeoutInMilliseconds, sizeof timeoutInMilliseconds);
	}

	int linger = 0;  // avoid having zmq_term block if the runner has tried to send a message on a broken connection
	zmq_setsockopt(zmqSocket, ZMQ_LINGER, &linger, sizeof linger);
}

/**
 * Creates a runner object that can run the composition in file @a compositionPath in a new process.
 *
 * @param compositionPath A composition (.vuo) file.
 * @param continueIfRunnerDies If true, the composition keeps running if the runner process exits without stopping the composition.
 * @param useExistingCache If true, the build is sped up by skipping the step of ensuring that the compiler's cache exists.
 */
VuoRunner * VuoRunner::newSeparateProcessRunnerFromCompositionFile(string compositionPath, bool continueIfRunnerDies, bool useExistingCache)
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
		string optimization = (useExistingCache ? "on" : "off");
		execl(vuoCompilePath.c_str(), "vuo-compile", "--output", compiledCompositionPath.c_str(), compositionPath.c_str(),
			  "--optimization", optimization.c_str(), "--omit-license-warning", NULL);
		VUserLog("Error: Couldn't run '%s'.", vuoCompilePath.c_str());
		_exit(-1);
	}
	else if (compilePid > 0)
	{
		int status;
		int ret;
		do {
			ret = waitpid(compilePid, &status, 0);
		} while (ret == -1 && errno == EINTR);
		if (WIFEXITED(status) && WEXITSTATUS(status))
		{
			VUserLog("Error: Couldn't compile '%s'.", compositionPath.c_str());
			return NULL;
		}
	}
	else
	{
		VUserLog("Error: Couldn't fork '%s'.", vuoCompilePath.c_str());
		return NULL;
	}

	// vuo-link --output /tmp/composition composition.bc
	pid_t linkPid = fork();
	if (linkPid == 0)
	{
		string optimization = (useExistingCache ? "fast-build-existing-cache" : "fast-build");
		execl(vuoLinkPath.c_str(), "vuo-link", "--output", linkedCompositionPath.c_str(), compiledCompositionPath.c_str(),
			  "--optimization", optimization.c_str(), "--omit-license-warning", NULL);
		VUserLog("Error: Couldn't run '%s'.", vuoLinkPath.c_str());
		_exit(-1);
	}
	else if (linkPid > 0)
	{
		int status;
		int ret;
		do {
			ret = waitpid(linkPid, &status, 0);
		} while (ret == -1 && errno == EINTR);
		if (WIFEXITED(status) && WEXITSTATUS(status))
		{
			VUserLog("Error: Couldn't link '%s'.", compositionPath.c_str());
			return NULL;
		}
	}
	else
	{
		VUserLog("Error: Couldn't fork '%s'.", vuoLinkPath.c_str());
		return NULL;
	}

	remove(compiledCompositionPath.c_str());

	return newSeparateProcessRunnerFromExecutable(linkedCompositionPath, compositionDir, continueIfRunnerDies, true);
}

/**
 * Creates a runner object that can run a composition in a new process.
 *
 * @param compositionString A string containing the composition source code.
 * @param name The executable filename that the running composition should use.
 * @param sourceDir The directory nodes should use to resolve relative paths in the composition.
 * @param continueIfRunnerDies If true, the composition keeps running if the runner process exits without stopping the composition.
 * @param useExistingCache If true, the build is sped up by skipping the step of ensuring that the compiler's cache exists.
 */
VuoRunner * VuoRunner::newSeparateProcessRunnerFromCompositionString(string compositionString, string name, string sourceDir,
																	 bool continueIfRunnerDies, bool useExistingCache)
{
	string compiledCompositionPath = VuoFileUtilities::makeTmpFile(name, "bc");

	string compiledCompositionDir, compiledCompositionFile, compiledCompositionExt;
	VuoFileUtilities::splitPath(compiledCompositionPath, compiledCompositionDir, compiledCompositionFile, compiledCompositionExt);
	string linkedCompositionPath = compiledCompositionDir + compiledCompositionFile;

	string vuoFrameworkDir = VuoFileUtilities::getVuoFrameworkPath();
	string vuoCompilePath = vuoFrameworkDir + "/Helpers/vuo-compile";
	string vuoLinkPath = vuoFrameworkDir + "/Helpers/vuo-link";

	int fd[2];
	int ret = pipe(fd);
	if (ret != 0)
	{
		VUserLog("Error: Couldn't open pipe to compiler.");
		return NULL;
	}

	// vuo-compile --output /tmp/composition.bc -
	pid_t compilePid = fork();
	if (compilePid == 0)
	{
		close(fd[1]);

		// Connect the pipe to vuo-compile's STDIN.
		dup2(fd[0], STDIN_FILENO);

		string optimization = (useExistingCache ? "on" : "off");
		execl(vuoCompilePath.c_str(), "vuo-compile", "--output", compiledCompositionPath.c_str(),
			  "--optimization", optimization.c_str(), "--omit-license-warning", "-", NULL);
		VUserLog("Error: Couldn't run '%s'.", vuoCompilePath.c_str());
		_exit(-1);
	}
	else if (compilePid > 0)
	{
		close(fd[0]);

		// Write compositionString to vuo-compile's STDIN.
		write(fd[1], compositionString.c_str(), compositionString.length());
		close(fd[1]);

		int status;
		int ret;
		do {
			ret = waitpid(compilePid, &status, 0);
		} while (ret == -1 && errno == EINTR);
		if (WIFEXITED(status) && WEXITSTATUS(status))
		{
			VUserLog("Error: Couldn't compile '%s'.", name.c_str());
			return NULL;
		}
	}
	else
	{
		VUserLog("Error: Couldn't fork '%s'.", vuoCompilePath.c_str());
		return NULL;
	}

	// vuo-link --output /tmp/composition composition.bc
	pid_t linkPid = fork();
	if (linkPid == 0)
	{
		string optimization = (useExistingCache ? "fast-build-existing-cache" : "fast-build");
		execl(vuoLinkPath.c_str(), "vuo-link", "--output", linkedCompositionPath.c_str(), compiledCompositionPath.c_str(),
			  "--optimization", optimization.c_str(), "--omit-license-warning", NULL);
		VUserLog("Error: Couldn't run '%s'.", vuoLinkPath.c_str());
		_exit(-1);
	}
	else if (linkPid > 0)
	{
		int status;
		int ret;
		do {
			ret = waitpid(linkPid, &status, 0);
		} while (ret == -1 && errno == EINTR);
		if (WIFEXITED(status) && WEXITSTATUS(status))
		{
			VUserLog("Error: Couldn't link '%s'.", name.c_str());
			return NULL;
		}
	}
	else
	{
		VUserLog("Error: Couldn't fork '%s'.", vuoLinkPath.c_str());
		return NULL;
	}

	remove(compiledCompositionPath.c_str());

	return newSeparateProcessRunnerFromExecutable(linkedCompositionPath, sourceDir, continueIfRunnerDies, true);
}

/**
 * Initializes the compiler's cache in a separate process.
 *
 * This function is useful in conjunction with newSeparateProcessRunnerFromCompositionFile() or
 * newSeparateProcessRunnerFromCompositionString() , when using the `useExistingCache` for those functions.
 *
 * This function is less efficient than VuoCompiler::prepareForFastBuild(), so you should call that function
 * instead unless you're running compositions from a 32-bit process and thus can't use VuoCompiler.
 */
void VuoRunner::initializeCompilerCache()
{
	string composition = "digraph G {}";
	string name = "InitializeCompilerCache";
	string sourceDir = VuoFileUtilities::getTmpDir();
	VuoRunner *r = newSeparateProcessRunnerFromCompositionString(composition, name, sourceDir, false, false);
	delete r;
}

/**
 * Creates a runner that can run a composition in a new process.
 *
 * @param executablePath A linked composition executable, produced by VuoCompiler::linkCompositionToCreateExecutable().
 * @param sourceDir The directory containing the composition (.vuo) source file, used by nodes in the composition to resolve relative paths.
 * @param continueIfRunnerDies If true, the composition keeps running if the runner process exits without stopping the composition.
 * @param deleteExecutableWhenFinished True if the runner should delete @c executablePath when it's finished using the file.
 *
 * @see CompileAndRunInNewProcess.cc
 */
VuoRunner * VuoRunner::newSeparateProcessRunnerFromExecutable(string executablePath, string sourceDir,
															  bool continueIfRunnerDies, bool deleteExecutableWhenFinished)
{
	VuoRunner * vr = new VuoRunner();
	vr->executablePath = executablePath;
	vr->shouldContinueIfRunnerDies = continueIfRunnerDies;
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
 * @param continueIfRunnerDies If true, the composition keeps running if the runner process exits without stopping the composition.
 * @param deleteDylibsWhenFinished True if the runner should delete @c compositionDylibPath and @c resourceDylibPath when it's finished using the files.
 */
VuoRunner * VuoRunner::newSeparateProcessRunnerFromDynamicLibrary(string compositionLoaderPath, string compositionDylibPath,
																  string resourceDylibPath, string sourceDir,
																  bool continueIfRunnerDies, bool deleteDylibsWhenFinished)
{
	VuoRunner * vr = new VuoRunner();
	vr->executablePath = compositionLoaderPath;
	vr->dylibPath = compositionDylibPath;
	vr->resourceDylibPaths.push_back(resourceDylibPath);
	vr->shouldContinueIfRunnerDies = continueIfRunnerDies;
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
	dispatch_release(terminatedZMQContextSemaphore);
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
	shouldContinueIfRunnerDies = false;
	shouldDeleteBinariesWhenFinished = false;
	paused = true;
	stopped = true;
	lostContact = false;
	listenCanceled = false;
	stoppedSemaphore = dispatch_semaphore_create(1);
	terminatedZMQContextSemaphore = dispatch_semaphore_create(0);
	beganListeningSemaphore = dispatch_semaphore_create(0);
	endedListeningSemaphore = dispatch_semaphore_create(1);
	controlQueue = dispatch_queue_create("org.vuo.runner.control", NULL);
	ZMQContext = NULL;
	ZMQSelfSend = NULL;
	ZMQSelfReceive = NULL;
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
	try
	{
		startInternal();

		if (isInCurrentProcess())
		{
			dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
			dispatch_async(queue, ^{
							   unpause();
						   });
			while (paused)
			{
				drainMainDispatchQueue();
				usleep(USEC_PER_SEC / 1000);
			}
		}
		else
		{
			unpause();
		}
	}
	catch (exception &e)
	{
		stopBecauseLostContact(e.what());
	}
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
	try
	{
		startInternal();
	}
	catch (exception &e)
	{
		stopBecauseLostContact(e.what());
	}
}

/**
 * Internal helper method for starting the composition running.
 *
 * Assumes the composition is not already running.
 *
 * Upon return, the composition has been started in a paused state.
 *
 * @throw std::runtime_error The composition couldn't start or the connection between the runner and the composition couldn't be established.
 */
void VuoRunner::startInternal(void)
{
	stopped = false;
	dispatch_semaphore_wait(stoppedSemaphore, DISPATCH_TIME_FOREVER);

	ZMQContext = zmq_init(1);

	if (isInCurrentProcess())
	{
		// Start the composition in the current process.

		dylibHandle = dlopen(dylibPath.c_str(), RTLD_NOW);
		if (!dylibHandle)
			throw std::runtime_error("The composition couldn't start because the library '" + dylibPath + "' couldn't be loaded : " + dlerror());

		VuoInitInProcessType *vuoInitInProcess = (VuoInitInProcessType *)dlsym(dylibHandle, "vuoInitInProcess");
		if (! vuoInitInProcess)
			throw std::runtime_error("The composition couldn't start because vuoInitInProcess() couldn't be found in '" + dylibPath + "' : " + dlerror());

		ZMQControlURL = "inproc://" + VuoFileUtilities::makeTmpFile("vuo-control", "");
		ZMQTelemetryURL = "inproc://" + VuoFileUtilities::makeTmpFile("vuo-telemetry", "");

		// Set the current working directory to that of the source .vuo composition so that
		// relative URL paths are resolved correctly.
		char buffer[PATH_MAX];
		char *retBuffer = getcwd(buffer, PATH_MAX);
		if (! retBuffer)
			throw std::runtime_error("The composition couldn't start because the working directory couldn't be obtained : " + string(strerror(errno)));
		originalWorkingDir = buffer;
		if (!sourceDir.empty())
		{
			int ret = chdir(sourceDir.c_str());
			if (ret)
				throw std::runtime_error("The composition couldn't start because the working directory couldn't be changed to '" + sourceDir + "' : " + strerror(errno));
		}

		vuoInitInProcess(ZMQContext, ZMQControlURL.c_str(), ZMQTelemetryURL.c_str(), true, getpid(), -1, false, trialRestrictionsEnabled, NULL);
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

		ostringstream oss;
		oss << compositionReadRunnerWritePipe[0];
		args.push_back("--vuo-runner-pipe=" + oss.str());
		if (shouldContinueIfRunnerDies)
			args.push_back("--vuo-continue-if-runner-dies");

		if (isUsingCompositionLoader())
		{
			ZMQLoaderControlURL = "ipc://" + VuoFileUtilities::makeTmpFile("vuo-loader", "");
			args.push_back("--vuo-loader=" + ZMQLoaderControlURL);

			if (!trialRestrictionsEnabled)
				args.push_back("--vuo-full");
		}
		else
		{
			args.push_back("--vuo-pause");

			if (trialRestrictionsEnabled)
				args.push_back("--vuo-trial");
		}

		int fd[2];
		int ret = pipe(fd);
		if (ret)
			throw std::runtime_error("The composition couldn't start because a pipe couldn't be opened : " + string(strerror(errno)));

		char * argv[7];
		int argSize = args.size();
		for (size_t i = 0; i < argSize; ++i)
		{
			argv[i] = (char *) malloc(args[i].length() + 1);
			strcpy(argv[i], args[i].c_str());
		}
		argv[argSize] = NULL;

		string errorWorkingDirectory = "The composition couldn't start because the working directory couldn't be changed to '" + sourceDir + "' : ";
		string errorExecutable = "The composition couldn't start because the file '" + executablePath + "' couldn't be executed : ";
		string errorFork = "The composition couldn't start because the composition process couldn't be forked : ";
		const size_t ERROR_BUFFER_LEN = 256;
		char errorBuffer[ERROR_BUFFER_LEN];

		pipe(runnerReadCompositionWritePipe);
		allCompositionWritePipes.push_back(runnerReadCompositionWritePipe[1]);

		pid_t childPid = fork();
		if (childPid == 0)
		{
			// There are only a limited set of functions you're allowed to call in the child process
			// after fork() and before exec(). Functions such as VUserLog() and exit() aren't allowed,
			// so instead we're calling alternatives such as write() and _exit().

			close(compositionReadRunnerWritePipe[1]);
			close(runnerReadCompositionWritePipe[0]);

			// Close the write pipes for this process's other running compositions, so we don't block their shutdown.
			for (vector<int>::iterator i = allCompositionWritePipes.begin(); i != allCompositionWritePipes.end(); ++i)
				if (*i != runnerReadCompositionWritePipe[1])
					close(*i);

			pid_t grandchildPid = fork();
			if (grandchildPid == 0)
			{
				close(fd[0]);
				close(fd[1]);

				// Set the current working directory to that of the source .vuo composition so that
				// relative URL paths are resolved correctly.
				if (!sourceDir.empty())
				{
					ret = chdir(sourceDir.c_str());
					if (ret)
					{
						strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
						write(STDERR_FILENO, errorWorkingDirectory.c_str(), errorWorkingDirectory.length());
						write(STDERR_FILENO, errorBuffer, strlen(errorBuffer));
						write(STDERR_FILENO, "\n", 1);
						_exit(-1);
					}
				}

				ret = execv(executablePath.c_str(), argv);
				if (ret)
				{
					strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
					write(STDERR_FILENO, errorExecutable.c_str(), errorExecutable.length());
					write(STDERR_FILENO, errorBuffer, strlen(errorBuffer));
					write(STDERR_FILENO, "\n", 1);
					_exit(-1);
				}
			}
			else if (grandchildPid > 0)
			{
				close(fd[0]);

				write(fd[1], &grandchildPid, sizeof(pid_t));
				close(fd[1]);

				_exit(0);
			}
			else
			{
				close(fd[0]);
				close(fd[1]);

				strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
				write(STDERR_FILENO, errorFork.c_str(), errorFork.length());
				write(STDERR_FILENO, errorBuffer, strlen(errorBuffer));
				write(STDERR_FILENO, "\n", 1);
				_exit(-1);
			}
		}
		else if (childPid > 0)
		{
			close(fd[1]);

			for (size_t i = 0; i < argSize; ++i)
				free(argv[i]);

			pid_t grandchildPid;
			read(fd[0], &grandchildPid, sizeof(pid_t));
			close(fd[0]);

			// Reap the child process.
			int status;
			int ret;
			do {
				ret = waitpid(childPid, &status, 0);
			} while (ret == -1 && errno == EINTR);
			if (WIFEXITED(status) && WEXITSTATUS(status))
				throw std::runtime_error("The composition couldn't start because the parent of the composition process exited with an error.");
			else if (WIFSIGNALED(status))
				throw std::runtime_error("The composition couldn't start because the parent of the composition process exited abnormally : " + string(strsignal(WTERMSIG(status))));

			if (grandchildPid > 0)
				compositionPid = grandchildPid;
			else
				throw std::runtime_error("The composition couldn't start because the composition process id couldn't be obtained");
		}
		else
		{
			for (size_t i = 0; i < argSize; ++i)
				free(argv[i]);

			throw std::runtime_error("The composition couldn't start because the parent of the composition process couldn't be forked : " + string(strerror(errno)));
		}
	}

	// Connect to the composition loader (if any) and composition.
	if (isUsingCompositionLoader())
	{
		ZMQLoaderControl = zmq_socket(ZMQContext,ZMQ_REQ);
		VuoRunner_configureSocket(ZMQLoaderControl, 5);

		// Try to connect to the composition loader. If at first we don't succeed, give the composition loader a little more time to set up the socket.
		int numTries = 0;
		while (zmq_connect(ZMQLoaderControl,ZMQLoaderControlURL.c_str()))
		{
			if (++numTries == 1000)
				throw std::runtime_error("The composition couldn't start because the runner couldn't establish communication with the composition loader : " + string(strerror(errno)));
			usleep(USEC_PER_SEC / 1000);
		}

		replaceComposition(dylibPath, resourceDylibPaths.at(0), "");
	}
	else
	{
		__block string errorMessage;
		dispatch_sync(controlQueue, ^{
						  try {
							  setUpConnections();
						  } catch (exception &e) {
							  errorMessage = e.what();
						  }
					  });
		if (! errorMessage.empty())
			throw std::runtime_error(errorMessage);
	}
}

/**
 * Establishes ZMQ connections with the running composition.
 *
 * @throw std::runtime_error The connection between the runner and the composition failed.
 */
void VuoRunner::setUpConnections(void)
{
	ZMQControl = zmq_socket(ZMQContext,ZMQ_REQ);
	VuoRunner_configureSocket(ZMQControl, -1);

	// Try to connect to the composition. If at first we don't succeed, give the composition a little more time to set up the socket.
	int numTries = 0;
	while (zmq_connect(ZMQControl,ZMQControlURL.c_str()))
	{
		if (++numTries == 1000)
			throw std::runtime_error("The composition couldn't start because the runner couldn't establish communication to control the composition : " + string(strerror(errno)));
		usleep(USEC_PER_SEC / 1000);
	}

	// Cache published ports so they're available whenever a caller starts listening for published port value changes.
	arePublishedInputPortsCached = false;
	arePublishedOutputPortsCached = false;
	if (isInCurrentProcess())
	{
		dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
		__block string publishedPortsError;
		dispatch_async(queue, ^{
						   try {
							   getCachedPublishedPorts(false);
							   getCachedPublishedPorts(true);
						   } catch (exception &e) {
							   publishedPortsError = e.what();
						   }
					   });
		while (! (arePublishedInputPortsCached && arePublishedOutputPortsCached) )
		{
			drainMainDispatchQueue();
			usleep(USEC_PER_SEC / 1000);

			if (! publishedPortsError.empty())
				throw std::runtime_error(publishedPortsError);
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
	__block bool listenOk = true;
	__block string listenError;
	dispatch_async(queue, ^{
					   listenOk = listen(listenError);
					   if (!listenOk)
						   dispatch_semaphore_signal(beganListeningSemaphore);
				   });
	dispatch_semaphore_wait(beganListeningSemaphore, DISPATCH_TIME_FOREVER);
	if (! listenOk)
		throw std::runtime_error("The composition couldn't start because the runner couldn't establish communication to listen to the composition : " + listenError);

	vuoControlRequestSend(VuoControlRequestSlowHeartbeat,NULL,0);
	vuoControlReplyReceive(VuoControlReplyHeartbeatSlowed);
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
		VuoEventLoop_processEvent(VuoEventLoop_WaitIndefinitely);
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

	VuoEventLoop_processEvent(VuoEventLoop_RunOnce);
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
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  vuoControlRequestSend(VuoControlRequestCompositionPause,NULL,0);
						  vuoControlReplyReceive(VuoControlReplyCompositionPaused);
					  }
					  catch (exception &e)
					  {
						  stopBecauseLostContact(e.what());
					  }

					  paused = true;
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
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  vuoControlRequestSend(VuoControlRequestCompositionUnpause,NULL,0);
						  vuoControlReplyReceive(VuoControlReplyCompositionUnpaused);
					  }
					  catch (exception &e)
					  {
						  stopBecauseLostContact(e.what());
					  }

					  paused = false;
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
					  if (stopped || lostContact) {
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

					  try
					  {
						  if (! paused)
						  {
							  vuoControlRequestSend(VuoControlRequestCompositionPause,NULL,0);
							  vuoControlReplyReceive(VuoControlReplyCompositionPaused);
						  }

						  cleanUpConnections();

						  zmq_msg_t messages[3];
						  vuoInitMessageWithString(&messages[0], dylibPath.c_str());
						  vuoInitMessageWithString(&messages[1], resourceDylibPath.c_str());
						  vuoInitMessageWithString(&messages[2], compositionDiff.c_str());

						  vuoLoaderControlRequestSend(VuoLoaderControlRequestCompositionReplace,messages,3);
						  vuoLoaderControlReplyReceive(VuoLoaderControlReplyCompositionReplaced);

						  setUpConnections();

						  if (! paused)
						  {
							  vuoControlRequestSend(VuoControlRequestCompositionUnpause,NULL,0);
							  vuoControlReplyReceive(VuoControlReplyCompositionUnpaused);
						  }
					  }
					  catch (exception &e)
					  {
						  stopBecauseLostContact(e.what());
					  }
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
					  if (! lostContact)
					  {
						  try
						  {
							  int timeoutInSeconds = (isInCurrentProcess() ? -1 : 5);
							  zmq_msg_t messages[2];
							  vuoInitMessageWithInt(&messages[0], timeoutInSeconds);
							  vuoInitMessageWithBool(&messages[1], false);
							  vuoControlRequestSend(VuoControlRequestCompositionStop,messages,2);

							  if (isInCurrentProcess() && isMainThread())
							  {
								  // If VuoRunner::stop() is blocking the main thread, wait for the composition's reply on another thread, and drain the main queue, in case the composition needs to shut down stuff that requires the main queue.
								  __block bool replyReceived = false;
								  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
									  vuoMemoryBarrier();
									  try
									  {
										  vuoControlReplyReceive(VuoControlReplyCompositionStopping);
									  }
									  catch (exception &e)
									  {
										  // do nothing; doesn't matter if connection timed out
									  }
									  replyReceived = true;
								  });
								  while (!replyReceived)
								  {
									  drainMainDispatchQueue();
									  usleep(USEC_PER_SEC / 1000);
								  }
								  vuoMemoryBarrier();
							  }
							  else
								  vuoControlReplyReceive(VuoControlReplyCompositionStopping);
						  }
						  catch (exception &e)
						  {
							  // do nothing; doesn't matter if connection timed out
						  }
					  }

					  cleanUpConnections();

					  if (isUsingCompositionLoader() && ZMQLoaderControl)
					  {
						  zmq_close(ZMQLoaderControl);
						  ZMQLoaderControl = NULL;
					  }

					  if (isInCurrentProcess() && dylibHandle)
					  {
						  VuoInitInProcessType *vuoInitInProcess = (VuoInitInProcessType *)dlsym(dylibHandle, "vuoInitInProcess");
						  if (vuoInitInProcess)  // Avoid double jeopardy if startInternal() already failed for missing vuoInitInProcess.
						  {
							  VuoFiniType *vuoFini = (VuoFiniType *)dlsym(dylibHandle, "vuoFini");
							  if (! vuoFini)
							  {
								  VUserLog("The composition couldn't stop because vuoFini() couldn't be found in '%s' : %s", dylibPath.c_str(), dlerror());
								  return;
							  }
							  vuoFini();
						  }

						  dlclose(dylibHandle);
						  dylibHandle = NULL;

						  int ret = chdir(originalWorkingDir.c_str());
						  if (ret)
						  {
							  VUserLog("The working directory couldn't be restored after the composition was stopped : %s", strerror(errno));
						  }
					  }
					  else if (isInCurrentProcess() && !dylibHandle)
					  {
						  // If the dylib isn't open, the composition isn't running, so there's nothing to clean up.
					  }
					  else
					  {
						  char buf[1];
						  close(runnerReadCompositionWritePipe[1]);

						  // Remove compositionPipe[1] from the process-wide pipe list.
						  vector<int>::iterator it = find(allCompositionWritePipes.begin(), allCompositionWritePipes.end(), runnerReadCompositionWritePipe[1]);
						  if (it != allCompositionWritePipes.end())
							  allCompositionWritePipes.erase(it);

						  // Wait for child process to end.
						  // Can't use waitpid() since it only waits on child processes, yet compositionPid is a grandchild.
						  // Instead, do a blocking read() — the grandchild never writes anything to the pipe, and when the grandchild exits,
						  // read() will return EOF (since it was the last process that had it open for writing).
						  read(runnerReadCompositionWritePipe[0], &buf, 1);
						  close(runnerReadCompositionWritePipe[0]);

						  if (! lostContact)
						  {
							  zmq_term(ZMQContext);
							  ZMQContext = NULL;
						  }
						  else
						  {
							  dispatch_semaphore_wait(terminatedZMQContextSemaphore, DISPATCH_TIME_FOREVER);
						  }
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
					  VuoEventLoop_break();
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

	// Break out of zmq_poll().
	char z = 0;
	zmq_msg_t message;
	zmq_msg_init_size(&message, sizeof z);
	memcpy(zmq_msg_data(&message), &z, sizeof z);
	zmq_send(ZMQSelfSend, &message, 0);
	zmq_msg_close(&message);

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
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  zmq_msg_t messages[2];
						  vuoInitMessageWithString(&messages[0], portIdentifier.c_str());
						  vuoInitMessageWithString(&messages[1], valueAsString);
						  vuoControlRequestSend(VuoControlRequestInputPortValueModify, messages, 2);
						  vuoControlReplyReceive(VuoControlReplyInputPortValueModified);
					  }
					  catch (exception &e)
					  {
						  stopBecauseLostContact(e.what());
					  }
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
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  zmq_msg_t messages[1];
						  vuoInitMessageWithString(&messages[0], portIdentifier.c_str());
						  vuoControlRequestSend(VuoControlRequestTriggerPortFireEvent, messages, 1);
						  vuoControlReplyReceive(VuoControlReplyTriggerPortFiredEvent);
					  }
					  catch (exception &e)
					  {
						  stopBecauseLostContact(e.what());
					  }
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
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  zmq_msg_t messages[2];
						  vuoInitMessageWithBool(&messages[0], !isInCurrentProcess());
						  vuoInitMessageWithString(&messages[1], portIdentifier.c_str());
						  vuoControlRequestSend(VuoControlRequestInputPortValueRetrieve, messages, 2);
						  vuoControlReplyReceive(VuoControlReplyInputPortValueRetrieved);
						  valueAsString = receiveString("null");
					  }
					  catch (exception &e)
					  {
						  stopBecauseLostContact(e.what());
					  }
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
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  zmq_msg_t messages[2];
						  vuoInitMessageWithBool(&messages[0], !isInCurrentProcess());
						  vuoInitMessageWithString(&messages[1], portIdentifier.c_str());
						  vuoControlRequestSend(VuoControlRequestOutputPortValueRetrieve, messages, 2);
						  vuoControlReplyReceive(VuoControlReplyOutputPortValueRetrieved);
						  valueAsString = receiveString("null");
					  }
					  catch (exception &e)
					  {
						  stopBecauseLostContact(e.what());
					  }
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
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  zmq_msg_t messages[1];
						  vuoInitMessageWithString(&messages[0], portIdentifier.c_str());
						  vuoControlRequestSend(VuoControlRequestInputPortSummaryRetrieve, messages, 1);
						  vuoControlReplyReceive(VuoControlReplyInputPortSummaryRetrieved);
						  summary = receiveString("");
					  }
					  catch (exception &e)
					  {
						  stopBecauseLostContact(e.what());
					  }
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
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  zmq_msg_t messages[1];
						  vuoInitMessageWithString(&messages[0], portIdentifier.c_str());
						  vuoControlRequestSend(VuoControlRequestOutputPortSummaryRetrieve, messages, 1);
						  vuoControlReplyReceive(VuoControlReplyOutputPortSummaryRetrieved);
						  summary = receiveString("");
					  }
					  catch (exception &e)
					  {
						  stopBecauseLostContact(e.what());
					  }
				  });
	return summary;
}

/**
 * Sends a control request to the composition telling it to start sending telemetry for each event through an input port.
 *
 * Assumes the composition has been started and has not been stopped.
 *
 * @param portIdentifier The compile-time identifier for the port (see VuoCompilerEventPort::getIdentifier()).
 * @return A brief description of the port's value.
 *
 * @see VuoTypes for information about types and their summaries.
 */
string VuoRunner::subscribeToInputPortTelemetry(string portIdentifier)
{
	__block string summary;
	dispatch_sync(controlQueue, ^{
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  zmq_msg_t messages[1];
						  vuoInitMessageWithString(&messages[0], portIdentifier.c_str());
						  vuoControlRequestSend(VuoControlRequestInputPortTelemetrySubscribe, messages, 1);
						  vuoControlReplyReceive(VuoControlReplyInputPortTelemetrySubscribed);
						  summary = receiveString("");
					  }
					  catch (exception &e)
					  {
						  stopBecauseLostContact(e.what());
					  }
				  });
	return summary;
}

/**
 * Sends a control request to the composition telling it to start sending telemetry for each event through an output port.
 *
 * Assumes the composition has been started and has not been stopped.
 *
 * @param portIdentifier The compile-time identifier for the port (see VuoCompilerEventPort::getIdentifier()).
 * @return A brief description of the port's value.
 *
 * @see VuoTypes for information about types and their summaries.
 */
string VuoRunner::subscribeToOutputPortTelemetry(string portIdentifier)
{
	__block string summary;
	dispatch_sync(controlQueue, ^{
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  zmq_msg_t messages[1];
						  vuoInitMessageWithString(&messages[0], portIdentifier.c_str());
						  vuoControlRequestSend(VuoControlRequestOutputPortTelemetrySubscribe, messages, 1);
						  vuoControlReplyReceive(VuoControlReplyOutputPortTelemetrySubscribed);
						  summary = receiveString("");
					  }
					  catch (exception &e)
					  {
						  stopBecauseLostContact(e.what());
					  }
				  });
	return summary;
}

/**
 * Sends a control request to the composition telling it to stop sending telemetry for each event through an input port.
 *
 * Assumes the composition has been started and has not been stopped.
 *
 * @param portIdentifier The compile-time identifier for the port (see VuoCompilerEventPort::getIdentifier()).
 */
void VuoRunner::unsubscribeFromInputPortTelemetry(string portIdentifier)
{
	dispatch_sync(controlQueue, ^{
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  zmq_msg_t messages[1];
						  vuoInitMessageWithString(&messages[0], portIdentifier.c_str());
						  vuoControlRequestSend(VuoControlRequestInputPortTelemetryUnsubscribe, messages, 1);
						  vuoControlReplyReceive(VuoControlReplyInputPortTelemetryUnsubscribed);
					  }
					  catch (exception &e)
					  {
						  stopBecauseLostContact(e.what());
					  }
				  });
}

/**
 * Sends a control request to the composition telling it to stop sending telemetry for each event through an output port.
 *
 * Assumes the composition has been started and has not been stopped.
 *
 * @param portIdentifier The compile-time identifier for the port (see VuoCompilerEventPort::getIdentifier()).
 */
void VuoRunner::unsubscribeFromOutputPortTelemetry(string portIdentifier)
{
	dispatch_sync(controlQueue, ^{
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  zmq_msg_t messages[1];
						  vuoInitMessageWithString(&messages[0], portIdentifier.c_str());
						  vuoControlRequestSend(VuoControlRequestOutputPortTelemetryUnsubscribe, messages, 1);
						  vuoControlReplyReceive(VuoControlReplyOutputPortTelemetryUnsubscribed);
					  }
					  catch (exception &e)
					  {
						  stopBecauseLostContact(e.what());
					  }
				  });
}

/**
 * Sends a control request to the composition telling it to start sending telemetry for all events.
 *
 * Assumes the composition has been started and has not been stopped.
 */
void VuoRunner::subscribeToEventTelemetry(void)
{
	dispatch_sync(controlQueue, ^{
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  vuoControlRequestSend(VuoControlRequestEventTelemetrySubscribe, NULL, 0);
						  vuoControlReplyReceive(VuoControlReplyEventTelemetrySubscribed);
					  }
					  catch (exception &e)
					  {
						  stopBecauseLostContact(e.what());
					  }
				  });
}

/**
 * Sends a control request to the composition telling it to stop sending telemetry for all events.
 * The composition will continue sending any telemetry subscribed by subscribeToInputPortTelemetry(),
 * subscribeToOutputPortTelemetry() or subscribeToAllTelemetry().
 *
 * Assumes the composition has been started and has not been stopped.
 */
void VuoRunner::unsubscribeFromEventTelemetry(void)
{
	dispatch_sync(controlQueue, ^{
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  vuoControlRequestSend(VuoControlRequestEventTelemetryUnsubscribe, NULL, 0);
						  vuoControlReplyReceive(VuoControlReplyEventTelemetryUnsubscribed);
					  }
					  catch (exception &e)
					  {
						  stopBecauseLostContact(e.what());
					  }
				  });
}

/**
 * Sends a control request to the composition telling it to start sending all telemetry.
 *
 * Assumes the composition has been started and has not been stopped.
 */
void VuoRunner::subscribeToAllTelemetry(void)
{
	dispatch_sync(controlQueue, ^{
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  vuoControlRequestSend(VuoControlRequestAllTelemetrySubscribe, NULL, 0);
						  vuoControlReplyReceive(VuoControlReplyAllTelemetrySubscribed);
					  }
					  catch (exception &e)
					  {
						  stopBecauseLostContact(e.what());
					  }
				  });
}

/**
 * Sends a control request to the composition telling it to stop sending all telemetry
 * The composition will continue sending any telemetry subscribed by subscribeToInputPortTelemetry(),
 * subscribeToOutputPortTelemetry(), or subscribeToEventTelemetry().
 *
 * Assumes the composition has been started and has not been stopped.
 */
void VuoRunner::unsubscribeFromAllTelemetry(void)
{
	dispatch_sync(controlQueue, ^{
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  vuoControlRequestSend(VuoControlRequestAllTelemetryUnsubscribe, NULL, 0);
						  vuoControlReplyReceive(VuoControlReplyAllTelemetryUnsubscribed);
					  }
					  catch (exception &e)
					  {
						  stopBecauseLostContact(e.what());
					  }
				  });
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
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  zmq_msg_t messages[2];
						  vuoInitMessageWithString(&messages[0], port->getName().c_str());
						  vuoInitMessageWithString(&messages[1], valueAsString);
						  vuoControlRequestSend(VuoControlRequestPublishedInputPortValueModify, messages, 2);
						  vuoControlReplyReceive(VuoControlReplyPublishedInputPortValueModified);
					  }
					  catch (exception &e)
					  {
						  stopBecauseLostContact(e.what());
					  }
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
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  anyPublishedPortEventSignaled = false;

					  try
					  {
						  zmq_msg_t messages[1];
						  vuoInitMessageWithString(&messages[0], port->getName().c_str());
						  vuoControlRequestSend(VuoControlRequestPublishedInputPortFireEvent, messages, 1);
						  vuoControlReplyReceive(VuoControlReplyPublishedInputPortFiredEvent);
					  }
					  catch (exception &e)
					  {
						  stopBecauseLostContact(e.what());
					  }
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
	Port *port = getPublishedInputPorts().at(0);
	firePublishedInputPortEvent(port);
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
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  zmq_msg_t messages[2];
						  vuoInitMessageWithBool(&messages[0], !isInCurrentProcess());
						  vuoInitMessageWithString(&messages[1], port->getName().c_str());
						  vuoControlRequestSend(VuoControlRequestPublishedInputPortValueRetrieve, messages, 2);
						  vuoControlReplyReceive(VuoControlReplyPublishedInputPortValueRetrieved);
						  valueAsString = receiveString("null");
					  }
					  catch (exception &e)
					  {
						  stopBecauseLostContact(e.what());
					  }
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
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  zmq_msg_t messages[2];
						  vuoInitMessageWithBool(&messages[0], !isInCurrentProcess());
						  vuoInitMessageWithString(&messages[1], port->getName().c_str());
						  vuoControlRequestSend(VuoControlRequestPublishedOutputPortValueRetrieve, messages, 2);
						  vuoControlReplyReceive(VuoControlReplyPublishedOutputPortValueRetrieved);
						  valueAsString = receiveString("null");
					  }
					  catch (exception &e)
					  {
						  stopBecauseLostContact(e.what());
					  }
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
 *
 * @throw std::runtime_error The connection between the runner and the composition failed.
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
 *
 * @throw std::runtime_error The connection between the runner and the composition failed.
 *		This function imposes a 5-second timeout.
 */
vector<VuoRunner::Port *> VuoRunner::refreshPublishedPorts(bool input)
{
	dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
	dispatch_source_t timeout = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue);
	dispatch_source_set_timer(timeout, dispatch_time(DISPATCH_TIME_NOW, 5*NSEC_PER_SEC), NSEC_PER_SEC, NSEC_PER_SEC/10);
	dispatch_source_set_event_handler(timeout, ^{
										  stopBecauseLostContact("The connection between the composition and runner timed out when trying to receive the list of published ports");
										  dispatch_source_cancel(timeout);
	});
	dispatch_resume(timeout);

	vector<VuoRunner::Port *> ports;

	try
	{
		vuoMemoryBarrier();

		enum VuoControlRequest requests[4];
		enum VuoControlReply replies[4];
		if (input)
		{
			requests[0] = VuoControlRequestPublishedInputPortNamesRetrieve;
			requests[1] = VuoControlRequestPublishedInputPortTypesRetrieve;
			requests[2] = VuoControlRequestPublishedInputPortDetailsRetrieve;
			replies[0] = VuoControlReplyPublishedInputPortNamesRetrieved;
			replies[1] = VuoControlReplyPublishedInputPortTypesRetrieved;
			replies[2] = VuoControlReplyPublishedInputPortDetailsRetrieved;
		}
		else
		{
			requests[0] = VuoControlRequestPublishedOutputPortNamesRetrieve;
			requests[1] = VuoControlRequestPublishedOutputPortTypesRetrieve;
			requests[2] = VuoControlRequestPublishedOutputPortDetailsRetrieve;
			replies[0] = VuoControlReplyPublishedOutputPortNamesRetrieved;
			replies[1] = VuoControlReplyPublishedOutputPortTypesRetrieved;
			replies[2] = VuoControlReplyPublishedOutputPortDetailsRetrieved;
		}

		vector<string> names;
		vector<string> types;
		vector<string> details;

		for (int i = 0; i < 3; ++i)
		{
			vuoControlRequestSend(requests[i], NULL, 0);
			vuoControlReplyReceive(replies[i]);
			vector<string> messageStrings = receiveListOfStrings();
			if (i == 0)
				names = messageStrings;
			else if (i == 1)
				types = messageStrings;
			else
				details = messageStrings;
		}

		for (size_t i = 0; i < names.size() && i < types.size() && i < details.size(); ++i)
		{
			VuoRunner::Port *port = new Port(names[i], types[i], json_tokener_parse(details[i].c_str()));
			ports.push_back(port);
		}
	}
	catch (exception &e)
	{
		dispatch_source_cancel(timeout);
		dispatch_release(timeout);
		throw;
	}

	dispatch_source_cancel(timeout);
	dispatch_release(timeout);

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
 * Assumes the composition has been started and has not yet been unpaused. After this function signals
 * @c beganListeningSemaphore, it's OK to unpause the composition.
 *
 * When this function signals beganListeningSemaphore, a connection has been established with the composition.
 * The composition should not be unpaused before then, or else it might miss important one-time-only messages,
 * such as VuoTelemetryOutputPortsUpdated from a `Fire on Start` event. After the semaphore is signaled, it's
 * safe to unpause the composition.
 */
bool VuoRunner::listen(string &error)
{
	ZMQSelfReceive = zmq_socket(ZMQContext, ZMQ_PAIR);
	VuoRunner_configureSocket(ZMQSelfReceive, -1);
	if (zmq_bind(ZMQSelfReceive, "inproc://vuo-runner-self") != 0)
	{
		error = strerror(errno);
		return false;
	}

	ZMQSelfSend = zmq_socket(ZMQContext, ZMQ_PAIR);
	VuoRunner_configureSocket(ZMQSelfSend, -1);
	if (zmq_connect(ZMQSelfSend, "inproc://vuo-runner-self") != 0)
	{
		error = strerror(errno);
		return false;
	}

	{
		ZMQTelemetry = zmq_socket(ZMQContext,ZMQ_SUB);
		VuoRunner_configureSocket(ZMQTelemetry, -1);
		if(zmq_connect(ZMQTelemetry,ZMQTelemetryURL.c_str()))
		{
			error = strerror(errno);
			return false;
		}
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
		type = VuoTelemetryPublishedOutputPortsUpdated;
		zmq_setsockopt(ZMQTelemetry, ZMQ_SUBSCRIBE, &type, sizeof type);
		type = VuoTelemetryEventDropped;
		zmq_setsockopt(ZMQTelemetry, ZMQ_SUBSCRIBE, &type, sizeof type);
		type = VuoTelemetryError;
		zmq_setsockopt(ZMQTelemetry, ZMQ_SUBSCRIBE, &type, sizeof type);
		type = VuoTelemetryStopRequested;
		zmq_setsockopt(ZMQTelemetry, ZMQ_SUBSCRIBE, &type, sizeof type);
	}

	{
		// Wait until the connection is established, as evidenced by a heartbeat telemetry message
		// being received from the composition. This is necessary because the ØMQ API doesn't provide
		// any way to tell when a SUB socket is ready to receive messages, and if you call zmq_poll()
		// on it before it's ready, then it might miss messages that came in while it was still trying
		// to get ready. (The zmq_connect() function doesn't make any guarantees about the socket being ready.
		// It just starts some setup that may continue asynchronously after zmq_connect() has returned.)
		// To avoid missing important telemetry messages from the composition, we make sure that the
		// runner doesn't tell the composition to unpause until the runner has verified that it's
		// receiving heartbeat telemetry messages. http://zguide.zeromq.org/page:all#Node-Coordination
		zmq_pollitem_t items[]=
		{
			{ZMQTelemetry,0,ZMQ_POLLIN,0},
		};
		int itemCount = 1;
		long timeout = -1;
		zmq_poll(items,itemCount,timeout);
	}

	dispatch_semaphore_signal(beganListeningSemaphore);

	while(! listenCanceled)
	{
		zmq_pollitem_t items[]=
		{
			{ZMQTelemetry,0,ZMQ_POLLIN,0},
			{ZMQSelfReceive,0,ZMQ_POLLIN,0},
		};
		int itemCount = 2;
		long timeout = USEC_PER_SEC;  // Wait 1 second.  If no telemetry was received in that second, we probably lost contact with the composition.
		zmq_poll(items,itemCount,timeout);
		if(items[0].revents & ZMQ_POLLIN)
		{
			// Receive telemetry type.
			char type = vuoReceiveInt(ZMQTelemetry, NULL);

			// Receive telemetry arguments and forward to VuoRunnerDelegate.
			switch (type)
			{
				case VuoTelemetryStats:
				{
					unsigned long utime = vuoReceiveUnsignedInt64(ZMQTelemetry, NULL);
					unsigned long stime = vuoReceiveUnsignedInt64(ZMQTelemetry, NULL);
					dispatch_sync(delegateQueue, ^{
									  if (delegate)
										  delegate->receivedTelemetryStats(utime, stime);
								  });
					break;
				}
				case VuoTelemetryNodeExecutionStarted:
				{
					char *nodeIdentifier = vuoReceiveAndCopyString(ZMQTelemetry, NULL);
					dispatch_sync(delegateQueue, ^{
									  if (delegate)
										  delegate->receivedTelemetryNodeExecutionStarted( string(nodeIdentifier) );
								  });
					free(nodeIdentifier);
					break;
				}
				case VuoTelemetryNodeExecutionFinished:
				{
					char *nodeIdentifier = vuoReceiveAndCopyString(ZMQTelemetry, NULL);
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
						char *portIdentifier = vuoReceiveAndCopyString(ZMQTelemetry, NULL);
						if (hasMoreToReceive(ZMQTelemetry))
						{
							bool receivedEvent = vuoReceiveBool(ZMQTelemetry, NULL);
							if (hasMoreToReceive(ZMQTelemetry))
							{
								bool receivedData = vuoReceiveBool(ZMQTelemetry, NULL);
								if (hasMoreToReceive(ZMQTelemetry))
								{
									string portDataSummary;
									char *s = vuoReceiveAndCopyString(ZMQTelemetry, NULL);
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
						char *portIdentifier = vuoReceiveAndCopyString(ZMQTelemetry, NULL);
						if (hasMoreToReceive(ZMQTelemetry))
						{
							bool sentData = vuoReceiveBool(ZMQTelemetry, NULL);
							if (hasMoreToReceive(ZMQTelemetry))
							{
								string portDataSummary;
								char *s = vuoReceiveAndCopyString(ZMQTelemetry, NULL);
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
							}
						}
						free(portIdentifier);
					}
					break;
				}
				case VuoTelemetryPublishedOutputPortsUpdated:
				{
					while (hasMoreToReceive(ZMQTelemetry))
					{
						char *portIdentifier = vuoReceiveAndCopyString(ZMQTelemetry, NULL);
						if (hasMoreToReceive(ZMQTelemetry))
						{
							bool sentData = vuoReceiveBool(ZMQTelemetry, NULL);
							if (hasMoreToReceive(ZMQTelemetry))
							{
								string portDataSummary;
								char *s = vuoReceiveAndCopyString(ZMQTelemetry, NULL);
								if (s)
								{
									portDataSummary = s;
									free(s);
								}
								else
									portDataSummary = "";

								Port *port = getPublishedOutputPortWithName(portIdentifier);

								dispatch_sync(delegateQueue, ^{
												  if (delegate)
													  delegate->receivedTelemetryPublishedOutputPortUpdated(port, sentData, portDataSummary);
											  });

								saturating_semaphore_signal(anyPublishedPortEventSemaphore, &anyPublishedPortEventSignaled);
							}
						}
					}
					break;
				}
				case VuoTelemetryEventDropped:
				{
					char *portIdentifier = vuoReceiveAndCopyString(ZMQTelemetry, NULL);
					dispatch_sync(delegateQueue, ^{
									  if (delegate)
										  delegate->receivedTelemetryEventDropped(portIdentifier);
								  });
					free(portIdentifier);
					break;
				}
				case VuoTelemetryError:
				{
					char *message = vuoReceiveAndCopyString(ZMQTelemetry, NULL);
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
			string dir, file, ext;
			VuoFileUtilities::splitPath(executablePath, dir, file, ext);
			stopBecauseLostContact("The connection between the composition ('" + file + "') and runner timed out while listening for telemetry.");
		}
	}

	zmq_close(ZMQTelemetry);
	ZMQTelemetry = NULL;

	zmq_close(ZMQSelfSend);
	ZMQSelfSend = NULL;
	zmq_close(ZMQSelfReceive);
	ZMQSelfReceive = NULL;

	dispatch_semaphore_signal(endedListeningSemaphore);
	return true;
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
	char *error = NULL;
	vuoSend("runner VuoControl",ZMQControl,request,messages,messageCount,false,&error);

	if (error)
	{
		string e(error);
		free(error);
		throw std::runtime_error(e);
	}
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
	char *error = NULL;
	vuoSend("runner VuoLoaderControl",ZMQLoaderControl,request,messages,messageCount,false,&error);

	if (error)
	{
		string e(error);
		free(error);
		throw std::runtime_error(e);
	}
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
	char *error = NULL;
	int reply = vuoReceiveInt(ZMQControl, &error);

	if (error)
	{
		string e(error);
		free(error);
		ostringstream oss;
		oss << e << " (expected " << expectedReply << ")";
		throw std::runtime_error(oss.str());
	}
	else if (reply != expectedReply)
	{
		ostringstream oss;
		oss << "The runner received the wrong message from the composition (expected " << expectedReply << ", received " << reply << ")";
		throw std::runtime_error(oss.str());
	}
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
	char *error = NULL;
	int reply = vuoReceiveInt(ZMQLoaderControl, &error);

	if (error)
	{
		string e(error);
		free(error);
		ostringstream oss;
		oss << e << " (expected " << expectedReply << ")";
		throw std::runtime_error(oss.str());
	}
	else if (reply != expectedReply)
	{
		ostringstream oss;
		oss << "The runner received the wrong message from the composition loader (expected " << expectedReply << ", received " << reply << ")";
		throw std::runtime_error(oss.str());
	}
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
 * Receives a string from the composition.
 *
 * @param fallbackIfNull The string to return in case the C-string received from the composition is null.
 */
string VuoRunner::receiveString(string fallbackIfNull)
{
	char *error = NULL;
	char *s = vuoReceiveAndCopyString(ZMQControl, &error);

	if (error)
	{
		string e(error);
		free(error);
		throw std::runtime_error(e);
	}

	string ret;
	if (s)
	{
		ret = s;
		free(s);
	}
	else
		ret = fallbackIfNull;

	return ret;
}

/**
 * Receives a list of 0 or more strings from the composition.
 */
vector<string> VuoRunner::receiveListOfStrings(void)
{
	vector<string> messageStrings;
	while (hasMoreToReceive(ZMQControl))
	{
		string s = receiveString("");
		messageStrings.push_back(s);
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
 * Notifies the delegate of lost contact, and causes the runner to stop and clean up.
 */
void VuoRunner::stopBecauseLostContact(string errorMessage)
{
	__block bool alreadyLostContact;
	dispatch_sync(delegateQueue, ^{
					  alreadyLostContact = lostContact;
					  lostContact = true;
				  });

	if (alreadyLostContact)
		return;

	dispatch_sync(delegateQueue, ^{
					  if (delegate)
						  delegate->lostContactWithComposition();
				  });

	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
					   stop();
				   });

	if (! isInCurrentProcess())
	{
		// Normally, stop() is responsible for terminating the ZMQ context.
		// But, if stopBecauseLostContact() is called, it takes the responsibility away from stop().
		// If there's an in-progress zmq_recv() call, stop() will get stuck waiting on controlQueue, so
		// the below call to terminate the ZMQ context interrupts zmq_recv() and allows stop() to proceed.
		dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
						   vuoMemoryBarrier();

						   zmq_term(ZMQContext);
						   ZMQContext = NULL;
						   dispatch_semaphore_signal(terminatedZMQContextSemaphore);
					   });
	}

	VUserLog("%s", errorMessage.c_str());
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
 *
 * Keys include:
 *
 *    - `default` — the port's default value
 *    - `menuItems` — an array where each element is an object with 2 keys: `value` (string: identifier) and `name` (string: display name)
 *    - `suggestedMin` — the port's suggested minimum value (for use on UI sliders and spinboxes)
 *    - `suggestedMax` — the port's suggested maximum value (for use on UI sliders and spinboxes)
 *    - `suggestedStep` — the port's suggested step (the amount the value changes with each click of a spinbox)
 *
 * If `menuItems` contains any values, the host application should display a select widget.
 * Otherwise, the host application should use `type` to determine the kind of widget to display.
 * Host applications are encouraged to provide widgets for the following specific `type`s:
 *
 *    - @ref VuoText
 *    - @ref VuoReal
 *    - @ref VuoInteger
 *    - @ref VuoBoolean
 *    - @ref VuoPoint2d
 *    - @ref VuoPoint3d
 *    - @ref VuoImage
 *    - @ref VuoColor
 */
json_object *VuoRunner::Port::getDetails(void)
{
	return details;
}

VuoRunnerDelegate::~VuoRunnerDelegate() { }  // Fixes "undefined symbols" error

/**
 * If true, some nodes may restrict how they can be used.
 *
 * Call this before @ref start.
 */
void VuoRunner::setTrialRestrictions(bool isTrial)
{
	trialRestrictionsEnabled = isTrial;
}

vector<int> VuoRunner::allCompositionWritePipes;

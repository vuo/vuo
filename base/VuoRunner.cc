/**
 * @file
 * VuoRunner implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoRunner.hh"
#include "VuoFileUtilities.hh"
#include "VuoImage.h"
#include "VuoStringUtilities.hh"
#include "VuoEventLoop.h"
#include "VuoException.hh"
#include "VuoRuntime.h"
#include "VuoRunningCompositionLibraries.hh"

#include <CoreServices/CoreServices.h>
#include <pthread.h>
#include <stdio.h>
#include <dlfcn.h>
#include <sstream>
#include <copyfile.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <sys/proc_info.h>
#include <sys/stat.h>

/// @{
/// Logs calls to VuoRunner's public methods.
#ifdef VUO_RUNNER_TRACE
// Since the VuoDefer is declared on the first line of the method,
// log line number 0 instead of the first line,
// to avoid confusion about where the method is being exited.
#define VuoRunnerTraceScope() VUserLog("{"); VuoDefer(^{ VuoLog(VuoLog_moduleName, __FILE__, 0, __func__, "}"); })
#else
#define VuoRunnerTraceScope()
#endif
/// @}

void *VuoApp_mainThread = NULL;	///< A reference to the main thread
static const char *mainThreadChecker = "/Applications/Xcode.app/Contents/Developer/usr/lib/libMainThreadChecker.dylib";  ///< The path to Xcode's libMainThreadChecker.dylib.
static int compositionReadRunnerWritePipe[2];  ///< A pipe used by the runtime to check if the runner process has ended.
static bool VuoRunner_isHostVDMX = false;  ///< True if this VuoRunner instance is running inside VDMX.

/**
 * Tells the specified Unix file descriptor
 * to automatically close itself when `exec()` is called.
 */
static void VuoRunner_closeOnExec(int fd)
{
	int flags = fcntl(fd, F_GETFD);
	if (flags < 0)
	{
		VUserLog("Error: Couldn't get flags for desciptor %d: %s", fd, strerror(errno));
		return;
	}

	flags |= FD_CLOEXEC;

	if (fcntl(fd, F_SETFD, flags) != 0)
		VUserLog("Error: Couldn't set FD_CLOEXEC on descriptor %d: %s", fd, strerror(errno));
}

/**
 * Get a reference to the main thread, so we can perform runtime thread assertions.
 */
static void __attribute__((constructor)) VuoRunner_init()
{
	VuoApp_mainThread = (void *)pthread_self();

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	// Calls _TSGetMainThread().
	// https://b33p.net/kosada/node/12944
	YieldToAnyThread();
#pragma clang diagnostic pop

	pipe(compositionReadRunnerWritePipe);

	// Ensure that the write end of this pipe gets closed upon fork()/exec(),
	// so child processes don't prop open this pipe,
	// which would prevent Vuo compositions from quitting when the VuoRunner process quits.
	VuoRunner_closeOnExec(compositionReadRunnerWritePipe[1]);

	if (VuoStringUtilities::makeFromCFString(CFBundleGetIdentifier(CFBundleGetMainBundle())) == "com.vidvox.VDMX5")
		VuoRunner_isHostVDMX = true;
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
static void VuoRunner_configureSocket(void *zmqSocket)
{
	int linger = 0;  // avoid having zmq_term block if the runner has tried to send a message on a broken connection
	zmq_setsockopt(zmqSocket, ZMQ_LINGER, &linger, sizeof linger);
}

/**
 * Private instance data for VuoRunner.
 */
class VuoRunner::Private
{
public:
	Private() :
		lastWidth(0),
		lastHeight(0)
	{
	}

	once_flag vuoImageFunctionsInitialized;  ///< Ensures we only try to initialize the below functions once.
	typedef void *(*vuoImageMakeFromJsonWithDimensionsType)(json_object *, unsigned int, unsigned int);  ///< VuoImage_makeFromJsonWithDimensions
	vuoImageMakeFromJsonWithDimensionsType vuoImageMakeFromJsonWithDimensions;  ///< VuoImage_makeFromJsonWithDimensions
	typedef json_object *(*vuoImageGetInterprocessJsonType)(void *);  ///< VuoImage_getInterprocessJson
	vuoImageGetInterprocessJsonType vuoImageGetInterprocessJson;  ///< VuoImage_getInterprocessJson

	uint64_t lastWidth;   ///< The most recent image size provided to `setPublishedInputPortValues`.
	uint64_t lastHeight;  ///< The most recent image size provided to `setPublishedInputPortValues`.
};

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
	VuoRunnerTraceScope();

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
 * @param compositionDylibPath A linked composition dynamic library, produced by @ref VuoCompiler::linkCompositionToCreateDynamicLibraries.
 * @param runningCompositionLibraries Information about libraries referenced by the composition, produced and updated by calls to
 *     @ref VuoCompiler::linkCompositionToCreateDynamicLibraries. The runner takes co-ownership of the `VuoRunningCompositionLibraries`
 *     object and will release it when the composition stops.
 * @param sourceDir The directory containing the composition (.vuo) source file, used by nodes in the composition to resolve relative paths.
 * @param continueIfRunnerDies If true, the composition keeps running if the runner process exits without stopping the composition.
 * @param deleteDylibsWhenFinished True if the runner should delete @a compositionDylibPath and the resource dylibs tracked by
 *     @a runningCompositionLibraries when it's finished using the files.
 * @version200Changed{Added `runningCompositionLibraries` argument; removed `resourceDylibPath` argument.}
 */
VuoRunner * VuoRunner::newSeparateProcessRunnerFromDynamicLibrary(string compositionLoaderPath, string compositionDylibPath,
																  const std::shared_ptr<VuoRunningCompositionLibraries> &runningCompositionLibraries,
																  string sourceDir, bool continueIfRunnerDies, bool deleteDylibsWhenFinished)
{
	VuoRunnerTraceScope();

	VuoRunner * vr = new VuoRunner();
	vr->executablePath = compositionLoaderPath;
	vr->dylibPath = compositionDylibPath;
	vr->dependencyLibraries = runningCompositionLibraries;
	vr->sourceDir = sourceDir;
	vr->shouldContinueIfRunnerDies = continueIfRunnerDies;
	vr->shouldDeleteBinariesWhenFinished = deleteDylibsWhenFinished;
	runningCompositionLibraries->setDeleteResourceLibraries(deleteDylibsWhenFinished);
	return vr;
}

/**
 * Creates a runner object that can run a composition in the current process.
 *
 * @param dylibPath A linked composition dynamic library, produced by VuoCompiler::linkCompositionToCreateDynamicLibrary().
 * @param sourceDir The directory containing the composition (.vuo) source file, used by nodes in the composition to resolve relative paths.
 * @param deleteDylibWhenFinished True if the runner should delete @c dylibPath when it's finished using the file.
 *
 * @see CompileAndRunInCurrentProcess.cc
 */
VuoRunner * VuoRunner::newCurrentProcessRunnerFromDynamicLibrary(string dylibPath, string sourceDir,
																 bool deleteDylibWhenFinished)
{
	VuoRunnerTraceScope();

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
	VuoRunnerTraceScope();

	dispatch_release(stoppedSemaphore);
	dispatch_release(terminatedZMQContextSemaphore);
	dispatch_release(beganListeningSemaphore);
	dispatch_release(endedListeningSemaphore);
	dispatch_release(lastFiredEventSemaphore);
	dispatch_release(delegateQueue);
	delete p;
}

/**
 * When enabled, additional runtime checks are performed on the target composition.
 * This can identify issues for node developers to address, but it may reduce performance.
 */
void VuoRunner::setRuntimeChecking(bool runtimeCheckingEnabled)
{
	VuoRunnerTraceScope();

	if (!stopped)
	{
		VUserLog("Error: Only call VuoRunner::setRuntimeChecking() prior to starting the composition.");
		return;
	}

	isRuntimeCheckingEnabled = runtimeCheckingEnabled && VuoFileUtilities::fileExists(mainThreadChecker);
}

/**
 * Private constructor, used by factory methods.
 */
VuoRunner::VuoRunner(void)
{
	p = new Private;
	dylibHandle = NULL;
	dependencyLibraries = NULL;
	shouldContinueIfRunnerDies = false;
	shouldDeleteBinariesWhenFinished = false;
	isRuntimeCheckingEnabled         = false;
	paused = true;
	stopped = true;
	lostContact = false;
	listenCanceled = false;
	stoppedSemaphore = dispatch_semaphore_create(1);
	terminatedZMQContextSemaphore = dispatch_semaphore_create(0);
	beganListeningSemaphore = dispatch_semaphore_create(0);
	endedListeningSemaphore = dispatch_semaphore_create(1);
	lastFiredEventSemaphore = dispatch_semaphore_create(0);
	lastFiredEventSignaled = false;
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

	static once_flag sleepHandlersInstalled;
	call_once(sleepHandlersInstalled, [](){
		VuoEventLoop_installSleepHandlers();
	});
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
	VuoRunnerTraceScope();

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
	catch (VuoException &e)
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
	VuoRunnerTraceScope();

	try
	{
		startInternal();
	}
	catch (VuoException &e)
	{
		stopBecauseLostContact(e.what());
	}
}

/**
 * Copies `dylibPath` to `newDylibPath`,
 * and changes its internal ID (`LC_ID_DYLIB`) to match its new filename.
 */
void VuoRunner::copyDylibAndChangeId(string dylibPath, string &outputDylibPath)
{
	string directory, file, extension;
	VuoFileUtilities::splitPath(dylibPath, directory, file, extension);

	const int makeTmpFileExtension = 7;
	if (file.length() > makeTmpFileExtension)
	{
		// makeTmpFile() appends "-XXXXXX"; make room for that.
		string trimmedFile = file.substr(0, file.length() - makeTmpFileExtension);

		bool alreadyLoaded;
		do
		{
			outputDylibPath = VuoFileUtilities::makeTmpFile(trimmedFile, "dylib");
			alreadyLoaded = dlopen(outputDylibPath.c_str(), RTLD_NOLOAD);
		} while (alreadyLoaded);
	}
	else
	{
		// For short names, like those generated by VDMX, just replace the entire name with a hash.
		// https://b33p.net/kosada/node/12917
		bool alreadyLoaded;
		do
		{
			string hash = VuoStringUtilities::makeRandomHash(file.length());
			outputDylibPath = "/tmp/" + hash + ".dylib";
			alreadyLoaded = dlopen(outputDylibPath.c_str(), RTLD_NOLOAD);
		} while (alreadyLoaded);
	}

	string newDirectory, newFile, newExtension;
	VuoFileUtilities::splitPath(outputDylibPath, newDirectory, newFile, newExtension);

	if (newFile.length() > file.length())
		throw VuoException("The composition couldn't start because the uniqued dylib name (" + newFile + ") is longer than the original dylib name (" + file + ").");

	if (copyfile(dylibPath.c_str(), outputDylibPath.c_str(), NULL, COPYFILE_ALL))
		throw VuoException("The composition couldn't start because a copy of the dylib couldn't be made.");

	FILE *fp = fopen(outputDylibPath.c_str(), "r+b");
	if (!fp)
		throw VuoException("The composition couldn't start because the dylib's header couldn't be opened.");

	__block bool fpClosed = false;
	VuoDefer(^{ if (! fpClosed) fclose(fp); });

	struct mach_header_64 header;
	if (fread(&header, sizeof(header), 1, fp) != 1)
		throw VuoException("The composition couldn't start because the dylib's header couldn't be read.");

	if (header.magic != MH_MAGIC_64)
		throw VuoException("The composition couldn't start because the dylib isn't a 64-bit Mach-O binary.");

	for (int i = 0; i < header.ncmds; ++i)
	{
		struct load_command lc;
		if (fread(&lc, sizeof(lc), 1, fp) != 1)
			throw VuoException("The composition couldn't start because the dylib's command couldn't be read.");

		// VLog("cmd[%d]: %x (size %d)",i,lc.cmd,lc.cmdsize);
		if (lc.cmd == LC_ID_DYLIB)
		{
			fseek(fp, sizeof(struct dylib), SEEK_CUR);

			size_t nameLength = lc.cmdsize - sizeof(struct dylib_command);
			char *name = (char *)calloc(nameLength + 1, 1);
			if (fread(name, nameLength, 1, fp) != 1)
				throw VuoException("The composition couldn't start because the dylib's ID command couldn't be read.");

//			VLog("Changing name \"%s\" to \"%s\"…", name, outputDylibPath.c_str());
			fseek(fp, -nameLength, SEEK_CUR);
			bzero(name, nameLength);
			memcpy(name, outputDylibPath.c_str(), min(nameLength, outputDylibPath.length()));
			fwrite(name, nameLength, 1, fp);

			fclose(fp);
			fpClosed = true;

			// Since VuoRunner doesn't have access to VuoCompiler,
			// we can't simply call VuoCompiler::getCodesignAllocatePath().
			// Hopefully that method will already have been called once on the system,
			// in order to generate the cache.
			vector<string> environment;
			string codesignAllocatePath = VuoFileUtilities::getCachePath() + "/codesign_allocate";
			if (VuoFileUtilities::fileExists(codesignAllocatePath))
				environment = { "CODESIGN_ALLOCATE=" + codesignAllocatePath };

			try
			{
				VuoFileUtilities::adHocCodeSign(outputDylibPath, environment);
			}
			catch (std::exception &e)
			{
				VUserLog("Warning: Couldn't code-sign the renamed dylib: %s", e.what());
			}

			return;
		}
		else
			fseek(fp, lc.cmdsize-sizeof(lc), SEEK_CUR);
	}

	throw VuoException("The composition couldn't start because the dylib's LC_ID_DYLIB command couldn't be found.");
}

/**
 * Returns the number of bytes the specified Mach-O binary occupies in virtual memory.
 */
int64_t VuoRunner_getDylibVMSize(const struct mach_header_64 *header)
{
	if (header->magic != MH_MAGIC_64)
		return 0;

	struct load_command *lc = (struct load_command *)((char *)header + sizeof(struct mach_header_64));
	int64_t maxExtent = 0;
	for (int i = 0; i < header->ncmds; ++i)
	{
		if (lc->cmd == LC_SEGMENT_64)
		{
			struct segment_command_64 *seg = (struct segment_command_64 *)lc;
			maxExtent = MAX(maxExtent, seg->vmaddr + seg->vmsize);
		}
		else if (lc->cmd == LC_CODE_SIGNATURE
			  || lc->cmd == LC_SEGMENT_SPLIT_INFO
			  || lc->cmd == LC_FUNCTION_STARTS
			  || lc->cmd == LC_DATA_IN_CODE
			  || lc->cmd == LC_DYLIB_CODE_SIGN_DRS
			  || lc->cmd == LC_LINKER_OPTIMIZATION_HINT
			  || lc->cmd == LC_DYLD_EXPORTS_TRIE
			  || lc->cmd == LC_DYLD_CHAINED_FIXUPS)
		{
			struct linkedit_data_command *data = (struct linkedit_data_command *)lc;
			maxExtent = MAX(maxExtent, data->dataoff + data->datasize);
		}
		else if (lc->cmd == LC_SYMTAB)
		{
			struct symtab_command *symtab = (struct symtab_command *)lc;
			maxExtent = MAX(maxExtent, symtab->symoff + symtab->nsyms * sizeof(struct nlist_64));
			maxExtent = MAX(maxExtent, symtab->stroff + symtab->strsize);
		}
		// AFAIK other load commands don't affect VM size.

		lc = (struct load_command *)((char *)lc + lc->cmdsize);
	}
	return maxExtent;
}

static bool VuoRunner_ignoreInitialDylibs;  ///< Don't log info about dylibs that were loaded before the first Vuo composition.

/**
 * Logs info about a dylib that was loaded or unloaded.
 */
static void VuoRunner_logDylibInfo(const struct mach_header_64 *mh, intptr_t vmaddr_slide, const char *func)
{
	if (VuoRunner_ignoreInitialDylibs)
		return;

	// Ignore system libraries.
	if (mh->flags & MH_DYLIB_IN_CACHE)
		return;

	const char *homeZ = getenv("HOME");
	string home;
	if (homeZ)
		home = homeZ;

	Dl_info info{"", nullptr, "", nullptr};
	dladdr((void *)vmaddr_slide, &info);
	string filename{info.dli_fname};
	if (VuoStringUtilities::beginsWith(filename, home))
		filename = "~" + VuoStringUtilities::substrAfter(filename, home);

	int64_t size = VuoRunner_getDylibVMSize(mh);

	VuoLog(VuoLog_moduleName, __FILE__, __LINE__, func, "%16lx - %16lx (%lld bytes) %s", vmaddr_slide, (intptr_t)((char *)vmaddr_slide + size), size, filename.c_str());
}

/**
 * Logs info about a dylib that was loaded.
 */
void VuoRunner_dylibLoaded(const struct mach_header *mh, intptr_t vmaddr_slide)
{
	VuoRunner_logDylibInfo((struct mach_header_64 *)mh, vmaddr_slide, __func__);
}

/**
 * Logs info about a dylib that was unloaded.
 */
void VuoRunner_dylibUnloaded(const struct mach_header *mh, intptr_t vmaddr_slide)
{
	VuoRunner_logDylibInfo((struct mach_header_64 *)mh, vmaddr_slide, __func__);
}

/**
 * Internal helper method for starting the composition running.
 *
 * Assumes the composition is not already running.
 *
 * Upon return, the composition has been started in a paused state.
 *
 * @throw VuoException The composition couldn't start or the connection between the runner and the composition couldn't be established.
 */
void VuoRunner::startInternal(void)
{
	stopped = false;
	dispatch_semaphore_wait(stoppedSemaphore, DISPATCH_TIME_FOREVER);

	ZMQContext = zmq_init(1);

	if (isInCurrentProcess())
	{
		// Start the composition in the current process.

		static once_flag dylibLoggerInitialized;
		call_once(dylibLoggerInitialized, [](){
			// Start logging info about the current process's dylibs, to assist in debugging.
			VuoRunner_ignoreInitialDylibs = true;
			_dyld_register_func_for_add_image(VuoRunner_dylibLoaded);
			_dyld_register_func_for_remove_image(VuoRunner_dylibUnloaded);
			VuoRunner_ignoreInitialDylibs = false;
		});

		bool alreadyLoaded = dlopen(dylibPath.c_str(), RTLD_NOLOAD);
		if (alreadyLoaded)
		{
			// Each composition instance needs its own global variables.
			// Change the dylib's internal name, to convince dlopen() to load another instance of it.

			string uniquedDylibPath;
			copyDylibAndChangeId(dylibPath, uniquedDylibPath);
			VDebugLog("\"%s\" is already loaded, so I duplicated it and changed its LC_ID_DYLIB to \"%s\".", dylibPath.c_str(), uniquedDylibPath.c_str());

			if (shouldDeleteBinariesWhenFinished)
				remove(dylibPath.c_str());

			dylibPath = uniquedDylibPath;
			shouldDeleteBinariesWhenFinished = true;
		}

		dylibHandle = dlopen(dylibPath.c_str(), RTLD_NOW);
		if (!dylibHandle)
			throw VuoException("The composition couldn't start because the library '" + dylibPath + "' couldn't be loaded : " + dlerror());

		try
		{
			VuoInitInProcessType *vuoInitInProcess = (VuoInitInProcessType *)dlsym(dylibHandle, "vuoInitInProcess");
			if (! vuoInitInProcess)
				throw VuoException("The composition couldn't start because vuoInitInProcess() couldn't be found in '" + dylibPath + "' : " + dlerror());

			ZMQControlURL = "inproc://" + VuoFileUtilities::makeTmpFile("vuo-control", "");
			ZMQTelemetryURL = "inproc://" + VuoFileUtilities::makeTmpFile("vuo-telemetry", "");

			vuoInitInProcess(ZMQContext, ZMQControlURL.c_str(), ZMQTelemetryURL.c_str(), true, getpid(), -1, false,
							 sourceDir.c_str(), dylibHandle, NULL, false);
		}
		catch (VuoException &e)
		{
			VUserLog("error: %s", e.what());
			dlclose(dylibHandle);
			dylibHandle = NULL;
			throw;
		}
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

		// https://b33p.net/kosada/node/16374
		// The socket's full pathname (`sockaddr_un::sun_path`) must be 104 characters or less
		// (https://opensource.apple.com/source/xnu/xnu-2782.1.97/bsd/sys/un.h.auto.html).
		// "/Users/me/Library/Containers/com.apple.ScreenSaver.Engine.legacyScreenSaver/Data/vuo-telemetry-rr8Br3"
		// is 101 characters, which limits the username to 5 characters.
		// "/Users/me/Library/Containers/com.apple.ScreenSaver.Engine.legacyScreenSaver/Data/v-rr8Br3"
		// is 89 characters, which limits the username to 17 characters
		// (still not a lot, but more likely to work with typical macOS usernames).
		ZMQControlURL = "ipc://" + VuoFileUtilities::makeTmpFile("v", "");
		ZMQTelemetryURL = "ipc://" + VuoFileUtilities::makeTmpFile("v", "");
		args.push_back("--vuo-control=" + ZMQControlURL);
		args.push_back("--vuo-telemetry=" + ZMQTelemetryURL);

		{
			ostringstream oss;
			oss << getpid();
			args.push_back("--vuo-runner-pid=" + oss.str());
		}

		{
			ostringstream oss;
			oss << compositionReadRunnerWritePipe[0];
			args.push_back("--vuo-runner-pipe=" + oss.str());
		}

		if (shouldContinueIfRunnerDies)
			args.push_back("--vuo-continue-if-runner-dies");

		if (isUsingCompositionLoader())
		{
			ZMQLoaderControlURL = "ipc://" + VuoFileUtilities::makeTmpFile("v", "");
			args.push_back("--vuo-loader=" + ZMQLoaderControlURL);
		}
		else
			args.push_back("--vuo-pause");

		int fd[2];
		int ret = pipe(fd);
		if (ret)
			throw VuoException("The composition couldn't start because a pipe couldn't be opened : " + string(strerror(errno)));

		int argSize = args.size();
		char *argv[argSize + 1];
		for (size_t i = 0; i < argSize; ++i)
		{
			size_t mallocSize = args[i].length() + 1;
			argv[i]           = (char *)malloc(mallocSize);
			strlcpy(argv[i], args[i].c_str(), mallocSize);
		}
		argv[argSize] = NULL;

		string errorWorkingDirectory = "The composition couldn't start because the working directory couldn't be changed to '" + sourceDir + "' : ";
		string errorExecutable = "The composition couldn't start because the file '" + executablePath + "' couldn't be executed : ";
		string errorFork = "The composition couldn't start because the composition process couldn't be forked : ";
		const size_t ERROR_BUFFER_LEN = 256;
		char errorBuffer[ERROR_BUFFER_LEN];

		pipe(runnerReadCompositionWritePipe);

		pid_t childPid = fork();
		if (childPid == 0)
		{
			// There are only a limited set of functions you're allowed to call in the child process
			// after fork() and before exec(). Functions such as VUserLog() and exit() aren't allowed,
			// so instead we're calling alternatives such as write() and _exit().

			close(runnerReadCompositionWritePipe[0]);

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

				if (isRuntimeCheckingEnabled)
				{
					const char *vuoRuntimeChecking = getenv("VUO_RUNTIME_CHECKING");
					if (vuoRuntimeChecking)
						setenv("DYLD_INSERT_LIBRARIES", vuoRuntimeChecking, 1);
					else
						setenv("DYLD_INSERT_LIBRARIES", mainThreadChecker, 1);
				}

				ret = execv(executablePath.c_str(), argv);
				if (ret)
				{
					strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
					write(STDERR_FILENO, errorExecutable.c_str(), errorExecutable.length());
					write(STDERR_FILENO, errorBuffer, strlen(errorBuffer));
					write(STDERR_FILENO, "\n", 1);
					for (size_t i = 0; i < argSize; ++i)
						free(argv[i]);
					_exit(-1);
				}
			}
			else if (grandchildPid > 0)
			{
				close(fd[0]);

				int ret = write(fd[1], &grandchildPid, sizeof(pid_t));
				if (ret != sizeof(pid_t))
				{
					strerror_r(errno, errorBuffer, ERROR_BUFFER_LEN);
					write(STDERR_FILENO, errorExecutable.c_str(), errorExecutable.length());
					write(STDERR_FILENO, errorBuffer, strlen(errorBuffer));
					write(STDERR_FILENO, "\n", 1);
				}
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

			// If this process launches compositions in addition to this one,
			// ensure they don't prop open this pipe,
			// which would prevent VuoRunner::stop's `read()` from terminating.
			VuoRunner_closeOnExec(runnerReadCompositionWritePipe[1]);

			for (size_t i = 0; i < argSize; ++i)
				free(argv[i]);

			pid_t grandchildPid = 0;
			int ret = read(fd[0], &grandchildPid, sizeof(pid_t));
			if (ret != sizeof(pid_t))
				throw VuoException("The composition couldn't start because the composition process id couldn't be obtained: " + string(strerror(errno)));
			close(fd[0]);

			// Reap the child process.
			int status;
			do {
				ret = waitpid(childPid, &status, 0);
			} while (ret == -1 && errno == EINTR);
			if (WIFEXITED(status) && WEXITSTATUS(status))
				throw VuoException("The composition couldn't start because the parent of the composition process exited with an error.");
			else if (WIFSIGNALED(status))
				throw VuoException("The composition couldn't start because the parent of the composition process exited abnormally : " + string(strsignal(WTERMSIG(status))));

			if (grandchildPid > 0)
				compositionPid = grandchildPid;
			else
				throw VuoException("The composition couldn't start because the composition process id couldn't be obtained");
		}
		else
		{
			for (size_t i = 0; i < argSize; ++i)
				free(argv[i]);

			throw VuoException("The composition couldn't start because the parent of the composition process couldn't be forked : " + string(strerror(errno)));
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
				throw VuoException("The composition couldn't start because the runner couldn't establish communication with the composition loader : " + string(strerror(errno)));
			usleep(USEC_PER_SEC / 1000);
		}

		// Actually start the composition.
		// Since we don't know how long this will take, the socket has an infinite timeout (https://b33p.net/kosada/vuo/vuo/-/issues/18450).
		replaceComposition(dylibPath, "");
	}
	else
	{
		__block string errorMessage;
		dispatch_sync(controlQueue, ^{
						  try {
							  setUpConnections();
						  } catch (VuoException &e) {
							  errorMessage = e.what();
						  }
					  });
		if (! errorMessage.empty())
			throw VuoException(errorMessage);
	}
}

/**
 * `pthread_create` can't directly invoke a C++ instance method,
 * so this is a C wrapper for it.
 */
void *VuoRunner_listen(void *context)
{
	pthread_detach(pthread_self());
	VuoRunner *runner = static_cast<VuoRunner *>(context);
	runner->listen();
	return NULL;
}

/**
 * Establishes ZMQ connections with the running composition.
 *
 * @throw VuoException The connection between the runner and the composition failed.
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
			throw VuoException("The composition couldn't start because the runner couldn't establish communication to control the composition : " + string(strerror(errno)));
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
						   } catch (VuoException &e) {
							   publishedPortsError = e.what();
						   }
					   });
		while (! (arePublishedInputPortsCached && arePublishedOutputPortsCached) )
		{
			drainMainDispatchQueue();
			usleep(USEC_PER_SEC / 1000);

			if (! publishedPortsError.empty())
				throw VuoException(publishedPortsError);
		}
	}
	else
	{
		getCachedPublishedPorts(false);
		getCachedPublishedPorts(true);
	}

	listenCanceled = false;
	dispatch_semaphore_wait(endedListeningSemaphore, DISPATCH_TIME_FOREVER);

	pthread_t listenThread;
	int ret = pthread_create(&listenThread, nullptr, &VuoRunner_listen, this);
	if (ret)
		throw VuoException(string("The composition couldn't start because the runner couldn't create a thread: ") + strerror(ret));

	dispatch_semaphore_wait(beganListeningSemaphore, DISPATCH_TIME_FOREVER);
	if (!listenError.empty())
		throw VuoException("The composition couldn't start because the runner couldn't establish communication to listen to the composition: " + listenError);

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
 * Throws a @c VuoException if this runner was not constructed to run the composition in the current process
 * or if this function was not called on the main thread.
 *
 * @see drainMainDispatchQueue(), an alternative to this function.
 */
void VuoRunner::runOnMainThread(void)
{
	VuoRunnerTraceScope();

	if (! isInCurrentProcess())
		throw VuoException("The composition is not running in the current process. Only use this function if the composition was constructed with newCurrentProcessRunnerFromDynamicLibrary().");

	if (! isMainThread())
		throw VuoException("This is not the main thread. Only call this function from the main thread.");

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
 * Throws a @c VuoException if this runner was not constructed to run the composition in the current process
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
 * @see runOnMainThread(), an alternative to this function.
 */
void VuoRunner::drainMainDispatchQueue(void)
{
	VuoRunnerTraceScope();

	if (! isInCurrentProcess())
		throw VuoException("The composition is not running in the current process. Only use this function if the composition was constructed with newCurrentProcessRunnerFromDynamicLibrary().");

	if (! isMainThread())
		throw VuoException("This is not the main thread. Only call this function from the main thread.");

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
	VuoRunnerTraceScope();

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
					  catch (VuoException &e)
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
	VuoRunnerTraceScope();

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
					  catch (VuoException &e)
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
 * Assumes the same @ref VuoRunningCompositionLibraries passed to @ref newSeparateProcessRunnerFromDynamicLibrary
 * was passed again through @ref VuoCompiler::linkCompositionToCreateDynamicLibrary when linking the updated
 * version of the composition. If any subcompositions or other node classes are being replaced in this live-coding
 * reload, the @ref VuoRunningCompositionLibraries also needs to be updated with a call to
 * @ref VuoRunningCompositionLibraries::enqueueLibraryContainingDependencyToUnload.
 *
 * Assumes the composition loader has been started and has not been stopped.
 *
 * @param compositionDylibPath A linked composition dynamic library, produced by @ref VuoCompiler::linkCompositionToCreateDynamicLibrary.
 * @param compositionDiff A comparison of the old and new compositions, produced by @ref VuoCompilerCompositionDiff::diff.
 * @version200Changed{Removed `resourceDylibPath` argument.}
 */
void VuoRunner::replaceComposition(string compositionDylibPath, string compositionDiff)
{
	VuoRunnerTraceScope();

	if (! isUsingCompositionLoader())
		throw VuoException("The runner is not using a composition loader. Only use this function if the composition was constructed with newSeparateProcessRunnerFromDynamicLibrary().");

	dispatch_sync(controlQueue, ^{
					  if (stopped || lostContact) {
						  return;
					  }

					  VUserLog("Loading composition…");

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
							  VUserLog("	Pausing…");
							  vuoControlRequestSend(VuoControlRequestCompositionPause,NULL,0);
							  vuoControlReplyReceive(VuoControlReplyCompositionPaused);
						  }

						  cleanUpConnections();

						  vector<string> dependencyDylibPathsRemoved = dependencyLibraries->dequeueLibrariesToUnload();
						  vector<string> dependencyDylibPathsAdded = dependencyLibraries->dequeueLibrariesToLoad();

						  unsigned int messageCount = 4 + dependencyDylibPathsAdded.size() + dependencyDylibPathsRemoved.size();
						  zmq_msg_t *messages = (zmq_msg_t *)malloc(messageCount * sizeof(zmq_msg_t));
						  int index = 0;

						  vuoInitMessageWithString(&messages[index++], dylibPath.c_str());

						  vuoInitMessageWithInt(&messages[index++], dependencyDylibPathsAdded.size());
						  for (vector<string>::iterator i = dependencyDylibPathsAdded.begin(); i != dependencyDylibPathsAdded.end(); ++i) {
							  vuoInitMessageWithString(&messages[index++], (*i).c_str());
						  }

						  vuoInitMessageWithInt(&messages[index++], dependencyDylibPathsRemoved.size());
						  for (vector<string>::iterator i = dependencyDylibPathsRemoved.begin(); i != dependencyDylibPathsRemoved.end(); ++i) {
							  vuoInitMessageWithString(&messages[index++], (*i).c_str());
						  }

						  vuoInitMessageWithString(&messages[index], compositionDiff.c_str());

						  if (! paused)
							  VUserLog("	Replacing composition…");

						  vuoLoaderControlRequestSend(VuoLoaderControlRequestCompositionReplace,messages,messageCount);
						  vuoLoaderControlReplyReceive(VuoLoaderControlReplyCompositionReplaced);

						  setUpConnections();

						  if (! paused)
						  {
							  VUserLog("	Unpausing…");
							  vuoControlRequestSend(VuoControlRequestCompositionUnpause,NULL,0);
							  vuoControlReplyReceive(VuoControlReplyCompositionUnpaused);
						  }

						  VUserLog("	Done.");
					  }
					  catch (VuoException &e)
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
	VuoRunnerTraceScope();

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
							  vuoInitMessageWithBool(&messages[1], false); // isBeingReplaced
							  vuoControlRequestSend(VuoControlRequestCompositionStop, messages, 2);

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
									  catch (...)
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
						  catch (...)
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
							  void *runtimeState = vuoFini();

							  VuoFiniRuntimeStateType *vuoFiniRuntimeState = (VuoFiniRuntimeStateType *)dlsym(dylibHandle, "vuoFiniRuntimeState");
							  if (! vuoFiniRuntimeState)
							  {
								  VUserLog("The composition couldn't stop because vuoFiniRuntimeState() couldn't be found in '%s' : %s", dylibPath.c_str(), dlerror());
								  return;
							  }
							  vuoFiniRuntimeState(runtimeState);
						  }

						  dlclose(dylibHandle);
						  dylibHandle = NULL;
					  }
					  else if (isInCurrentProcess() && !dylibHandle)
					  {
						  // If the dylib isn't open, the composition isn't running, so there's nothing to clean up.
					  }
					  else
					  {
						  char buf[1];
						  close(runnerReadCompositionWritePipe[1]);

						  if (! lostContact)
						  {
							  // Wait for child process to end.
							  // Can't use waitpid() since it only waits on child processes, yet compositionPid is a grandchild.
							  // Instead, do a blocking read() — the grandchild never writes anything to the pipe, and when the grandchild exits,
							  // read() will return EOF (since it was the last process that had it open for writing).
							  read(runnerReadCompositionWritePipe[0], &buf, 1);
						  }

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

					  dependencyLibraries = nullptr;  // release shared_ptr

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

	if (ZMQSelfSend)
		// Break out of zmq_poll().
		vuoSend("VuoRunner::ZMQSelfSend", ZMQSelfSend, 0, nullptr, 0, false, nullptr);

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
	VuoRunnerTraceScope();

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
 * @param compositionIdentifier The runtime identifier for the (sub)composition instance that contains the port.
 *     If the port is in the top-level composition, you can just pass an empty string.
 * @param portIdentifier The compile-time identifier for the port (see VuoCompilerEventPort::getIdentifier()).
 * @param value JSON representation of the port's new value.
 *
 * @see VuoTypes for information about types and their JSON representations.
 * @version200Changed{Added `compositionIdentifier` argument.}
 */
void VuoRunner::setInputPortValue(string compositionIdentifier, string portIdentifier, json_object *value)
{
	VuoRunnerTraceScope();

	const char *valueAsString = json_object_to_json_string_ext(value, JSON_C_TO_STRING_PLAIN);

	dispatch_sync(controlQueue, ^{
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  zmq_msg_t messages[3];
						  vuoInitMessageWithString(&messages[0], compositionIdentifier.c_str());
						  vuoInitMessageWithString(&messages[1], portIdentifier.c_str());
						  vuoInitMessageWithString(&messages[2], valueAsString);
						  vuoControlRequestSend(VuoControlRequestInputPortValueModify, messages, 3);
						  vuoControlReplyReceive(VuoControlReplyInputPortValueModified);
					  }
					  catch (VuoException &e)
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
 * @param compositionIdentifier The runtime identifier for the (sub)composition instance that contains the port.
 *     If the port is in the top-level composition, you can just pass an empty string.
 * @param portIdentifier The compile-time identifier for the port (see VuoCompilerEventPort::getIdentifier()).
 * @version200Changed{Added `compositionIdentifier` argument.}
 */
void VuoRunner::fireTriggerPortEvent(string compositionIdentifier, string portIdentifier)
{
	VuoRunnerTraceScope();

	dispatch_sync(controlQueue, ^{
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  zmq_msg_t messages[2];
						  vuoInitMessageWithString(&messages[0], compositionIdentifier.c_str());
						  vuoInitMessageWithString(&messages[1], portIdentifier.c_str());
						  vuoControlRequestSend(VuoControlRequestTriggerPortFireEvent, messages, 2);
						  vuoControlReplyReceive(VuoControlReplyTriggerPortFiredEvent);
					  }
					  catch (VuoException &e)
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
 * @param compositionIdentifier The runtime identifier for the (sub)composition instance that contains the port.
 *     If the port is in the top-level composition, you can just pass an empty string.
 * @param portIdentifier The compile-time identifier for the port (see VuoCompilerEventPort::getIdentifier()).
 * @return JSON representation of the port's value. When the composition is running in the current process, the JSON
 *     representations for some port data types (e.g. `VuoImage`) contain pointers that are shared between the runner
 *     and the composition. For correct memory management of these pointers, it's important to call `*_makeFromJson()`,
 *     `VuoRetain()`, and `json_object_put()` in the right order — see MyType_makeFromJson().
 *
 * @see VuoTypes for information about types and their JSON representations.
 * @version200Changed{Added `compositionIdentifier` argument.}
 */
json_object * VuoRunner::getInputPortValue(string compositionIdentifier, string portIdentifier)
{
	VuoRunnerTraceScope();

	__block string valueAsString;
	dispatch_sync(controlQueue, ^{
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  zmq_msg_t messages[3];
						  vuoInitMessageWithBool(&messages[0], !isInCurrentProcess());
						  vuoInitMessageWithString(&messages[1], compositionIdentifier.c_str());
						  vuoInitMessageWithString(&messages[2], portIdentifier.c_str());
						  vuoControlRequestSend(VuoControlRequestInputPortValueRetrieve, messages, 3);
						  vuoControlReplyReceive(VuoControlReplyInputPortValueRetrieved);
						  valueAsString = receiveString("null");
					  }
					  catch (VuoException &e)
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
 * @param compositionIdentifier The runtime identifier for the (sub)composition instance that contains the port.
 *     If the port is in the top-level composition, you can just pass an empty string.
 * @param portIdentifier The compile-time identifier for the port (see VuoCompilerEventPort::getIdentifier()).
 * @return JSON representation of the port's value. When the composition is running in the current process, the JSON
 *     representations for some port data types (e.g. `VuoImage`) contain pointers that are shared between the runner
 *     and the composition. For correct memory management of these pointers, it's important to call `*_makeFromJson()`,
 *     `VuoRetain()`, and `json_object_put()` in the right order — see MyType_makeFromJson().
 *
 * @see VuoTypes for information about types and their JSON representations.
 * @version200Changed{Added `compositionIdentifier` argument.}
 */
json_object * VuoRunner::getOutputPortValue(string compositionIdentifier, string portIdentifier)
{
	VuoRunnerTraceScope();

	__block string valueAsString;
	dispatch_sync(controlQueue, ^{
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  zmq_msg_t messages[3];
						  vuoInitMessageWithBool(&messages[0], !isInCurrentProcess());
						  vuoInitMessageWithString(&messages[1], compositionIdentifier.c_str());
						  vuoInitMessageWithString(&messages[2], portIdentifier.c_str());
						  vuoControlRequestSend(VuoControlRequestOutputPortValueRetrieve, messages, 3);
						  vuoControlReplyReceive(VuoControlReplyOutputPortValueRetrieved);
						  valueAsString = receiveString("null");
					  }
					  catch (VuoException &e)
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
 * @param compositionIdentifier The runtime identifier for the (sub)composition instance that contains the port.
 *     If the port is in the top-level composition, you can just pass an empty string.
 * @param portIdentifier The compile-time identifier for the port (see VuoCompilerEventPort::getIdentifier()).
 * @return A brief description of the port's value.
 *
 * @see VuoTypes for information about types and their summaries.
 * @version200Changed{Added `compositionIdentifier` argument.}
 */
string VuoRunner::getInputPortSummary(string compositionIdentifier, string portIdentifier)
{
	VuoRunnerTraceScope();

	__block string summary;
	dispatch_sync(controlQueue, ^{
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  zmq_msg_t messages[2];
						  vuoInitMessageWithString(&messages[0], compositionIdentifier.c_str());
						  vuoInitMessageWithString(&messages[1], portIdentifier.c_str());
						  vuoControlRequestSend(VuoControlRequestInputPortSummaryRetrieve, messages, 2);
						  vuoControlReplyReceive(VuoControlReplyInputPortSummaryRetrieved);
						  summary = receiveString("");
					  }
					  catch (VuoException &e)
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
 * @param compositionIdentifier The runtime identifier for the (sub)composition instance that contains the port.
 *     If the port is in the top-level composition, you can just pass an empty string.
 * @param portIdentifier The compile-time identifier for the port (see VuoCompilerEventPort::getIdentifier()).
 * @return A brief description of the port's value.
 *
 * @see VuoTypes for information about types and their summaries.
 * @version200Changed{Added `compositionIdentifier` argument.}
 */
string VuoRunner::getOutputPortSummary(string compositionIdentifier, string portIdentifier)
{
	VuoRunnerTraceScope();

	__block string summary;
	dispatch_sync(controlQueue, ^{
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  zmq_msg_t messages[2];
						  vuoInitMessageWithString(&messages[0], compositionIdentifier.c_str());
						  vuoInitMessageWithString(&messages[1], portIdentifier.c_str());
						  vuoControlRequestSend(VuoControlRequestOutputPortSummaryRetrieve, messages, 2);
						  vuoControlReplyReceive(VuoControlReplyOutputPortSummaryRetrieved);
						  summary = receiveString("");
					  }
					  catch (VuoException &e)
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
 * @param compositionIdentifier The runtime identifier for the (sub)composition instance that contains the port.
 *     If the port is in the top-level composition, you can just pass an empty string.
 * @param portIdentifier The compile-time identifier for the port (see VuoCompilerEventPort::getIdentifier()).
 * @return A brief description of the port's value.
 *
 * @see VuoTypes for information about types and their summaries.
 * @version200Changed{Added `compositionIdentifier` argument.}
 */
string VuoRunner::subscribeToInputPortTelemetry(string compositionIdentifier, string portIdentifier)
{
	VuoRunnerTraceScope();

	__block string summary;
	dispatch_sync(controlQueue, ^{
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  zmq_msg_t messages[2];
						  vuoInitMessageWithString(&messages[0], compositionIdentifier.c_str());
						  vuoInitMessageWithString(&messages[1], portIdentifier.c_str());
						  vuoControlRequestSend(VuoControlRequestInputPortTelemetrySubscribe, messages, 2);
						  vuoControlReplyReceive(VuoControlReplyInputPortTelemetrySubscribed);
						  summary = receiveString("");
					  }
					  catch (VuoException &e)
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
 * @param compositionIdentifier The runtime identifier for the (sub)composition instance that contains the port.
 *     If the port is in the top-level composition, you can just pass an empty string.
 * @param portIdentifier The compile-time identifier for the port (see VuoCompilerEventPort::getIdentifier()).
 * @return A brief description of the port's value.
 *
 * @see VuoTypes for information about types and their summaries.
 * @version200Changed{Added `compositionIdentifier` argument.}
 */
string VuoRunner::subscribeToOutputPortTelemetry(string compositionIdentifier, string portIdentifier)
{
	VuoRunnerTraceScope();

	__block string summary;
	dispatch_sync(controlQueue, ^{
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  zmq_msg_t messages[2];
						  vuoInitMessageWithString(&messages[0], compositionIdentifier.c_str());
						  vuoInitMessageWithString(&messages[1], portIdentifier.c_str());
						  vuoControlRequestSend(VuoControlRequestOutputPortTelemetrySubscribe, messages, 2);
						  vuoControlReplyReceive(VuoControlReplyOutputPortTelemetrySubscribed);
						  summary = receiveString("");
					  }
					  catch (VuoException &e)
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
 * @param compositionIdentifier The runtime identifier for the (sub)composition instance that contains the port.
 *     If the port is in the top-level composition, you can just pass an empty string.
 * @param portIdentifier The compile-time identifier for the port (see VuoCompilerEventPort::getIdentifier()).
 * @version200Changed{Added `compositionIdentifier` argument.}
 */
void VuoRunner::unsubscribeFromInputPortTelemetry(string compositionIdentifier, string portIdentifier)
{
	VuoRunnerTraceScope();

	dispatch_sync(controlQueue, ^{
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  zmq_msg_t messages[2];
						  vuoInitMessageWithString(&messages[0], compositionIdentifier.c_str());
						  vuoInitMessageWithString(&messages[1], portIdentifier.c_str());
						  vuoControlRequestSend(VuoControlRequestInputPortTelemetryUnsubscribe, messages, 2);
						  vuoControlReplyReceive(VuoControlReplyInputPortTelemetryUnsubscribed);
					  }
					  catch (VuoException &e)
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
 * @param compositionIdentifier The runtime identifier for the (sub)composition instance that contains the port.
 *     If the port is in the top-level composition, you can just pass an empty string.
 * @param portIdentifier The compile-time identifier for the port (see VuoCompilerEventPort::getIdentifier()).
 * @version200Changed{Added `compositionIdentifier` argument.}
 */
void VuoRunner::unsubscribeFromOutputPortTelemetry(string compositionIdentifier, string portIdentifier)
{
	VuoRunnerTraceScope();

	dispatch_sync(controlQueue, ^{
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  zmq_msg_t messages[2];
						  vuoInitMessageWithString(&messages[0], compositionIdentifier.c_str());
						  vuoInitMessageWithString(&messages[1], portIdentifier.c_str());
						  vuoControlRequestSend(VuoControlRequestOutputPortTelemetryUnsubscribe, messages, 2);
						  vuoControlReplyReceive(VuoControlReplyOutputPortTelemetryUnsubscribed);
					  }
					  catch (VuoException &e)
					  {
						  stopBecauseLostContact(e.what());
					  }
				  });
}

/**
 * Sends a control request to the composition telling it to start sending telemetry for all events.
 *
 * Assumes the composition has been started and has not been stopped.
 *
 * @param compositionIdentifier The runtime identifier for the (sub)composition instance whose telemetry is requested.
 *     If the top-level composition, you can just pass an empty string. The request only applies to events at the level
 *     of the requested composition instance, not recursively to subcompositions within it.
 * @version200Changed{Added `compositionIdentifier` argument.}
 */
void VuoRunner::subscribeToEventTelemetry(string compositionIdentifier)
{
	VuoRunnerTraceScope();

	dispatch_sync(controlQueue, ^{
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  zmq_msg_t messages[1];
						  vuoInitMessageWithString(&messages[0], compositionIdentifier.c_str());
						  vuoControlRequestSend(VuoControlRequestEventTelemetrySubscribe, messages, 1);
						  vuoControlReplyReceive(VuoControlReplyEventTelemetrySubscribed);
					  }
					  catch (VuoException &e)
					  {
						  stopBecauseLostContact(e.what());
					  }
				  });
}

/**
 * Sends a control request to the composition telling it to stop sending telemetry for all events.
 * The composition will continue sending any telemetry subscribed by @ref subscribeToInputPortTelemetry,
 * @ref subscribeToOutputPortTelemetry, or @ref subscribeToAllTelemetry.
 *
 * Assumes the composition has been started and has not been stopped.
 *
 * @param compositionIdentifier The runtime identifier for the (sub)composition instance whose telemetry is to be stopped.
 *     If the top-level composition, you can just pass an empty string. The request only applies to events at the level
 *     of the requested composition instance, not recursively to subcompositions within it.
 * @version200Changed{Added `compositionIdentifier` argument.}
 */
void VuoRunner::unsubscribeFromEventTelemetry(string compositionIdentifier)
{
	VuoRunnerTraceScope();

	dispatch_sync(controlQueue, ^{
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  zmq_msg_t messages[1];
						  vuoInitMessageWithString(&messages[0], compositionIdentifier.c_str());
						  vuoControlRequestSend(VuoControlRequestEventTelemetryUnsubscribe, messages, 1);
						  vuoControlReplyReceive(VuoControlReplyEventTelemetryUnsubscribed);
					  }
					  catch (VuoException &e)
					  {
						  stopBecauseLostContact(e.what());
					  }
				  });
}

/**
 * Sends a control request to the composition telling it to start sending all telemetry.
 *
 * Assumes the composition has been started and has not been stopped.
 *
 * @param compositionIdentifier The runtime identifier for the (sub)composition instance whose telemetry is requested.
 *     If the top-level composition, you can just pass an empty string. The request only applies to events at the level
 *     of the requested composition instance, not recursively to subcompositions within it.
 * @version200Changed{Added `compositionIdentifier` argument.}
 */
void VuoRunner::subscribeToAllTelemetry(string compositionIdentifier)
{
	VuoRunnerTraceScope();

	dispatch_sync(controlQueue, ^{
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  zmq_msg_t messages[1];
						  vuoInitMessageWithString(&messages[0], compositionIdentifier.c_str());
						  vuoControlRequestSend(VuoControlRequestAllTelemetrySubscribe, messages, 1);
						  vuoControlReplyReceive(VuoControlReplyAllTelemetrySubscribed);
					  }
					  catch (VuoException &e)
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
 *
 * @param compositionIdentifier The runtime identifier for the (sub)composition instance whose telemetry is to be stopped.
 *     If the top-level composition, you can just pass an empty string. The request only applies to events at the level
 *     of the requested composition instance, not recursively to subcompositions within it.
 * @version200Changed{Added `compositionIdentifier` argument.}
 */
void VuoRunner::unsubscribeFromAllTelemetry(string compositionIdentifier)
{
	VuoRunnerTraceScope();

	dispatch_sync(controlQueue, ^{
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  zmq_msg_t messages[1];
						  vuoInitMessageWithString(&messages[0], compositionIdentifier.c_str());
						  vuoControlRequestSend(VuoControlRequestAllTelemetryUnsubscribe, messages, 1);
						  vuoControlReplyReceive(VuoControlReplyAllTelemetryUnsubscribed);
					  }
					  catch (VuoException &e)
					  {
						  stopBecauseLostContact(e.what());
					  }
				  });
}

/**
 * Sends a control request to the composition telling it to modify the value of one or more published input ports.
 *
 * When you need to set multiple published input port values, it's more efficient to make a single call to
 * this function passing all of the ports and values than a separate call for each port.
 *
 * Assumes the composition has been started and has not been stopped.
 *
 * @param portsAndValuesToSet The JSON representation of each port's new value.
 *
 * @see VuoTypes for information about types and their JSON representations.
 * @version200New
 */
void VuoRunner::setPublishedInputPortValues(map<Port *, json_object *> portsAndValuesToSet)
{
	VuoRunnerTraceScope();

	if (VuoRunner_isHostVDMX)
		for (auto i : portsAndValuesToSet)
		{
			string portName = i.first->getName();
			if (portName == "width")
				p->lastWidth = json_object_get_int64(i.second);
			else if (portName == "height")
				p->lastHeight = json_object_get_int64(i.second);
			else if (portName == "image" || portName == "startImage")
			{
				json_object *o;
				if (json_object_object_get_ex(i.second, "pixelsWide", &o))
					p->lastWidth = json_object_get_int64(o);
				if (json_object_object_get_ex(i.second, "pixelsHigh", &o))
					p->lastHeight = json_object_get_int64(o);
				if (json_object_object_get_ex(i.second, "pointer", &o))
				{
					VuoImage vi = (VuoImage)json_object_get_int64(o);
					p->lastWidth = vi->pixelsWide;
					p->lastHeight = vi->pixelsHigh;
				}
			}
		}

	dispatch_sync(controlQueue, ^{
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  try
					  {
						  int messageCount = portsAndValuesToSet.size() * 2;
						  zmq_msg_t messages[messageCount];

						  int i = 0;
						  for (auto &kv : portsAndValuesToSet)
						  {
							  vuoInitMessageWithString(&messages[i++], kv.first->getName().c_str());
							  vuoInitMessageWithString(&messages[i++], json_object_to_json_string_ext(kv.second, JSON_C_TO_STRING_PLAIN));
						  }

						  vuoControlRequestSend(VuoControlRequestPublishedInputPortValueModify, messages, messageCount);
						  vuoControlReplyReceive(VuoControlReplyPublishedInputPortValueModified);
					  }
					  catch (VuoException &e)
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
	VuoRunnerTraceScope();

	set<VuoRunner::Port *> portAsSet;
	portAsSet.insert(port);
	firePublishedInputPortEvent(portAsSet);
}

/**
 * Sends a control request to the composition telling it to fire an event into all of the given
 * published input ports simultaneously.
 *
 * Upon return, the event will have been fired.
 *
 * Assumes the composition has been started, is not paused, and has not been stopped.
 *
 * @version200Changed{Added `ports` argument.}
 */
void VuoRunner::firePublishedInputPortEvent(const set<Port *> &ports)
{
	VuoRunnerTraceScope();

	dispatch_sync(controlQueue, ^{
					  if (stopped || lostContact) {
						  return;
					  }

					  vuoMemoryBarrier();

					  lastFiredEventSignaled = false;

					  try
					  {
						  size_t messageCount = ports.size() + 1;
						  zmq_msg_t messages[messageCount];

						  vuoInitMessageWithInt(&messages[0], ports.size());
						  int i = 1;
						  for (VuoRunner::Port *port : ports) {
							  vuoInitMessageWithString(&messages[i++], port->getName().c_str());
						  }

						  vuoControlRequestSend(VuoControlRequestPublishedInputPortFireEvent, messages, messageCount);
						  vuoControlReplyReceive(VuoControlReplyPublishedInputPortFiredEvent);
					  }
					  catch (VuoException &e)
					  {
						  stopBecauseLostContact(e.what());
					  }
				  });
}

/**
 * Waits until the event most recently fired by firePublishedInputPortEvent() finishes
 * propagating through the composition.
 *
 * Assumes no further calls are made to firePublishedInputPortEvent() until this function returns.
 *
 * Assumes the composition has been started and has not been stopped.
 *
 * @eg{
 *   map<VuoRunner::Port *, json_object *> portsAndValues;
 *   portsAndValues[inputPort] = inputValue;
 *   portsAndValues[anotherInputPort] = anotherInputValue;
 *
 *   set<VuoRunner::Port *> changedPorts;
 *   changedPorts.insert(inputPort);
 *   changedPorts.insert(anotherInputPort);
 *
 *   runner.setPublishedInputPortValues(portsAndValues);
 *   runner.firePublishedInputPortEvent(changedPorts);
 *   runner.waitForFiredPublishedInputPortEvent();
 *   result = runner.getPublishedOutputPortValue(outputPort);
 * }
 * @version200New
 */
void VuoRunner::waitForFiredPublishedInputPortEvent(void)
{
	VuoRunnerTraceScope();

	saturating_semaphore_wait(lastFiredEventSemaphore, &lastFiredEventSignaled);
}

/**
 * Sends a control request to the composition telling it to retrieve a published input port's value.
 *
 * Assumes the composition has been started and has not been stopped.
 *
 * @param port The published input port.
 * @return JSON representation of the port's value. When the composition is running in the current process, the JSON
 *     representations for some port data types (e.g. `VuoImage`) contain pointers that are shared between the runner
 *     and the composition. For correct memory management of these pointers, it's important to call `*_makeFromJson()`,
 *     `VuoRetain()`, and `json_object_put()` in the right order — see MyType_makeFromJson().
 *
 * @see VuoTypes for information about types and their JSON representations.
 */
json_object * VuoRunner::getPublishedInputPortValue(VuoRunner::Port *port)
{
	VuoRunnerTraceScope();

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
					  catch (VuoException &e)
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
 * @return JSON representation of the port's value. When the composition is running in the current process, the JSON
 *     representations for some port data types (e.g. `VuoImage`) contain pointers that are shared between the runner
 *     and the composition. For correct memory management of these pointers, it's important to call `*_makeFromJson()`,
 *     `VuoRetain()`, and `json_object_put()` in the right order — see MyType_makeFromJson().
 *
 * @see VuoTypes for information about types and their JSON representations.
 */
json_object * VuoRunner::getPublishedOutputPortValue(VuoRunner::Port *port)
{
	VuoRunnerTraceScope();

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
					  catch (VuoException &e)
					  {
						  stopBecauseLostContact(e.what());
					  }
				  });

	// https://b33p.net/kosada/node/17535
	json_object *js = json_tokener_parse(valueAsString.c_str());
	if (VuoRunner_isHostVDMX && port->getName() == "outputImage")
	{
		json_object *o;
		uint64_t actualWidth = 0;
		if (json_object_object_get_ex(js, "pixelsWide", &o))
			actualWidth = json_object_get_int64(o);
		uint64_t actualHeight = 0;
		if (json_object_object_get_ex(js, "pixelsHigh", &o))
			actualHeight = json_object_get_int64(o);
		if (json_object_object_get_ex(js, "pointer", &o))
		{
			VuoImage vi = (VuoImage)json_object_get_int64(o);
			actualWidth = vi->pixelsWide;
			actualHeight = vi->pixelsHigh;
		}

		if (p->lastWidth && p->lastHeight
			&& (actualWidth != p->lastWidth || actualHeight != p->lastHeight))
		{
			call_once(p->vuoImageFunctionsInitialized, [=](){
				p->vuoImageMakeFromJsonWithDimensions = (Private::vuoImageMakeFromJsonWithDimensionsType)dlsym(RTLD_SELF, "VuoImage_makeFromJsonWithDimensions");
				if (!p->vuoImageMakeFromJsonWithDimensions)
				{
					VUserLog("Error: Couldn't find VuoImage_makeFromJsonWithDimensions.");
					return;
				}

				p->vuoImageGetInterprocessJson = (Private::vuoImageGetInterprocessJsonType)dlsym(RTLD_SELF, "VuoImage_getInterprocessJson");
				if (!p->vuoImageGetInterprocessJson)
				{
					VUserLog("Error: Couldn't find VuoImage_getInterprocessJson.");
					return;
				}
			});

			if (p->vuoImageMakeFromJsonWithDimensions && p->vuoImageGetInterprocessJson)
			{
				void *vi = p->vuoImageMakeFromJsonWithDimensions(js, p->lastWidth, p->lastHeight);
				json_object *jsResized = p->vuoImageGetInterprocessJson(vi);
				json_object_put(js);
				return jsResized;
			}
		}
	}

	return js;
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
 * @throw VuoException The connection between the runner and the composition failed.
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
 * @throw VuoException The connection between the runner and the composition failed.
 *		This function imposes a 5-second timeout.
 */
vector<VuoRunner::Port *> VuoRunner::refreshPublishedPorts(bool input)
{
	dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
	dispatch_source_t timeout = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, DISPATCH_TIMER_STRICT, queue);
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
	catch (...)
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
 *
 * @throw VuoException The connection between the runner and the composition failed.
 */
vector<VuoRunner::Port *> VuoRunner::getPublishedInputPorts(void)
{
	VuoRunnerTraceScope();

	return getCachedPublishedPorts(true);
}

/**
 * Returns the list of published output ports in the composition.
 *
 * This function may either send a control request to the composition or use cached values.
 *
 * Assumes the composition has been started and has not been stopped.
 *
 * @throw VuoException The connection between the runner and the composition failed.
 */
vector<VuoRunner::Port *> VuoRunner::getPublishedOutputPorts(void)
{
	VuoRunnerTraceScope();

	return getCachedPublishedPorts(false);
}

/**
 * Returns the published input port with the given name, or NULL if no such port exists.
 *
 * This function may either send a control request to the composition or use cached values.
 *
 * Assumes the composition has been started and has not been stopped.
 *
 * @throw VuoException The connection between the runner and the composition failed.
 */
VuoRunner::Port * VuoRunner::getPublishedInputPortWithName(string name)
{
	VuoRunnerTraceScope();

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
 *
 * @throw VuoException The connection between the runner and the composition failed.
 */
VuoRunner::Port * VuoRunner::getPublishedOutputPortWithName(string name)
{
	VuoRunnerTraceScope();

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
void VuoRunner::listen()
{
	// Name this thread.
	{
		const char *compositionName = dylibPath.empty() ? executablePath.c_str() : dylibPath.c_str();

		// Trim the path, if present.
		if (const char *lastSlash = strrchr(compositionName, '/'))
			compositionName = lastSlash + 1;

		char threadName[MAXTHREADNAMESIZE];
		snprintf(threadName, MAXTHREADNAMESIZE, "org.vuo.runner.telemetry: %s", compositionName);
		pthread_setname_np(threadName);
	}

	ZMQSelfReceive = zmq_socket(ZMQContext, ZMQ_PAIR);
	VuoRunner_configureSocket(ZMQSelfReceive);
	if (zmq_bind(ZMQSelfReceive, "inproc://vuo-runner-self") != 0)
	{
		listenError = strerror(errno);
		dispatch_semaphore_signal(beganListeningSemaphore);
		return;
	}

	ZMQSelfSend = zmq_socket(ZMQContext, ZMQ_PAIR);
	VuoRunner_configureSocket(ZMQSelfSend);
	if (zmq_connect(ZMQSelfSend, "inproc://vuo-runner-self") != 0)
	{
		listenError = strerror(errno);
		dispatch_semaphore_signal(beganListeningSemaphore);
		return;
	}

	{
		ZMQTelemetry = zmq_socket(ZMQContext,ZMQ_SUB);
		VuoRunner_configureSocket(ZMQTelemetry);
		if(zmq_connect(ZMQTelemetry,ZMQTelemetryURL.c_str()))
		{
			listenError = strerror(errno);
			dispatch_semaphore_signal(beganListeningSemaphore);
			return;
		}

		const int highWaterMark = 0;  // no limit
		if(zmq_setsockopt(ZMQTelemetry,ZMQ_RCVHWM,&highWaterMark,sizeof(highWaterMark)))
		{
			listenError = strerror(errno);
			dispatch_semaphore_signal(beganListeningSemaphore);
			return;
		}
	}

	{
		// subscribe to all types of telemetry
		char type = VuoTelemetryHeartbeat;
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
		type = VuoTelemetryEventFinished;
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

	bool pendingCancel = false;
	while(! listenCanceled)
	{
		zmq_pollitem_t items[]=
		{
			{ZMQTelemetry,0,ZMQ_POLLIN,0},
			{ZMQSelfReceive,0,ZMQ_POLLIN,0},
		};
		int itemCount = 2;

		// Wait 1 second.  If no telemetry was received in that second, we probably lost contact with the composition.
		long timeout = pendingCancel ? 100 : 1000;
		zmq_poll(items,itemCount,timeout);
		if(items[0].revents & ZMQ_POLLIN)
		{
			// Receive telemetry type.
			char type = vuoReceiveInt(ZMQTelemetry, NULL);

			// Receive telemetry arguments and forward to VuoRunnerDelegate.
			switch (type)
			{
				case VuoTelemetryHeartbeat:
				{
					dispatch_sync(delegateQueue, ^{
									  if (delegate)
										  delegate->receivedTelemetryStats(0, 0);
								  });
					break;
				}
				case VuoTelemetryNodeExecutionStarted:
				{
					char *compositionIdentifier = vuoReceiveAndCopyString(ZMQTelemetry, NULL);
					char *nodeIdentifier = vuoReceiveAndCopyString(ZMQTelemetry, NULL);
					dispatch_sync(delegateQueue, ^{
									  if (delegate)
										  delegate->receivedTelemetryNodeExecutionStarted(compositionIdentifier, nodeIdentifier);
								  });
					free(compositionIdentifier);
					free(nodeIdentifier);
					break;
				}
				case VuoTelemetryNodeExecutionFinished:
				{
					char *compositionIdentifier = vuoReceiveAndCopyString(ZMQTelemetry, NULL);
					char *nodeIdentifier = vuoReceiveAndCopyString(ZMQTelemetry, NULL);
					dispatch_sync(delegateQueue, ^{
									  if (delegate)
										  delegate->receivedTelemetryNodeExecutionFinished(compositionIdentifier, nodeIdentifier);
								  });
					free(compositionIdentifier);
					free(nodeIdentifier);
					break;
				}
				case VuoTelemetryInputPortsUpdated:
				{
					while (VuoTelemetry_hasMoreToReceive(ZMQTelemetry))
					{
						char *compositionIdentifier = vuoReceiveAndCopyString(ZMQTelemetry, NULL);
						if (VuoTelemetry_hasMoreToReceive(ZMQTelemetry))
						{
							char *portIdentifier = vuoReceiveAndCopyString(ZMQTelemetry, NULL);
							if (VuoTelemetry_hasMoreToReceive(ZMQTelemetry))
							{
								bool receivedEvent = vuoReceiveBool(ZMQTelemetry, NULL);
								if (VuoTelemetry_hasMoreToReceive(ZMQTelemetry))
								{
									bool receivedData = vuoReceiveBool(ZMQTelemetry, NULL);
									if (VuoTelemetry_hasMoreToReceive(ZMQTelemetry))
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
															  delegate->receivedTelemetryInputPortUpdated(compositionIdentifier, portIdentifier, receivedEvent, receivedData, portDataSummary);
													  });
									}
								}
							}
							free(portIdentifier);
						}
						free(compositionIdentifier);
					}
					break;
				}
				case VuoTelemetryOutputPortsUpdated:
				{
					while (VuoTelemetry_hasMoreToReceive(ZMQTelemetry))
					{
						char *compositionIdentifier = vuoReceiveAndCopyString(ZMQTelemetry, NULL);
						if (VuoTelemetry_hasMoreToReceive(ZMQTelemetry))
						{
							char *portIdentifier = vuoReceiveAndCopyString(ZMQTelemetry, NULL);
							if (VuoTelemetry_hasMoreToReceive(ZMQTelemetry))
							{
								bool sentEvent = vuoReceiveBool(ZMQTelemetry, NULL);
								if (VuoTelemetry_hasMoreToReceive(ZMQTelemetry))
								{
									bool sentData = vuoReceiveBool(ZMQTelemetry, NULL);
									if (VuoTelemetry_hasMoreToReceive(ZMQTelemetry))
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
															  delegate->receivedTelemetryOutputPortUpdated(compositionIdentifier, portIdentifier, sentEvent, sentData, portDataSummary);
													  });
									}
								}
							}
							free(portIdentifier);
						}
						free(compositionIdentifier);
					}
					break;
				}
				case VuoTelemetryPublishedOutputPortsUpdated:
				{
					while (VuoTelemetry_hasMoreToReceive(ZMQTelemetry))
					{
						char *portIdentifier = vuoReceiveAndCopyString(ZMQTelemetry, NULL);
						if (VuoTelemetry_hasMoreToReceive(ZMQTelemetry))
						{
							bool sentData = vuoReceiveBool(ZMQTelemetry, NULL);
							if (VuoTelemetry_hasMoreToReceive(ZMQTelemetry))
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
							}
						}
						free(portIdentifier);
					}
					break;
				}
				case VuoTelemetryEventFinished:
				{
					saturating_semaphore_signal(lastFiredEventSemaphore, &lastFiredEventSignaled);
					break;
				}
				case VuoTelemetryEventDropped:
				{
					char *compositionIdentifier = vuoReceiveAndCopyString(ZMQTelemetry, NULL);
					char *portIdentifier = vuoReceiveAndCopyString(ZMQTelemetry, NULL);
					dispatch_sync(delegateQueue, ^{
									  if (delegate)
										  delegate->receivedTelemetryEventDropped(compositionIdentifier, portIdentifier);
								  });
					free(compositionIdentifier);
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
				default:
					VUserLog("Error: Unknown telemetry message type: %d", type);
					break;
			}
		}
		else if (! listenCanceled)	// Either the 1-second timeout elapsed, or we got a stop-listening message from ZMQSelfSend
		{
			if (items[1].revents & ZMQ_POLLIN)
			{
				// This is a stop-listening message.
				vuoReceiveInt(ZMQSelfReceive, NULL);

				// Drain any remaining telemetry messages.
				pendingCancel = true;
			}

			else if (pendingCancel)
				listenCanceled = true;

			else
			{
				// Timeout.
				// Could happen if the composition crashed, or if the system fell asleep then hibernated (standby mode).
				// If it's a crash we should disconnect; if it's hibernation we should ignore the timeout and try zmq_poll again.
				if (VuoEventLoop_isSystemAsleep())
					VDebugLog("zmq_poll timed out, but system is sleeping so I'll try again.");
				else if (VuoLog_isDebuggerAttached())
					VDebugLog("zmq_poll timed out, but a debugger is attached to the host so I'll try again.");
				else
				{
					listenCanceled = true;
					string dir, file, ext;
					VuoFileUtilities::splitPath(executablePath, dir, file, ext);
					stopBecauseLostContact("The connection between the composition ('" + file + "') and runner timed out while listening for telemetry.");
				}
			}
		}
	}

	zmq_close(ZMQTelemetry);
	ZMQTelemetry = NULL;

	zmq_close(ZMQSelfSend);
	ZMQSelfSend = NULL;
	zmq_close(ZMQSelfReceive);
	ZMQSelfReceive = NULL;

	dispatch_semaphore_signal(endedListeningSemaphore);
	return;
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
		throw VuoException(e);
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
		throw VuoException(e);
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
		throw VuoException(oss.str());
	}
	else if (reply != expectedReply)
	{
		ostringstream oss;
		oss << "The runner received the wrong message from the composition (expected " << expectedReply << ", received " << reply << ")";
		throw VuoException(oss.str());
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
		throw VuoException(oss.str());
	}
	else if (reply != expectedReply)
	{
		ostringstream oss;
		oss << "The runner received the wrong message from the composition loader (expected " << expectedReply << ", received " << reply << ")";
		throw VuoException(oss.str());
	}
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
		throw VuoException(e);
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
	while (VuoTelemetry_hasMoreToReceive(ZMQControl))
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
	if (__sync_bool_compare_and_swap(signaled, false, true))
		dispatch_semaphore_signal(dsema);
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
	VuoRunnerTraceScope();

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
	VuoRunnerTraceScope();

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

	saturating_semaphore_signal(lastFiredEventSemaphore, &lastFiredEventSignaled);

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
 * Returns the Unix process id of the running composition.
 *
 * Only applicable for compositions running in a separate process.
 */
pid_t VuoRunner::getCompositionPid()
{
	VuoRunnerTraceScope();

	return compositionPid;
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
 *    - `menuItems` — an array where each element is one of the following:
 *       - an object with 2 keys: `value` (string or number: identifier) and `name` (string: display name)
 *       - the string `---` — a menu separator line
 *       - any other string — a non-selectable menu label, for labeling multiple sections within the menu
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

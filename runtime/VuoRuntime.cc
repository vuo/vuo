/**
 * @file
 * VuoRuntime implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <getopt.h>
#include <mach-o/dyld.h> // for _NSGetExecutablePath()
#include <libgen.h> // for dirname()
#include <dirent.h>
#include <dlfcn.h>
#include <stdexcept>
#include <sys/mman.h>

#include "VuoRuntime.h"
#include "VuoRuntimeCommunicator.hh"
#include "VuoRuntimePersistentState.hh"
#include "VuoRuntimeState.hh"

extern "C"
{

#ifndef DOXYGEN
bool *VuoTrialRestrictionsEnabled;	///< If true, some nodes may restrict how they can be used.
#endif

static VuoRuntimeState *runtimeState = NULL;  ///< Runtime instance specific to this composition, keeping it separate from any other compositions running in the current process.
void *vuoRuntimeState = NULL;  ///< Casted version of `runtimeState` for use by generated bitcode.

/**
 * Parses command-line arguments, then calls @ref vuoInitInProcess().
 */
void vuoInit(int argc, char **argv)
{
	bool doAppInit = false;
	char *controlURL = NULL;
	char *telemetryURL = NULL;
	bool isPaused = false;
	pid_t runnerPid = 0;
	int runnerPipe = -1;
	bool continueIfRunnerDies = false;
	bool doPrintHelp = false;
	bool doPrintLicenses = false;
	bool trialRestrictionsEnabled = false;

	// parse commandline arguments
	{
		int getoptArgC = 0;
		char **getoptArgV = (char **)malloc(sizeof(char *) * argc);
		for(int i = 0; i < argc; ++i)
		{
			// Don't pass the OS X Process Serial Number argument to getopt, since it can't handle long arguments with a single hyphen.
			if (strncmp(argv[i], "-psn_", 5) == 0)
			{
				// Since we have a process serial number, we can assume this is an exported app being invoked by LaunchServices.
				// Therefore we need to initialize the app (so it shows up in the dock).
				doAppInit = true;
				continue;
			}

			getoptArgV[getoptArgC++] = argv[i];
		}

		static struct option options[] = {
			{"help", no_argument, NULL, 0},
			{"vuo-control", required_argument, NULL, 0},
			{"vuo-telemetry", required_argument, NULL, 0},
			{"vuo-pause", no_argument, NULL, 0},
			{"vuo-loader", required_argument, NULL, 0},
			{"vuo-runner-pipe", required_argument, NULL, 0},
			{"vuo-continue-if-runner-dies", no_argument, NULL, 0},
			{"vuo-licenses", no_argument, NULL, 0},
			{"vuo-trial", no_argument, NULL, 0},
			{"vuo-runner-pid", required_argument, NULL, 0},
			{NULL, no_argument, NULL, 0}
		};
		int optionIndex=-1;
		while((getopt_long(getoptArgC, getoptArgV, "", options, &optionIndex)) != -1)
		{
			switch(optionIndex)
			{
				case 0:  // --help
					doPrintHelp = true;
					break;
				case 1:	 // --vuo-control
					if (controlURL)
						free(controlURL);
					controlURL = (char *)malloc(strlen(optarg) + 1);
					strcpy(controlURL, optarg);
					break;
				case 2:	 // --vuo-telemetry
					if (telemetryURL)
						free(telemetryURL);
					telemetryURL = (char *)malloc(strlen(optarg) + 1);
					strcpy(telemetryURL, optarg);
					break;
				case 3:  // --vuo-pause
					isPaused = true;
					break;
				case 4:  // --vuo-loader (ignored, but added here to avoid "unrecognized option" warning)
					break;
				case 5:  // --vuo-runner-pipe
					runnerPipe = atoi(optarg);
					break;
				case 6:  // --vuo-continue-if-runner-dies
					continueIfRunnerDies = true;
					break;
				case 7:  // --vuo-licenses
					doPrintLicenses = true;
					break;
				case 8:  // --vuo-trial
					trialRestrictionsEnabled = true;
					break;
				case 9:  // --vuo-runner-pid
					runnerPid = atoi(optarg);
					break;
			}
		}
		free(getoptArgV);
	}

	// Get the exported executable path.
	char rawExecutablePath[PATH_MAX+1];
	uint32_t size = sizeof(rawExecutablePath);
	_NSGetExecutablePath(rawExecutablePath, &size);

	if (doPrintHelp)
	{
		printf("Usage: %s [options]\n"
			   "Options:\n"
			   "  --help                                   Display this information.\n"
			   "  --vuo-licenses                           Display license information.\n",
			   argv[0]);

		exit(0);
	}
	else if (doPrintLicenses)
	{
		printf("This composition may include software licensed under the following terms:\n\n");

		char cleanedExecutablePath[PATH_MAX+1];
		realpath(rawExecutablePath, cleanedExecutablePath);

		// Derive the path of the app bundle's "Licenses" directory from its executable path.
		char executableDir[PATH_MAX+1];
		strcpy(executableDir, dirname(cleanedExecutablePath));

		const char *licensesPathFromExecutable = "/../Frameworks/Vuo.framework/Versions/" VUO_VERSION_STRING "/Documentation/Licenses";
		char rawLicensesPath[strlen(executableDir)+strlen(licensesPathFromExecutable)+1];
		strcpy(rawLicensesPath, executableDir);
		strcat(rawLicensesPath, licensesPathFromExecutable);

		char cleanedLicensesPath[PATH_MAX+1];
		realpath(rawLicensesPath, cleanedLicensesPath);

		bool foundLicenses = false;
		if (access(cleanedLicensesPath, 0) == 0)
		{
			DIR *dirp = opendir(cleanedLicensesPath);
			struct dirent *dp;
			while ((dp = readdir(dirp)) != NULL)
			{
				if (dp->d_name[0] == '.')
					continue;

				printf("=== %s =====================================================\n\n",dp->d_name);

				char licensePath[strlen(cleanedLicensesPath) + dp->d_namlen + 2];
				strcpy(licensePath, cleanedLicensesPath);
				strcat(licensePath, "/");
				strcat(licensePath, dp->d_name);

				int fd = open(licensePath, O_RDONLY);
				char data[1024];
				int bytesRead;
				while((bytesRead = read(fd, data, 1024)) > 0)
					write(1, data, bytesRead);
				close(fd);

				printf("\n\n\n");
				foundLicenses = true;
			}
			closedir(dirp);
		}

		if (!foundLicenses)
			printf("(No license information found.)\n");

		free(controlURL);
		free(telemetryURL);
		exit(0);
	}

	void *executableHandle = dlopen(rawExecutablePath, 0);
	if (! executableHandle)
	{
		VUserLog("The composition couldn't be started because a handle to the executable couldn't be obtained : %s", dlerror());
		free(controlURL);
		free(telemetryURL);
		return;
	}

	vuoInitInProcess(NULL, controlURL, telemetryURL, isPaused, runnerPid, runnerPipe, continueIfRunnerDies, trialRestrictionsEnabled, "",
					 executableHandle, NULL, doAppInit);

	dlclose(executableHandle);

	free(controlURL);
	free(telemetryURL);
}

/**
 * Starts a composition running in the current process.
 *
 * @param ZMQContext A ØMQ context shared with the runner (since ØMQ only allows one context per process). If not needed, pass null.
 * @param controlURL The URL to which the runtime should bind the ØMQ socket that it uses to receive control requests and send replies.
 * @param telemetryURL The URL to which the runtime should bind the ØMQ socket that it uses to publish telemetry.
 * @param isPaused If true, the composition starts out paused, and it's up to the runner to unpause it.
 * @param runnerPid The process ID of the runner.
 * @param runnerPipe The file descriptor of the pipe that the runtime should use to check if the runner process has ended.
 *     If not needed, pass -1.
 * @param continueIfRunnerDies If true, the runtime should allow the composition to keep running if @a runnerPipe indicates that the
 *     runner process has ended. If false, the runtime should instead stop the composition.
 * @param trialRestrictionsEnabled If true, the composition should run in free-trial mode.
 * @param workingDirectory The directory that the composition should use to resolve relative paths.
 * @param compositionBinaryHandle The handle of the composition's dynamic library or executable returned by `dlopen()`.
 * @param previousRuntimeState If the composition is restarting for a live-coding reload, pass the value returned by the previous
 *     call to @ref vuoFini(). Otherwise, pass null.
 * @param doAppInit Should we call VuoApp_init()?
 */
void vuoInitInProcess(void *ZMQContext, const char *controlURL, const char *telemetryURL, bool isPaused, pid_t runnerPid,
					  int runnerPipe, bool continueIfRunnerDies, bool trialRestrictionsEnabled, const char *workingDirectory,
					  void *compositionBinaryHandle, void *previousRuntimeState, bool doAppInit)
{
	runtimeState = (VuoRuntimeState *)previousRuntimeState;
	if (! runtimeState)
		runtimeState = new VuoRuntimeState();

	vuoRuntimeState = (void *)runtimeState;

	try
	{
		runtimeState->init(ZMQContext, controlURL, telemetryURL, isPaused, runnerPid, runnerPipe, continueIfRunnerDies,
						   workingDirectory, compositionBinaryHandle);
	}
	catch (std::runtime_error &e)
	{
		VUserLog("%s", e.what());
		return;
	}

	// Set the `VuoTrialRestrictionsEnabled` global, and protect it.
	{
		VuoTrialRestrictionsEnabled = NULL;
		int pagesize = sysconf(_SC_PAGE_SIZE);
		if (pagesize == -1)
		{
//			VLog("Error: Couldn't configure VuoTrialRestrictionsEnabled: %s", strerror(errno));
		}
		else
		{
			VuoTrialRestrictionsEnabled = (bool *)mmap(NULL, pagesize, PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
			if (VuoTrialRestrictionsEnabled == MAP_FAILED)
			{
//				VLog("Error: Couldn't configure VuoTrialRestrictionsEnabled: %s", strerror(errno));
			}
			else
			{
				*VuoTrialRestrictionsEnabled = trialRestrictionsEnabled;
				if (mprotect(VuoTrialRestrictionsEnabled, pagesize, PROT_READ) == -1)
				{
//					VLog("Error: Couldn't configure VuoTrialRestrictionsEnabled: %s", strerror(errno));
				}
			}
		}
	}

	if (doAppInit)
	{
		typedef void (*vuoAppInitType)(void);
		vuoAppInitType vuoAppInit = (vuoAppInitType) dlsym(RTLD_SELF, "VuoApp_init");
		if (vuoAppInit)
			vuoAppInit();
	}

	runtimeState->startComposition();

	// If the composition had a pending call to vuoStopComposition() when it was stopped, call it again.
	if (runtimeState->persistentState->isStopRequested())
		runtimeState->stopCompositionAsOrderedByComposition();
}

/**
 * Cleans up the ØMQ connections. To be called after the composition has stopped.
 *
 * Returns a data structure containing runtime state that should persist across a live-coding reload.
 * If this function is called for a live-coding reload, pass the return value to the next call to @ref vuoInitInProcess().
 * Otherwise, pass it to @ref vuoFiniRuntimeState().
 */
void * vuoFini(void)
{
	runtimeState->fini();
	return (void *)runtimeState;
}

/**
 * Deallocates the return value of @ref vuoFini().
 */
void vuoFiniRuntimeState(void *runtimeState)
{
	delete (VuoRuntimeState *)runtimeState;
}

/**
 * Callback prototype for @ref vuoAddCompositionFiniCallback.
 */
typedef void (*VuoCompositionFiniCallback)(void);

/**
 * Calls @ref VuoRuntimePersistentState::getWorkingDirectory() on the given runtime state if any,
 * otherwise on the current runtime state if any, otherwise returns null.
 */
char * vuoGetWorkingDirectory(VuoCompositionState *compositionState)
{
	VuoRuntimeState *r = (compositionState ? (VuoRuntimeState *)compositionState->runtimeState : runtimeState);
	if (r)
		return r->persistentState->getWorkingDirectory();
	else
		return strdup(VuoRuntimePersistentState::getCurrentWorkingDirectory().c_str());
}

/**
 * Calls @ref VuoRuntimeState::getRunnerPid() on the given runtime state if any,
 * otherwise on the current runtime state if any, otherwise returns -1.
 */
pid_t vuoGetRunnerPid(VuoCompositionState *compositionState)
{
	VuoRuntimeState *r = (compositionState ? (VuoRuntimeState *)compositionState->runtimeState : runtimeState);
	if (r)
		return r->getRunnerPid();
	else
		return -1;
}

/**
 * Calls @ref VuoRuntimeState::stopCompositionAsOrderedByComposition() on the given runtime state if any,
 * otherwise on the current runtime state if any, otherwise does nothing.
 */
void vuoStopComposition(VuoCompositionState *compositionState)
{
	VuoRuntimeState *r = (compositionState ? (VuoRuntimeState *)compositionState->runtimeState : runtimeState);
	if (r)
		r->stopCompositionAsOrderedByComposition();
}

/**
 * Calls @ref VuoRuntimePersistentState::addFiniCallback() on the given runtime state if any,
 * otherwise on the current runtime state if any, otherwise does nothing.
 */
void vuoAddCompositionFiniCallback(VuoCompositionState *compositionState, VuoCompositionFiniCallback fini)
{
	VuoRuntimeState *r = (compositionState ? (VuoRuntimeState *)compositionState->runtimeState : runtimeState);
	if (r)
		r->persistentState->addFiniCallback(fini);
}

/**
 * Calls @ref VuoRuntimeState::disableTermination() on the given runtime state if any,
 * otherwise on the current runtime state if any, otherwise does nothing.
 */
void vuoDisableTermination(VuoCompositionState *compositionState)
{
	VuoRuntimeState *r = (compositionState ? (VuoRuntimeState *)compositionState->runtimeState : runtimeState);
	if (r)
		r->disableTermination();
}

/**
 * Calls @ref VuoRuntimeState::enableTermination() on the given runtime state if any,
 * otherwise on the current runtime state if any, otherwise does nothing.
 */
void vuoEnableTermination(VuoCompositionState *compositionState)
{
	VuoRuntimeState *r = (compositionState ? (VuoRuntimeState *)compositionState->runtimeState : runtimeState);
	if (r)
		r->enableTermination();
}

/**
 * Returns true if the composition has not yet started or if it has stopped.
 *
 * Assumes that just one composition is running in the process.
 */
bool vuoIsCurrentCompositionStopped(void)
{
	return runtimeState->isStopped();
}

}  // extern "C"

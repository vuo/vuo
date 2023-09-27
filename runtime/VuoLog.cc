/**
 * @file
 * VuoLog implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoMacOSSDKWorkaround.h"
#include <asl.h>
#include <assert.h>
#include <cxxabi.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <libgen.h>
#include <mach/mach_time.h>
#include <mach-o/dyld.h>
#include <math.h>
#include <objc/objc-runtime.h>
#include <os/log.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <xlocale.h>

#include <CoreFoundation/CoreFoundation.h>

#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>

#include "VuoLog.h"
#include "VuoHeap.h"
#include "VuoStringUtilities.hh"

/// The existing std::terminate handler before we installed ours.
static std::terminate_handler nextTerminateHandler = nullptr;

dispatch_queue_t VuoLog_utf8LocaleQueue = NULL;    ///< Serializes access to VuoText_utf8Locale.
locale_t VuoLog_utf8Locale = NULL;                 ///< A shared UTF-8 locale object, initialized at startup.

/**
 * Log the timestamp at which std::terminate was called,
 * so we can tell whether the logged exceptions are related to it.
 */
extern "C" [[noreturn]] void VuoTerminateHandler()
{
	if (auto ep = std::current_exception())
		try
		{
			std::rethrow_exception(ep);
		}
		catch (std::exception const &e)
		{
			int status;
			const char *typeName = typeid(e).name();
			char *unmangled = abi::__cxa_demangle(typeName, 0, 0, &status);
			if (status == 0)
				typeName = unmangled;

			VUserLog("Terminating due to uncaught %s: \"%s\"", typeName, e.what());
		}
		catch (...)
		{
			VUserLog("Terminating due to uncaught exception of unknown type");
		}
	else
		// Could still be due to an exception:
		// https://b33p.net/kosada/node/16404
		VUserLog("Terminating because std::terminate was called (no exception data available)");

	nextTerminateHandler();
	abort();
}

/**
 * Returns the number of seconds (including fractional seconds) since an arbitrary start time.
 */
double VuoLogGetTime(void)
{
	static mach_timebase_info_data_t info = {0,0};
	if (info.denom == 0)
	{
		kern_return_t ret = mach_timebase_info(&info);
		if (ret != KERN_SUCCESS)
		{
			VUserLog("Failed to get mach timebase: %d", ret);
			// Default to 1/1, which is correct on at least some machines.
			info.numer = 1;
			info.denom = 1;
		}
	}

	return (double)mach_absolute_time() * info.numer / (info.denom * NSEC_PER_SEC);
}

double VuoLogStartTime;	///< The time when this module was loaded.

const int VuoLogHistoryItems = 20;	///< How many VLog messages to include in crash reports.
char *VuoLogHistory[VuoLogHistoryItems];	///< VLog messages to include in crash reports.
dispatch_queue_t VuoLogHistoryQueue;		///< Serializes access to @ref VuoLogHistory.

#ifdef VUO_PROFILE
#include <map>
#include <string>
/// Profiler times, in seconds.
typedef struct
{
	double total;
	double min;
	double max;
	uint64_t count;
} VuoLogProfileEntry;
typedef std::map<std::string, VuoLogProfileEntry> VuoLogProfileType;	///< A profiler entry: description and times.
static VuoLogProfileType *VuoLogProfile;	///< Keeps track of profiler times.
static dispatch_queue_t VuoLogProfileQueue;			///< Serializes access to @ref VuoLogProfile.
#endif

static pthread_t VuoLog_mainThread       = nullptr;  ///< To later identify which is the main thread.
static const char *VuoLog_executableName = nullptr;  ///< The process's main executable name (as opposed to @ref VuoLog_moduleName, the dylib the log function was called from).

/**
 * Initializes logging and exception handling.
 */
static void __attribute__((constructor)) VuoLog_init(void)
{
	// Store the time at which this module was loaded, for use by @ref VuoLogGetElapsedTime().
	VuoLogStartTime = VuoLogGetTime();

	nextTerminateHandler = std::set_terminate(&VuoTerminateHandler);

	VuoLog_mainThread = pthread_self();

	char executablePath[PATH_MAX + 1];
	uint32_t size = sizeof(executablePath);
	if (!_NSGetExecutablePath(executablePath, &size))
		VuoLog_executableName = basename_r(executablePath, (char *)malloc(size));

	VuoLog_utf8LocaleQueue = dispatch_queue_create("org.vuo.VuoLog.locale.utf8", NULL);
	VuoLog_utf8Locale = newlocale(LC_ALL_MASK, "en_US.UTF-8", NULL);

#ifdef VUO_PROFILE
	VuoLogProfileQueue = dispatch_queue_create("VuoLogProfile", NULL);
	VuoLogProfile = new VuoLogProfileType;
#endif
}

#if defined(VUO_PROFILE) || defined(DOXYGEN)
/**
 * Adds time to the specified profile `name`.
 */
void VuoLog_recordTime(const char *name, double time)
{
	dispatch_sync(VuoLogProfileQueue, ^{
					  VuoLogProfileType::iterator i = VuoLogProfile->find(name);
					  if (i != VuoLogProfile->end())
					  {
						  i->second.total += time;
						  if (time < i->second.min)
							  i->second.min = time;
						  if (time > i->second.max)
							  i->second.max = time;
						  ++i->second.count;
					  }
					  else
						  (*VuoLogProfile)[name] = (VuoLogProfileEntry){time, time, time, 1};
				  });
}

/**
 * Outputs all time profiles.
 */
extern "C" void __attribute__((destructor)) VuoLog_dumpProfile(void)
{
	dispatch_sync(VuoLogProfileQueue, ^{
					  double totalRuntime = VuoLogGetElapsedTime();
					  for (VuoLogProfileType::iterator i = VuoLogProfile->begin(); i != VuoLogProfile->end(); ++i)
						  fprintf(stderr, "%30s   %12.9f s (%7.4f%%)   (min %12.9f, max %12.9f, avg %12.9f, count %lld)\n",
							  i->first.c_str(),
							  i->second.total,
							  i->second.total * 100. / totalRuntime,
							  i->second.min,
							  i->second.max,
							  i->second.total / i->second.count,
							  i->second.count);
				  });
}
#endif

/**
 * Returns the number of seconds (including fractional seconds) since this module was loaded.
 */
double VuoLogGetElapsedTime(void)
{
	return VuoLogGetTime() - VuoLogStartTime;
}

/// Align on 8-byte boundaries
#define VuoCrashReport_alignment __attribute__((aligned(8)))

/**
 * Data to be inserted into macOS crash reports.
 * Via http://alastairs-place.net/blog/2013/01/10/interesting-os-x-crash-report-tidbits/
 *
 * Since the `message` and `message2` fields don't work on macOS 12 and later,
 * use the `signature` and `backtrace` fields instead.
 */
typedef struct {
	unsigned int version	VuoCrashReport_alignment;
	char *message           VuoCrashReport_alignment;  ///< On macOS 11 and earlier, shows up in the crash report's "Application Specific Information" section.
	char *signature         VuoCrashReport_alignment;  ///< On macOS 10.15 through 13 (at least), shows up in the crash report's "Application Specific Signatures" section.
	char *backtrace         VuoCrashReport_alignment;  ///< On macOS 10.15 through 13 (at least), shows up in the crash report's "Application Specific Backtrace" section.
	char *message2          VuoCrashReport_alignment;  ///< On macOS 11 and earlier, shows up in the crash report's "Application Specific Information" section, _above_ `message`.
	void *reserved			VuoCrashReport_alignment;
	void *reserved2			VuoCrashReport_alignment;
} VuoCrashReport_infoType;

/// Data to be inserted into macOS crash reports.
VuoCrashReport_infoType VuoCrashReport __attribute__((section("__DATA,__crash_info"))) = { 4, NULL, NULL, NULL, NULL, NULL, NULL };

void VuoLog_statusF(const char *moduleName, const char *file, const unsigned int linenumber, const char *function, const char *format, ...)
{
	static dispatch_once_t statusInitialized = 0;
	static dispatch_queue_t statusQueue;
	dispatch_once(&statusInitialized, ^{
		statusQueue = dispatch_queue_create("VuoLogStatus", NULL);
	});

	char *message = nullptr;
	if (format)
	{
		va_list args;
		va_start(args, format);
		vasprintf(&message, format, args);
		va_end(args);
	}

	dispatch_sync(statusQueue, ^{
		if (VuoCrashReport.signature)
			free(VuoCrashReport.signature);

		if (format)
			VuoCrashReport.signature = message;
		else
			VuoCrashReport.signature = nullptr;
	});
}

/**
 * Return value of `-[NSProcessInfo operatingSystemVersion]`, from `NSProcessInfo.h`.
 */
struct NSOperatingSystemVersion
{
	long majorVersion;
	long minorVersion;
	long patchVersion;
};

/**
 * Returns the current operating system version.
 */
static NSOperatingSystemVersion VuoLog_getOSVersion(void)
{
	Class NSProcessInfoClass = objc_getClass("NSProcessInfo");
	id processInfo = ((id(*)(id, SEL))objc_msgSend)((id)NSProcessInfoClass, sel_getUid("processInfo"));
	typedef NSOperatingSystemVersion (*operatingSystemVersionType)(id receiver, SEL selector);
#if __x86_64__
	operatingSystemVersionType operatingSystemVersionFunc = (operatingSystemVersionType)objc_msgSend_stret;
#elif __arm64__
	operatingSystemVersionType operatingSystemVersionFunc = (operatingSystemVersionType)objc_msgSend;
#endif
	NSOperatingSystemVersion operatingSystemVersion = operatingSystemVersionFunc(processInfo, sel_getUid("operatingSystemVersion"));
	return operatingSystemVersion;
}

static bool VuoLog_isInstrumentsFrameworkLoaded = false;  ///< True if DVTInstrumentsFoundation.framework is loaded in the current process (i.e., if the process has been launched for debugging with Instruments.app).

/**
 * Helper for @ref VuoLog_isDebuggerAttached.
 */
static void VuoLog_dylibLoaded(const struct mach_header *mh32, intptr_t vmaddr_slide)
{
	if (VuoLog_isInstrumentsFrameworkLoaded)
		return;

	const struct mach_header_64 *mh = reinterpret_cast<const mach_header_64 *>(mh32);

	// Ignore system libraries.
	if (mh->flags & MH_DYLIB_IN_CACHE)
		return;

	// Get the file path of the current dylib.
	Dl_info info{"", nullptr, "", nullptr};
	dladdr((void *)vmaddr_slide, &info);

	// Check whether it's the dylib we're looking for.
	if (strstr(info.dli_fname, "/DVTInstrumentsFoundation.framework/"))
		VuoLog_isInstrumentsFrameworkLoaded = true;
}

/**
 * Returns true if the current process is being debugged
 * (either launched by LLDB/Instruments, or LLDB/Instruments attached after the process launched).
 */
bool VuoLog_isDebuggerAttached(void)
{
	// Detect LLDB.
	{
		// Based on https://developer.apple.com/library/archive/qa/qa1361/_index.html
		int mib[4];
		mib[0] = CTL_KERN;
		mib[1] = KERN_PROC;
		mib[2] = KERN_PROC_PID;
		mib[3] = getpid();

		struct kinfo_proc info;
		info.kp_proc.p_flag = 0;
		size_t size         = sizeof(info);
		if (sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0) == 0
		 && info.kp_proc.p_flag & P_TRACED)
			return true;
	}

	// Detect Instruments.
	{
		static once_flag once;
		call_once(once, []() {
			_dyld_register_func_for_add_image(VuoLog_dylibLoaded);
		});
		if (VuoLog_isInstrumentsFrameworkLoaded)
			return true;
	}

	return false;
}

extern struct mach_header __dso_handle;

void VuoLog(const char *moduleName, const char *file, const unsigned int linenumber, const char *function, const char *format, ...)
{
	if (!VuoHeap_isPointerReadable(moduleName))
	{
		fprintf(stderr, "VuoLog() error: Invalid 'moduleName' argument (%p).  You may need to rebuild the module calling VuoLog().\n", moduleName);
		VuoLog_backtrace();
		return;
	}
	if (!VuoHeap_isPointerReadable(file))
	{
		fprintf(stderr, "VuoLog() error: Invalid 'file' argument (%p).  You may need to rebuild the module calling VuoLog().\n", file);
		VuoLog_backtrace();
		return;
	}
	if (!VuoHeap_isPointerReadable(function))
	{
		fprintf(stderr, "VuoLog() error: Invalid 'function' argument (%p).  You may need to rebuild the module calling VuoLog().\n", function);
		VuoLog_backtrace();
		return;
	}
	if (!VuoHeap_isPointerReadable(format))
	{
		fprintf(stderr, "VuoLog() error: Invalid 'format' argument (%p).  You may need to rebuild the module calling VuoLog().\n", format);
		VuoLog_backtrace();
		return;
	}

	va_list args;

	va_start(args, format);
	int size = vsnprintf(NULL, 0, format, args);
	va_end(args);

	char *formattedString = (char *)malloc(size+1);
	va_start(args, format);
	vsnprintf(formattedString, size+1, format, args);
	va_end(args);

	char *formattedFunction = NULL;

	// This may be a mangled function name of the form `__6+[f g]_block_invoke`.
	// Trim the prefix and suffix, since the line number is sufficient to locate the code within the function+block.
	if (function && function[0] == '_' && function[1] == '_')
	{
		int actualFunctionLength = atoi(function + 2);
		if (actualFunctionLength)
			formattedFunction = strndup(function + 3 + (int)log10(actualFunctionLength), actualFunctionLength);
		else
			formattedFunction = strndup(function + 2, strrchr(function + 2, '_') - (function + 2));

		// We may now be left with a C++-mangled symbol followed by `_block`,
		// such as `_ZN9VuoRunner18replaceCompositionESsSsSs_block`.
		// Extract just the method name (the last length-specified section).
		if (strncmp(formattedFunction, "_ZN", 3) == 0)
		{
			int pos = 3;
			int len, priorLen = 0;
			while ((len = atoi(formattedFunction + pos)))
			{
				pos += 1 + (int)log10(len) + len;
				priorLen = len;
			}
			char *f2 = strndup(formattedFunction + pos - priorLen, priorLen);
			free(formattedFunction);
			formattedFunction = f2;
		}
	}
	else
	{
		const char *blockInvokePos = strstr(function, "_block_invoke");
		if (blockInvokePos)
			formattedFunction = strndup(function, blockInvokePos - function);
	}

	// Add a trailing `()`, unless it's an Objective-C method.
	if (function)
	{
		const char *f = formattedFunction ? formattedFunction : function;
		size_t fLen   = strlen(f);
		if (f[fLen - 1] != ']')
		{
			size_t mallocSize = fLen + 2 + 1;
			char *f2          = (char *)malloc(mallocSize);
			strlcpy(f2, f, mallocSize);
			strlcat(f2, "()", mallocSize);
			if (formattedFunction)
				free(formattedFunction);
			formattedFunction = f2;
		}
	}

	const char *formattedFile = file;

	if (!VuoHeap_isPointerReadable(formattedFile))
		formattedFile = "(unknown)";

	// Trim the path, if present.
	if (const char *lastSlash = strrchr(formattedFile, '/'))
		formattedFile = lastSlash + 1;

	double time = VuoLogGetElapsedTime();

	// If it's been a while since the last log, add a separator.
	static double priorTime = 0;
	const char *separator = "";
	if (priorTime > 0 && time - priorTime > 0.5)
		separator = "\n";
	priorTime = time;

	// Decide on a name for the thread emitting the log message.
	pthread_t thread = pthread_self();
	char threadName[256];
	if (pthread_equal(thread, VuoLog_mainThread))
		strcpy(threadName, "main");
	else
	{
		bzero(threadName, 256);
		int ret = pthread_getname_np(thread, threadName, 256);
		if (!(ret == 0 && strlen(threadName)))
			snprintf(threadName, 256, "%llx", (uint64_t)thread);
	}

	// Use a bright color for the main text.
	const char *mainColor = "\033[97m";
	// Use a medium color for the metadata.
	const char *metadataColor = "\033[38;5;249m";
	// Use a darker color for separators.
	const char *separatorColor = "\033[90m";
	// Try to give each process a unique color, to distinguish the host and the running composition(s).
	// ANSI-256's 6x6x6 color cube begins at index 16 and spans 216 indices.
	// Skip the first 72 indices, which are darker (illegible against a black terminal background).
	int pidColor = getpid() % 144 + 88;
	// Color the main thread the same as the process; choose a unique color for each other thread.
	int threadColor = pthread_equal(thread, VuoLog_mainThread)
		? pidColor
		: ((uint64_t)thread) % 144 + 88;

	fprintf(stderr, "%s%s[%s%8.3fs%s] %s%12.12s%s:%s%-12.12s %s[\033[38;5;%dm%5d%s:\033[38;5;%dm%-8.8s%s]  %s%20.20s%s:%s%-4u  %32.32s  %s%s\033[0m\n",
		separator,
		separatorColor,
		metadataColor,
		time,
		separatorColor,
		metadataColor,
		VuoLog_executableName,
		separatorColor,
		metadataColor,
		moduleName ? moduleName : "Vuo",
		separatorColor,
		pidColor,
		getpid(),
		separatorColor,
		threadColor,
		threadName,
		separatorColor,
		metadataColor,
		formattedFile,
		separatorColor,
		metadataColor,
		linenumber,
		formattedFunction ? formattedFunction : function,
		mainColor,
		formattedString);


	// Also send it to the macOS Console.
	{
		// Can't just call this directly because it relies on Apple-specific compiler extensions that aren't available in Clang 3.2.
		// os_log(OS_LOG_DEFAULT, "...", ...);

		typedef void (*vuoMacOsLogInternalType)(void *dso, void *log, uint8_t type, const char *message, ...);
		typedef void (*vuoMacOsLogImplType)(void *dso, void *log, uint8_t type, const char *format, uint8_t *buf, uint32_t size);
		static vuoMacOsLogInternalType vuoMacOsLogInternal = NULL;
		static vuoMacOsLogImplType vuoMacOsLogImpl = NULL;
		static dispatch_once_t once = 0;
		static bool debuggerAttached = false;
		dispatch_once(&once, ^{
			debuggerAttached = VuoLog_isDebuggerAttached();
			NSOperatingSystemVersion operatingSystemVersion = VuoLog_getOSVersion();
			if (operatingSystemVersion.majorVersion == 10 && operatingSystemVersion.minorVersion == 12)
				// _os_log_impl doesn't work on macOS 10.12.
				vuoMacOsLogInternal = (vuoMacOsLogInternalType)dlsym(RTLD_SELF, "_os_log_internal");
			else if ((operatingSystemVersion.majorVersion == 10 && operatingSystemVersion.minorVersion > 12)
					 || operatingSystemVersion.majorVersion > 10)
				// _os_log_internal doesn't work on macOS 10.13+.
				vuoMacOsLogImpl = (vuoMacOsLogImplType)dlsym(RTLD_SELF, "_os_log_impl");
		});

		if (!debuggerAttached)
		{
			void *log = os_log_create("org.vuo", formattedFile);

			if (vuoMacOsLogInternal)
				vuoMacOsLogInternal(&__dso_handle, log, 0 /*OS_LOG_TYPE_DEFAULT*/, "%{public}41s:%-4u  %{public}s", formattedFunction ? formattedFunction : function, linenumber, formattedString);
			else if (vuoMacOsLogImpl)
			{
				char *formattedForOsLog;
				asprintf(&formattedForOsLog, "%41s:%-4u  %s", formattedFunction ? formattedFunction : function, linenumber, formattedString);

				// https://reviews.llvm.org/rC284990#C2197511NL2613
				uint8_t logFormatDescriptor[12] = {
					2,  // "summary"       = HasNonScalarItems (a C string)
					1,  // "numArgs"       = one C string
					34, // "argDescriptor" = 2 (IsPublic) + 2 (StringKind) << 4
					8,  // "argSize"       = one C string (a 64-bit pointer)
				};
				// …followed by the pointer itself.
				memcpy(logFormatDescriptor + 4, &formattedForOsLog, 8);

				vuoMacOsLogImpl(&__dso_handle, log, 0 /*OS_LOG_TYPE_DEFAULT*/, "%{public}s", logFormatDescriptor, sizeof(logFormatDescriptor));

				free(formattedForOsLog);
			}

			os_release(log);
		}
	}


	static dispatch_once_t historyInitialized = 0;
	dispatch_once(&historyInitialized, ^{
					  for (int i = 0; i < VuoLogHistoryItems; ++i)
						  VuoLogHistory[i] = NULL;
					  VuoLogHistoryQueue = dispatch_queue_create("VuoLogHistory", NULL);
				  });

	dispatch_sync(VuoLogHistoryQueue, ^{
	// Keep the most recent messages in VuoLogHistory.
	{
		char *formattedPrefixedString;
		asprintf(&formattedPrefixedString, "t=%8.4fs %27.27s:%-4u  %41.41s  %s", time, formattedFile, linenumber, formattedFunction ? formattedFunction : function, formattedString);
		free(formattedString);
		if (formattedFunction)
			free(formattedFunction);

		// Find the first open history slot.
		int i;
		for (i = 0; i < VuoLogHistoryItems; ++i)
			if (!VuoLogHistory[i])
				break;

		if (i >= VuoLogHistoryItems)
		{
			// No open slots; expire 1 and rotate the rest.
			free(VuoLogHistory[0]);
			for (int i = 0; i < VuoLogHistoryItems-1; ++i)
				VuoLogHistory[i] = VuoLogHistory[i+1];
			i = VuoLogHistoryItems - 1;
		}

		VuoLogHistory[i] = formattedPrefixedString;
	}

	// Combine VuoLogHistory into a single string for the crash report.
	{
		if (VuoCrashReport.backtrace)
			free(VuoCrashReport.backtrace);

		long size = 0;
		for (int i = 0; i < VuoLogHistoryItems; ++i)
			if (VuoLogHistory[i])
				size += strlen(VuoLogHistory[i]) + 1 /* \n */;
		size += 1 /* null terminator */;

		char *message = (char *)malloc(size);
		strlcpy(message, VuoLogHistory[0], size);
		for (int i = 1; i < VuoLogHistoryItems; ++i)
			if (VuoLogHistory[i])
			{
				strlcat(message, "\n", size);
				strlcat(message, VuoLogHistory[i], size);
			}

		VuoCrashReport.backtrace = message;
	}
	});
}

/**
 * Returns true if debug mode is enabled.
 *
 * Users can enable debug mode by executing `defaults write org.vuo.Editor debug -boolean true`,
 * and can disable it by executing `defaults delete org.vuo.Editor debug`.
 *
 * Nodes should use this to control verbose logging.
 */
bool VuoIsDebugEnabled(void)
{
	static dispatch_once_t checked = 0;
	static bool debug = false;
	dispatch_once(&checked, ^{
					  debug = CFPreferencesGetAppBooleanValue(CFSTR("debug"), CFSTR("org.vuo.Editor"), NULL);
				  });
	return debug;
}

/**
 * Prints the stack backtrace to stderr.
 */
void VuoLog_backtrace(void)
{
	vector<string> backtrace = VuoLog_getBacktrace();

	// Skip the VuoLog_backtrace() function.
	backtrace.erase(backtrace.begin());

	int i = 1;
	for (auto line : backtrace)
		fprintf(stderr, "%3d %s\n", i++, line.c_str());
}

/**
 * Removes all occurrences of a substring.
 *
 * `replacement` must be shorter than the expanded `substringToRemove`.
 */
void VuoLog_replaceString(char *wholeString, const char *substringToRemove, const char *replacement)
{
	dispatch_sync(VuoLog_utf8LocaleQueue, ^{
		size_t replacementLen = strlen(replacement);
		regex_t regex;
		regcomp_l(&regex, substringToRemove, REG_EXTENDED, VuoLog_utf8Locale);
		size_t nmatch = 1;
		regmatch_t pmatch[nmatch];
		while (!regexec(&regex, wholeString, nmatch, pmatch, 0))
		{
			strncpy(wholeString + pmatch[0].rm_so, replacement, replacementLen);
			memmove(wholeString + pmatch[0].rm_so + replacementLen, wholeString + pmatch[0].rm_eo, strlen(wholeString + pmatch[0].rm_eo) + 1);
		}
		regfree(&regex);
	});
}

/**
 * Returns the stack backtrace.
 */
vector<string> VuoLog_getBacktrace(void)
{
	void *array[100];
	size_t size    = backtrace(array, 100);
	char **strings = backtrace_symbols(array, size);
	vector<string> outputStrings;

	// Start at 1 to skip the VuoLog_getBacktrace() function.
	for (size_t i = 1; i < size; i++)
	{
		// Trim off the line number, since callees may want to skip additional stack frames.
		const int lineNumberLen = 4;
		string trimmedLine = strings[i] + lineNumberLen;

		const int libraryLen = 36;
		const int addressLen = 19;
		string symbol = strings[i] + lineNumberLen + libraryLen + addressLen;
		string instructionOffset = symbol.substr(symbol.find(' '));
		symbol = symbol.substr(0, symbol.find(' '));

		int status;
		char *unmangled = abi::__cxa_demangle(symbol.c_str(), nullptr, nullptr, &status);
		if (status == 0)
		{
			// Boil down the C++ standard library's bloviation.
			VuoLog_replaceString(unmangled, "std::__1::", "");
			VuoLog_replaceString(unmangled, "basic_string<char, char_traits<char>, allocator<char> >", "string");
			VuoLog_replaceString(unmangled, ", less<[^>]*>, allocator<pair<[^>]*, [^>]*> > >", ">");  // map<…>
			VuoLog_replaceString(unmangled, ", less<[^>]*>, allocator<[^>]*> >", ">");  // set<…>
			VuoLog_replaceString(unmangled, ", allocator<[^>]*> >", ">");  // vector<…>

			outputStrings.push_back(trimmedLine.substr(0, libraryLen + addressLen) + unmangled + instructionOffset);
			free(unmangled);
		}
		else
			outputStrings.push_back(trimmedLine);
	}

	// Hide uninformative stack frames.
	{
		// If the last stack frame is something like "??? 0x0000000000000010 0x0 + 16",
		// the stack parser's gone off the rails.
		auto last = outputStrings.back();
		if (last.substr(0, 4) == "??? "
		 && last.find(" 0x0 + ") != string::npos)
			outputStrings.pop_back();

		last = outputStrings.back();
		if (last.substr(0, 13) == "libdyld.dylib"
			&& last.find(" start + ") != string::npos)
			outputStrings.pop_back();

		outputStrings.erase(
			remove_if(outputStrings.begin(), outputStrings.end(),
				[](const string &s) {
					return s.find("_dispatch_queue_override_invoke") != string::npos
						|| s.find("_dispatch_root_queue_drain") != string::npos
						|| s.find("_dispatch_worker_thread2") != string::npos
						|| s.find("_dispatch_lane_serial_drain") != string::npos
						|| s.find("_dispatch_lane_barrier_sync_invoke_and_complete") != string::npos
						|| s.find("_dispatch_lane_invoke") != string::npos
						|| s.find("_dispatch_workloop_worker_thread") != string::npos
						|| s.find("_dispatch_continuation_pop") != string::npos
						|| s.find("_dispatch_main_queue_callback_4CF") != string::npos
						|| s.find("__CFRunLoopDoObservers") != string::npos
						|| s.find("__CFRunLoopDoSource0") != string::npos
						|| s.find("__CFRunLoopDoSources0") != string::npos
						|| s.find("__CFRunLoopRun") != string::npos
						|| s.find("__CFRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE0_PERFORM_FUNCTION__") != string::npos
						|| s.find("__CFRUNLOOP_IS_SERVICING_THE_MAIN_DISPATCH_QUEUE__") != string::npos
						|| s.find("__CFNOTIFICATIONCENTER_IS_CALLING_OUT_TO_AN_OBSERVER__") != string::npos
						|| s.find("_CFXRegistrationPost") != string::npos
						|| s.find("_CFXNotification") != string::npos
						|| s.find("_NSWindowSendWindowDidMove") != string::npos
						|| s.find("_NSSendEventToObservers") != string::npos
						|| s.find("NSCarbonMenuImpl") != string::npos
						|| s.find("NSSLMMenuEventHandler") != string::npos
						|| s.find("CopyCarbonUIElementAttributeValue") != string::npos
						|| s.find("NSApplication(NSEvent) _") != string::npos
						|| s.find("NSApplication(NSAppleEventHandling) _") != string::npos
						|| s.find("withWindowOrderingObserverHeuristic") != string::npos
						|| s.find("aeProcessAppleEvent") != string::npos
						|| s.find("dispatchEventAndSendReply") != string::npos
						|| s.find("aeDispatchAppleEvent") != string::npos
						|| s.find("_NSAppleEventManagerGenericHandler") != string::npos
						|| s.find("NSWindow _") != string::npos
						|| s.find("HIServices") != string::npos
						|| s.find("HIToolbox") != string::npos
						|| s.find("RunCurrentEventLoopInMode") != string::npos
						|| s.find("ReceiveNextEventCommon") != string::npos
						|| s.find("_BlockUntilNextEventMatchingListInModeWithFilter") != string::npos
						|| s.find("_DPSNextEvent") != string::npos
						|| s.find("_pthread_wqthread") != string::npos
						|| s.find("qt_plugin_instance") != string::npos
						|| s.find("QWindowSystemInterface::") != string::npos
						|| s.find("QWidgetPrivate::") != string::npos
						|| s.find("QApplicationPrivate::") != string::npos
						|| s.find("QGuiApplicationPrivate::") != string::npos
						|| s.find("QCoreApplication::notifyInternal2") != string::npos
						|| s.find("QCoreApplicationPrivate::") != string::npos;
				}),
			outputStrings.end());
	}

	free(strings);
	return outputStrings;
}

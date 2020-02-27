/**
 * @file
 * VuoLog implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mach/mach_time.h>
#include <unistd.h>
#include <asl.h>
#include <math.h>
#include <dlfcn.h>
#include <objc/objc-runtime.h>
#include <cxxabi.h>
#include <regex.h>

#ifndef __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES
	/// Avoid conflict between Cocoa and LLVM headers.
	#define __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES 0
#endif
#include <CoreFoundation/CoreFoundation.h>

#include <AvailabilityMacros.h>
#if (MAC_OS_X_VERSION_MIN_REQUIRED == MAC_OS_X_VERSION_10_6) || (MAC_OS_X_VERSION_MIN_REQUIRED == MAC_OS_X_VERSION_10_7)
	#include <ApplicationServices/ApplicationServices.h>
#else
	#include <CoreGraphics/CoreGraphics.h>
	#include <CoreText/CoreText.h>
#endif

#include "VuoLog.h"
#include "VuoStringUtilities.hh"

/// The existing std::terminate handler before we installed ours.
static std::terminate_handler nextTerminateHandler = nullptr;

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

/**
 * Stores the time at which this module was loaded, for use by @ref VuoLogGetElapsedTime().
 *
 * Installs our C++ exception handler.
 */
static void __attribute__((constructor)) VuoLog_init(void)
{
	VuoLogStartTime = VuoLogGetTime();

	nextTerminateHandler = std::set_terminate(&VuoTerminateHandler);

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
 */
typedef struct {
	unsigned int version	VuoCrashReport_alignment;
	char *message           VuoCrashReport_alignment;  ///< Shows up in the crash report's "Application Specific Information" section.
	char *signature			VuoCrashReport_alignment;
	char *backtrace			VuoCrashReport_alignment;
	char *message2          VuoCrashReport_alignment;  ///< Shows up in the crash report's "Application Specific Information" section, _above_ `message`.
	void *reserved			VuoCrashReport_alignment;
	void *reserved2			VuoCrashReport_alignment;
} VuoCrashReport_infoType;

/// Data to be inserted into macOS crash reports.
VuoCrashReport_infoType VuoCrashReport __attribute__((section("__DATA,__crash_info"))) = { 4, NULL, NULL, NULL, NULL, NULL, NULL };

void VuoLog_statusF(const char *file, const unsigned int linenumber, const char *function, const char *format, ...)
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
		if (VuoCrashReport.message2)
			free(VuoCrashReport.message2);

		if (format)
			VuoCrashReport.message2 = message;
		else
			VuoCrashReport.message2 = nullptr;
	});
}

/**
 * Returns the minor component of the OS version.
 */
int VuoLog_getOSVersionMinor(void)
{
	Class NSProcessInfoClass = objc_getClass("NSProcessInfo");
	id processInfo = objc_msgSend((id)NSProcessInfoClass, sel_getUid("processInfo"));
	struct NSOperatingSystemVersion
	{
		long majorVersion;
		long minorVersion;
		long patchVersion;
	};
	typedef NSOperatingSystemVersion (*operatingSystemVersionType)(id receiver, SEL selector);
	operatingSystemVersionType operatingSystemVersionFunc = (operatingSystemVersionType)objc_msgSend_stret;
	NSOperatingSystemVersion operatingSystemVersion = operatingSystemVersionFunc(processInfo, sel_getUid("operatingSystemVersion"));
	return operatingSystemVersion.minorVersion;
}

void VuoLog(const char *file, const unsigned int linenumber, const char *function, const char *format, ...)
{
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

	// Trim the path, if present.
	if (const char *lastSlash = strrchr(file, '/'))
		formattedFile = lastSlash + 1;

	double time = VuoLogGetElapsedTime();

	// If it's been a while since the last log, add a separator.
	static double priorTime = 0;
	const char *separator = "";
	if (priorTime > 0 && time - priorTime > 0.5)
		separator = "\n";
	priorTime = time;

	// ANSI-256's 6x6x6 color cube begins at index 16 and spans 216 indices.
	// Skip the first 72 indices, which are darker (illegible against a black terminal background).
	fprintf(stderr, "%s\033[38;5;%dm# pid=%5d  t=%8.4fs %27.27s:%-4u  %41.41s  %s\033[0m\n", separator, getpid()%144+88, getpid(), time, formattedFile, linenumber, formattedFunction ? formattedFunction : function, formattedString);


	// Also send it to the macOS Console.
	{
		// Can't just call this directly because (1) it only exists on 10.12+,
		// and (2) it relies on Apple-specific complier extensions that aren't available in Clang 3.2.
		// os_log(OS_LOG_DEFAULT, "...", ...);

		extern struct mach_header __dso_handle;
		typedef void *(*vuoMacOsLogCreateType)(const char *subsystem, const char *category);
		typedef void (*vuoMacOsLogInternalType)(void *dso, void *log, uint8_t type, const char *message, ...);
		typedef void (*vuoMacOsLogImplType)(void *dso, void *log, uint8_t type, const char *format, uint8_t *buf, uint32_t size);
		static vuoMacOsLogCreateType vuoMacOsLogCreate = NULL;
		static vuoMacOsLogInternalType vuoMacOsLogInternal = NULL;
		static vuoMacOsLogImplType vuoMacOsLogImpl = NULL;
		static dispatch_once_t once = 0;
		dispatch_once(&once, ^{
			vuoMacOsLogCreate = (vuoMacOsLogCreateType)dlsym(RTLD_SELF, "os_log_create");
			if (VuoLog_getOSVersionMinor() == 12) // _os_log_impl doesn't work on macOS 10.12.
				vuoMacOsLogInternal = (vuoMacOsLogInternalType)dlsym(RTLD_SELF, "_os_log_internal");
			else if (VuoLog_getOSVersionMinor() > 12) // _os_log_internal doesn't work on macOS 10.13+.
				vuoMacOsLogImpl = (vuoMacOsLogImplType)dlsym(RTLD_SELF, "_os_log_impl");
		});

		if (vuoMacOsLogCreate)
		{
			void *log = vuoMacOsLogCreate("org.vuo", formattedFile);

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

		else // For Mac OS X 10.11 and prior
		{
			aslmsg msg = asl_new(ASL_TYPE_MSG);
			asl_set(msg, ASL_KEY_READ_UID, "-1");
			asl_log(NULL, msg, ASL_LEVEL_WARNING, "%27s:%-4u  %41s  %s", formattedFile, linenumber, formattedFunction ? formattedFunction : function, formattedString);
			asl_free(msg);
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
		if (VuoCrashReport.message)
			free(VuoCrashReport.message);

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

		VuoCrashReport.message = message;
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
 * Returns a C string description of `variable`, a reference to a CoreFoundation object.
 *
 * The caller is responsible for `free()`ing the returned string.
 */
char *VuoLog_copyCFDescription(const void *variable)
{
	if (!variable)
		return strdup("(null)");

	CFStringRef d = CFCopyDescription(variable);
	CFIndex len = CFStringGetLength(d)+1;
	char *z = (char *)malloc(len);
	CFStringGetCString(d, z, len, kCFStringEncodingUTF8);
	CFRelease(d);
	return z;
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
	size_t replacementLen = strlen(replacement);
	regex_t regex;
	regcomp(&regex, substringToRemove, REG_EXTENDED);
	size_t nmatch = 1;
	regmatch_t pmatch[nmatch];
	while (!regexec(&regex, wholeString, nmatch, pmatch, 0))
	{
		strncpy(wholeString + pmatch[0].rm_so, replacement, replacementLen);
		memmove(wholeString + pmatch[0].rm_so + replacementLen, wholeString + pmatch[0].rm_eo, strlen(wholeString + pmatch[0].rm_eo) + 1);
	}
	regfree(&regex);
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

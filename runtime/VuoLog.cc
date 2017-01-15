/**
 * @file
 * VuoLog implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mach/mach_time.h>
#include <unistd.h>
#include <asl.h>
#include <math.h>

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

#ifdef PROFILE
#include <map>
#include <string>
/// Profiler times, in seconds.
typedef struct
{
	double total;
	double min;
	double max;
} VuoLogProfileEntry;
typedef std::map<std::string, VuoLogProfileEntry> VuoLogProfileType;	///< A profiler entry: description and times.
static VuoLogProfileType *VuoLogProfile;	///< Keeps track of profiler times.
static dispatch_queue_t VuoLogProfileQueue;			///< Serializes access to @ref VuoLogProfile.
#endif

/**
 * Stores the time at which this module was loaded, for use by @ref VuoLogGetElapsedTime().
 */
static void __attribute__((constructor)) VuoLog_init(void)
{
	VuoLogStartTime = VuoLogGetTime();
#ifdef PROFILE
	VuoLogProfileQueue = dispatch_queue_create("VuoLogProfile", NULL);
	VuoLogProfile = new VuoLogProfileType;
#endif
}

#if defined(PROFILE) || defined(DOXYGEN)
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
					  }
					  else
						  (*VuoLogProfile)[name] = (VuoLogProfileEntry){time, time, time};
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
						  fprintf(stderr, "%30s   %12.9f s (%7.4f%%)   (min %12.9f, max %12.9f)\n",
							  i->first.c_str(),
							  i->second.total,
							  i->second.total * 100. / totalRuntime,
							  i->second.min,
							  i->second.max);
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
 * Data to be inserted into OS X crash reports.
 * Via http://alastairs-place.net/blog/2013/01/10/interesting-os-x-crash-report-tidbits/
 */
typedef struct {
	unsigned int version	VuoCrashReport_alignment;
	char *message			VuoCrashReport_alignment;
	char *signature			VuoCrashReport_alignment;
	char *backtrace			VuoCrashReport_alignment;
	char *message2			VuoCrashReport_alignment;
	void *reserved			VuoCrashReport_alignment;
	void *reserved2			VuoCrashReport_alignment;
} VuoCrashReport_infoType;

/// Data to be inserted into OS X crash reports.
VuoCrashReport_infoType VuoCrashReport __attribute__((section("__DATA,__crash_info"))) = { 4, NULL, NULL, NULL, NULL, NULL, NULL };

void VuoLog(const char *file, const unsigned int line, const char *function, const char *format, ...)
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
	if (function[0] == '_' && function[1] == '_')
	{
		int actualFunctionLength = atoi(function + 2);
		if (actualFunctionLength)
			formattedFunction = strndup(function + 3 + (int)log10(actualFunctionLength), actualFunctionLength);
		else
			formattedFunction = strndup(function + 2, strrchr(function + 2, '_') - (function + 2));
	}

	// Add a trailing `()`, unless it's an Objective-C method.
	{
		const char *f = formattedFunction ? formattedFunction : function;
		if (f[strlen(f) - 1] != ']')
		{
			char *f2 = (char *)malloc(strlen(f) + 3);
			strcpy(f2, f);
			strcat(f2, "()");
			if (formattedFunction)
				free(formattedFunction);
			formattedFunction = f2;
		}
	}

	double time = VuoLogGetElapsedTime();

	fprintf(stderr, "\033[38;5;%dm# pid=%d  t=%8.4fs %27s:%-4u  %41s \t%s\033[0m\n", getpid()%212+19, getpid(), time, file, line, formattedFunction ? formattedFunction : function, formattedString);

	aslmsg msg = asl_new(ASL_TYPE_MSG);
	asl_set(msg, ASL_KEY_READ_UID, "-1");
	asl_log(NULL, msg, ASL_LEVEL_WARNING, "%s:%u  %s  %s", file, line, formattedFunction ? formattedFunction : function, formattedString);
	asl_free(msg);

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
		asprintf(&formattedPrefixedString, "t=%8.4fs  %s:%u  %s  %s", time, file, line, formattedFunction ? formattedFunction : function, formattedString);
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
		strcpy(message, VuoLogHistory[0]);
		for (int i = 1; i < VuoLogHistoryItems; ++i)
			if (VuoLogHistory[i])
			{
				strcat(message, "\n");
				strcat(message, VuoLogHistory[i]);
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
	void *array[100];
	size_t size = backtrace(array, 100);
	char **strings = backtrace_symbols(array, size);

	// Start at 1 to skip the VuoLog_backtrace() function.
	for (size_t i = 1; i < size; i++)
	   fprintf(stderr, "%s\n", strings[i]);

	free(strings);
}

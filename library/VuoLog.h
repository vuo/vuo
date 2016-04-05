/**
 * @file
 * Logging functions.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOLOG_H
#define VUOLOG_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <asl.h>

/**
 * @ingroup DevelopingNodeClasses DevelopingTypes DevelopingLibraryModules
 * @defgroup VuoModuleDebug Module Debugging
 * Macros to help with debugging.
 *
 * @{
 */


/// @cond
/**
 * Returns the number of seconds (including fractional seconds) since midnight 1970.01.01 GMT.
 */
static double VuoLogGetTime(void)
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec + t.tv_usec / 1000000.;
}

static double VuoLogStartTime;	///< The time when this module was loaded.

/**
 * Stores the time at which this module was loaded, for use by @ref VuoLogGetElapsedTime().
 */
static void __attribute__((constructor)) VuoLogInit(void)
{
	VuoLogStartTime = VuoLogGetTime();
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused"
/**
 * Returns the number of seconds (including fractional seconds) since this module was loaded.
 */
static double VuoLogGetElapsedTime()
{
	return VuoLogGetTime() - VuoLogStartTime;
}

#ifndef DOXYGEN
	#define VUO_FORMAT_ATTRIBUTE(formatStringIndex, formatArgumentsIndex) __attribute__((format(printf, formatStringIndex, formatArgumentsIndex)))
#endif
static void VuoLog(const char *file, const unsigned int line, const char *function, const char *format, ...) VUO_FORMAT_ATTRIBUTE(4,5);

/**
 * Outputs a message to the system log and to `stderr`.
 */
static void VuoLog(const char *file, const unsigned int line, const char *function, const char *format, ...)
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
			formattedFunction = strndup(function + 2, strchr(function + 2, '_') - (function + 2));
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

	fprintf(stderr, "\033[38;5;%dm# pid=%d  t=%8.4fs %27s:%-4d  %41s \t%s\033[0m\n", getpid()%212+19, getpid(), time, file, line, formattedFunction ? formattedFunction : function, formattedString);

	aslmsg msg = asl_new(ASL_TYPE_MSG);
	asl_set(msg, ASL_KEY_READ_UID, "-1");
	asl_log(NULL, msg, ASL_LEVEL_WARNING, "%s:%d  %s()  %s", file, line, function, formattedString);
	asl_free(msg);

	free(formattedString);
}
#pragma clang diagnostic pop
/// @endcond

/**
 * Prints the name of the file and function to the system log and to `stderr` (and implicitly flushes the output buffer).
 *
 * \eg{
 * void nodeEvent()
 * {
 *     VL();
 * }
 * }
 *
 * @hideinitializer
 */
#define VL() VuoLog(__FILE__, __LINE__, __func__, "")

/**
 * Prints the name of the file and function, and `printf()`-style format/arguments, to the system log and to `stderr` (and implicitly flushes the output buffer).
 *
 * \eg{
 * void nodeEvent(VuoInputData(VuoInteger, "42") number)
 * {
 *     VLog("%d", number);
 * }
 * }
 *
 * @hideinitializer
 */
#define VLog(format, ...) VuoLog(__FILE__, __LINE__, __func__, format, ##__VA_ARGS__)


#if defined(DOXYGEN) || defined(VuoSpecializedNode)
	/**
	 * Prints the name of the file and function, and the summarized contents of the specified variable, to the system log and to `stderr` (and implicitly flushes the output buffer).
	 *
	 * The variable can be any of the following:
	 *
	 *    - a Vuo Core Type
	 *    - a `json_object *`
	 *    - an `NSObject` or subclass
	 *    - a Core Foundation object
	 *    - an `llvm::Value` or subclass
	 *
	 * \eg{
	 * void nodeEvent(VuoInputData(VuoInteger, "42") number)
	 * {
	 *     VLV(number);
	 * }
	 * }
	 *
	 * @hideinitializer
	 */
	#define VLV(variable)
#else
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

	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wunused"
		/**
		 * Returns true if debug mode is enabled.
		 *
		 * Users can enable debug mode by executing `defaults write org.vuo.Editor debug -boolean true`,
		 * and can disable it by executing `defaults delete org.vuo.Editor debug`.
		 *
		 * Nodes should use this to control verbose logging.
		 */
		static bool VuoIsDebugEnabled(void)
		{
			return CFPreferencesGetAppBooleanValue(CFSTR("debug"), CFSTR("org.vuo.Editor"), NULL);
		}

		static char *VuoLog_copyCFDescription(CFTypeRef variable)
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
	#pragma clang diagnostic pop

	#ifdef __cplusplus
		#ifdef __OBJC__
			#define VUO_NSOBJECT_STRINGIFY char *toString(id variable) { return VuoLog_copyCFDescription(variable); }
		#else
			#define VUO_NSOBJECT_STRINGIFY
		#endif

		#ifdef LLVM
			#define VUO_LLVM_TYPES_STRINGIFY char *toString(llvm::Value *variable) { std::string s; llvm::raw_string_ostream os(s); variable->print(os); return strdup(s.c_str()); }
		#else
			#define VUO_LLVM_TYPES_STRINGIFY
		#endif

		#include "coreTypesStringify.hh"
		#define VLV(v) do { class { public:	\
			char *toString(struct json_object *variable)    { return strdup(json_object_to_json_string(variable)); }		\
			char *toString(CFStringRef variable)            { return VuoLog_copyCFDescription(variable); }					\
			char *toString(CFAttributedStringRef variable)  { return VuoLog_copyCFDescription(variable); }					\
			char *toString(CFArrayRef variable)             { return VuoLog_copyCFDescription(variable); }					\
			char *toString(CFMutableArrayRef variable)      { return VuoLog_copyCFDescription(variable); }					\
			char *toString(CFDictionaryRef variable)        { return VuoLog_copyCFDescription(variable); }					\
			char *toString(CFMutableDictionaryRef variable) { return VuoLog_copyCFDescription(variable); }					\
			char *toString(CFNumberRef variable)            { return VuoLog_copyCFDescription(variable); }					\
			char *toString(CGColorRef variable)             { return VuoLog_copyCFDescription(variable); }					\
			char *toString(CGContextRef variable)           { return VuoLog_copyCFDescription(variable); }					\
			char *toString(CGColorSpaceRef variable)        { return VuoLog_copyCFDescription(variable); }					\
			char *toString(CTFontRef variable)              { return VuoLog_copyCFDescription(variable); }					\
			char *toString(CTLineRef variable)              { return VuoLog_copyCFDescription(variable); }					\
			char *toString(CGPoint variable)                { return VuoText_format("(%g,%g)", variable.x, variable.y); }	\
			char *toString(CGRect variable)                 { return VuoText_format("%gx%g @ (%g,%g)", variable.size.width, variable.size.height, variable.origin.x, variable.origin.y); }	\
			VUO_NSOBJECT_STRINGIFY	\
			VUO_CORE_TYPES_STRINGIFY_CPP	\
			VUO_LLVM_TYPES_STRINGIFY \
			} VuoLogConvert;	\
			char *_VuoLog_value = VuoLogConvert.toString(v);	\
			VLog("%s = %s", #v, _VuoLog_value);	\
			free(_VuoLog_value);	\
		} while (0)
	#else
		// Based on https://www.mikeash.com/pyblog/friday-qa-2010-12-31-c-macro-tips-and-tricks.html .

		#ifdef __OBJC__
			#define VUO_NSOBJECT_STRINGIFY(variable) __builtin_choose_expr(__builtin_types_compatible_p(__typeof__(variable), id), VuoLog_copyCFDescription(FORCETYPE(variable, id)),
		#else
			#define VUO_NSOBJECT_STRINGIFY(variable) (
		#endif

		#define FORCETYPE(x, type) (*(type *)(__typeof__(x) []){ x })
		#include "coreTypesStringify.h"
		#define VuoLog_convertToString(variable)	\
			__builtin_choose_expr(__builtin_types_compatible_p(__typeof__(variable), struct json_object *), strdup(json_object_to_json_string(FORCETYPE(variable, struct json_object *))),	\
			__builtin_choose_expr(__builtin_types_compatible_p(__typeof__(variable), CFStringRef),            VuoLog_copyCFDescription(FORCETYPE(variable, CFTypeRef)),				\
			__builtin_choose_expr(__builtin_types_compatible_p(__typeof__(variable), CFAttributedStringRef),  VuoLog_copyCFDescription(FORCETYPE(variable, CFTypeRef)),				\
			__builtin_choose_expr(__builtin_types_compatible_p(__typeof__(variable), CFArrayRef),             VuoLog_copyCFDescription(FORCETYPE(variable, CFTypeRef)),				\
			__builtin_choose_expr(__builtin_types_compatible_p(__typeof__(variable), CFMutableArrayRef),      VuoLog_copyCFDescription(FORCETYPE(variable, CFTypeRef)),				\
			__builtin_choose_expr(__builtin_types_compatible_p(__typeof__(variable), CFDictionaryRef),        VuoLog_copyCFDescription(FORCETYPE(variable, CFTypeRef)),				\
			__builtin_choose_expr(__builtin_types_compatible_p(__typeof__(variable), CFMutableDictionaryRef), VuoLog_copyCFDescription(FORCETYPE(variable, CFTypeRef)),				\
			__builtin_choose_expr(__builtin_types_compatible_p(__typeof__(variable), CFNumberRef),            VuoLog_copyCFDescription(FORCETYPE(variable, CFTypeRef)),				\
			__builtin_choose_expr(__builtin_types_compatible_p(__typeof__(variable), CGColorRef),             VuoLog_copyCFDescription(FORCETYPE(variable, CFTypeRef)),				\
			__builtin_choose_expr(__builtin_types_compatible_p(__typeof__(variable), CGContextRef),           VuoLog_copyCFDescription(FORCETYPE(variable, CFTypeRef)),				\
			__builtin_choose_expr(__builtin_types_compatible_p(__typeof__(variable), CGColorSpaceRef),        VuoLog_copyCFDescription(FORCETYPE(variable, CFTypeRef)),				\
			__builtin_choose_expr(__builtin_types_compatible_p(__typeof__(variable), CTFontRef),              VuoLog_copyCFDescription(FORCETYPE(variable, CFTypeRef)),				\
			__builtin_choose_expr(__builtin_types_compatible_p(__typeof__(variable), CTLineRef),              VuoLog_copyCFDescription(FORCETYPE(variable, CFTypeRef)),				\
			__builtin_choose_expr(__builtin_types_compatible_p(__typeof__(variable), CGPoint),                VuoText_format("(%g,%g)", FORCETYPE(variable, CGPoint).x, FORCETYPE(variable, CGPoint).y),	\
			__builtin_choose_expr(__builtin_types_compatible_p(__typeof__(variable), CGRect),                 VuoText_format("%gx%g @ (%g,%g)", FORCETYPE(variable, CGRect).size.width, FORCETYPE(variable, CGRect).size.height, FORCETYPE(variable, CGRect).origin.x, FORCETYPE(variable, CGRect).origin.y),	\
			VUO_NSOBJECT_STRINGIFY(variable)	\
			VUO_CORE_TYPES_STRINGIFY_C(variable)))))))))))))))))
		#define VLV(variable) do {	\
			char *_VuoLog_value = VuoLog_convertToString(variable);	\
			VLog("%s = %s", #variable, _VuoLog_value);	\
			free(_VuoLog_value);	\
		} while (0)
	#endif
#endif


/**
 * Prints the name of the current file and function, and the address and description of the specified `heapPointer`, to the system log and to `stderr` (and implicitly flushes the output buffer).
 *
 * \eg{
 * void nodeEvent(VuoInputData(VuoShader) shader)
 * {
 *     VLogHeap(shader);
 * }
 * }
 *
 * @hideinitializer
 */
#define VLogHeap(heapPointer) VLog("%s = %p (registered at %s)", #heapPointer, heapPointer, VuoHeap_getDescription(heapPointer))

/**
 * @}
 */

#endif // VUOLOG_H

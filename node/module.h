/**
 * @file
 * Prototypes for node class, type, and library module implementations.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef MODULE_H
#define MODULE_H

#include "VuoHeap.h"



/**
 * @ingroup DevelopingNodeClasses DevelopingTypes DevelopingLibraryModules
 * @defgroup VuoModuleMetadata Module Metadata
 * Name, version, and behavior information to be defined by node classes, port types, and library modules.
 *
 * @{
 */

/**
 * Name, version, and behavior information to be defined by node classes, port types, and library modules.
 *
 * \eg{
 * VuoModuleMetadata({
 * 					 "title" : "Display Console Window",
 * 					 "description" : "<p>Opens a window that displays console output ...</p>",
 * 					 "keywords" : [ "print", "log" ],
 * 					 "version" : "1.0.0",
 * 					 "dependencies" : [ "VuoWindow" ],
 * 					 "compatibleOperatingSystems": {
 * 						 "macosx" : { "min": "10.7" }
 * 					 },
 * 					 "node": {
 * 						 "isInterface" : true
 * 					 }
 * 				 });
 * }
 *
 * Pass a <a href="http://www.json.org/">JSON</a> specification that contains metadata about the node class,
 * port type, or library module.
 *
 * For any type of module, the keys in the JSON specification may optionally include:
 *   - "title" — A human-readable name for the module. It can be overridden by the user.
 *   - "description" — A description of the module's purpose. It may be formatted as plain text or HTML.
 *   - "keywords" — A list of words related to the module's name. This list is used when searching for node classes in the Vuo Editor's Node Library.
 *   - "version" — A version number for the module. It should use the <a href="http://semver.org/">Semantic Versioning (SemVer)</a> scheme.
 *   - "dependencies" — A list of libraries that this module depends upon.
 *   - "compatibleOperatingSystems" — A set of operating systems on which this module can run. Unless this key is present, the module is assumed to run on all operating systems.
 *      This object contains keys for operating system names and values for the range of versions supported. Each range may specify "min", "max", or both.
 *      The operating systems and versions currently supported are:
 *      - "macosx" — "10.6", "10.7", "10.8", "10.9"
 *
 * For node classes, the keys in the JSON specification may optionally include "node". Its keys may optionally include:
 *   - "isInterface" — True if this node class sends data to or receives data from somewhere external to the composition (e.g., input device, file, network). False by default.
 *
 * @see DevelopingNodeClasses
 * @see DevelopingTypes
 * @see DevelopingLibraryModules
 */
#define VuoModuleMetadata(...) const char *moduleDetails = #__VA_ARGS__

/**
 * @}
 */


/**
 * @ingroup DevelopingNodeClasses DevelopingTypes DevelopingLibraryModules
 * @defgroup VuoModuleDebug Module Debugging
 * Macros to help with debugging.
 *
 * @{
 */

#include <stdio.h>
#include <stdlib.h>

/**
 * Prints the name of the file and function to @c stderr (and implicitly flushes the output buffer).  Useful for debugging.
 *
 * \eg{
 * void nodeEvent()
 * {
 *     VL();
 * }
 * }
 */
#define VL() fprintf(stderr, "\033[38;5;%dm# pid=%d\t%s:%d :: %s()\033[0m\n", getpid()%212+19, getpid(), __FILE__, __LINE__, __func__);

/**
 * Prints the name of the file and function, and `printf()`-style format/arguments, to @c stderr (and implicitly flushes the output buffer).  Useful for debugging.
 *
 * \eg{
 * void nodeEvent(VuoInputData(VuoInteger, "42") number)
 * {
 *     VLog("%d", number);
 * }
 * }
 */
#define VLog(format, ...) fprintf(stderr, "\033[38;5;%dm# pid=%d\t%s:%d :: %s()\t" format "\033[0m\n", getpid()%212+19, getpid(), __FILE__, __LINE__, __func__, ##__VA_ARGS__);

/**
 * Prints the name of the current file and function, and the address and description of the specified @c heapPointer, to @c stderr (and implicitly flushes the output buffer).  Useful for debugging.
 *
 * \eg{
 * void nodeEvent(VuoInputData(VuoShader) shader)
 * {
 *     VLogHeap(shader);
 * }
 * }
 */
#define VLogHeap(heapPointer) VLog("%s = %p (registered at %s)", #heapPointer, heapPointer, VuoHeap_getDescription(heapPointer));

/**
 * Prints the specified Core Foundation object.
 */
#define VLogCF(coreFoundationRef) { CFStringRef d = CFCopyDescription(coreFoundationRef); CFIndex len = CFStringGetLength(d)+1; char *z = (char *)malloc(len); CFStringGetCString(d, z, len, kCFStringEncodingUTF8); VLog("%s = %s", #coreFoundationRef, z); free(z); CFRelease(d); }

/**
 * @}
 */


#ifndef __OBJC__

/**
 * Returns the smaller of @c a and @c b.
 */
#define	MIN(a,b) (((a)<(b))?(a):(b))

/**
 * Returns the larger of @c a and @c b.
 */
#define	MAX(a,b) (((a)>(b))?(a):(b))

#endif



#endif

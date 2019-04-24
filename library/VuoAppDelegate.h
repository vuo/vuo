/**
 * @file
 * VuoAppDelegate interface.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifdef __OBJC__

#ifndef NS_RETURNS_INNER_POINTER
#define NS_RETURNS_INNER_POINTER
#endif
#import <AppKit/AppKit.h>
#undef NS_RETURNS_INNER_POINTER

/**
 * NSApplication delegate, to shut down cleanly when the user requests to quit.
 */
@interface VuoAppDelegate : NSObject<NSApplicationDelegate>
@end

#endif

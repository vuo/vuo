/**
 * @file
 * VuoAppAboutBox interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifdef __OBJC__

#ifndef NS_RETURNS_INNER_POINTER
#define NS_RETURNS_INNER_POINTER
#endif
#import <AppKit/AppKit.h>
#undef NS_RETURNS_INNER_POINTER

/**
 * The About dialog for VuoCompositionLoader and exported Vuo apps.
 */
@interface VuoAppAboutBox : NSObject
- (void)displayAboutPanel:(id)sender;
@end

#endif

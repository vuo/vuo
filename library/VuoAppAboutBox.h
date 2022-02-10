/**
 * @file
 * VuoAppAboutBox interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifdef __OBJC__

#include "VuoMacOSSDKWorkaround.h"
#import <AppKit/AppKit.h>

/**
 * The About dialog for VuoCompositionLoader and exported Vuo apps.
 */
@interface VuoAppAboutBox : NSObject
- (void)displayAboutPanel:(id)sender;
@end

#endif

/**
 * @file
 * VuoRunnerCocoa+Conversion header.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#if defined(__OBJC__)

#include <string>
using namespace std;

#include "VuoRunnerCocoa.h"

extern "C" {
#include "VuoImage.h"
}

/**
 * (deprecated) Methods for converting between Cocoa types and Vuo types.
 *
 * @vuoRunnerCocoaDeprecated
 */
VUO_RUNNER_COCOA_DEPRECATED @interface VuoRunnerCocoa (Conversion)
+ (id)cocoaObjectWithVuoValue:(json_object *)vuoValue ofType:(string)type VUO_RUNNER_COCOA_DEPRECATED;
+ (json_object *)vuoValueWithCocoaObject:(id)value VUO_RUNNER_COCOA_DEPRECATED;
+ (NSImage *)nsImageWithVuoImage:(VuoImage)vi VUO_RUNNER_COCOA_DEPRECATED;
@end

/**
 * Methods for converting between Cocoa types and Vuo types.
 *
 * @vuoRunnerCocoaDeprecated
 */
VUO_RUNNER_COCOA_DEPRECATED @interface NSNumber (VuoRunnerCocoaConversion)
- (json_object *)vuoValue VUO_RUNNER_COCOA_DEPRECATED;
@end

/**
 * Methods for converting between Cocoa types and Vuo types.
 *
 * @vuoRunnerCocoaDeprecated
 */
VUO_RUNNER_COCOA_DEPRECATED @interface NSString (VuoRunnerCocoaConversion)
- (json_object *)vuoValue VUO_RUNNER_COCOA_DEPRECATED;
@end

/**
 * Methods for converting between Cocoa types and Vuo types.
 *
 * @vuoRunnerCocoaDeprecated
 */
VUO_RUNNER_COCOA_DEPRECATED @interface NSColor (VuoRunnerCocoaConversion)
- (json_object *)vuoValue VUO_RUNNER_COCOA_DEPRECATED;
@end

/**
 * Methods for converting between Cocoa types and Vuo types.
 *
 * @vuoRunnerCocoaDeprecated
 */
VUO_RUNNER_COCOA_DEPRECATED @interface CIColor (VuoRunnerCocoaConversion)
- (json_object *)vuoValue VUO_RUNNER_COCOA_DEPRECATED;
@end

/**
 * Methods for converting between Cocoa types and Vuo types.
 *
 * @vuoRunnerCocoaDeprecated
 */
VUO_RUNNER_COCOA_DEPRECATED @interface NSImage (VuoRunnerCocoaConversion)
- (json_object *)vuoValue VUO_RUNNER_COCOA_DEPRECATED;
@end

/**
 * Methods for converting between Cocoa types and Vuo types.
 *
 * @vuoRunnerCocoaDeprecated
 */
VUO_RUNNER_COCOA_DEPRECATED @interface NSBitmapImageRep (VuoRunnerCocoaConversion)
- (json_object *)vuoValue VUO_RUNNER_COCOA_DEPRECATED;
@end

/**
 * Methods for converting between Cocoa types and Vuo types.
 *
 * @vuoRunnerCocoaDeprecated
 */
VUO_RUNNER_COCOA_DEPRECATED @interface NSValue (VuoRunnerCocoaConversion)
- (json_object *)vuoValue VUO_RUNNER_COCOA_DEPRECATED;
@end

/**
 * Methods for converting between Cocoa types and Vuo types.
 *
 * @vuoRunnerCocoaDeprecated
 */
VUO_RUNNER_COCOA_DEPRECATED @interface NSData (VuoRunnerCocoaConversion)
- (json_object *)vuoValue VUO_RUNNER_COCOA_DEPRECATED;
@end

/**
 * Methods for converting between Cocoa types and Vuo types.
 *
 * @vuoRunnerCocoaDeprecated
 */
VUO_RUNNER_COCOA_DEPRECATED @interface NSArray (VuoRunnerCocoaConversion)
- (json_object *)vuoValue VUO_RUNNER_COCOA_DEPRECATED;
@end

#endif // defined(__OBJC__) || defined(DOXYGEN)

/**
 * @file
 * VuoRunnerCocoa+Conversion header.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#if defined(__OBJC__) || defined(DOXYGEN)

#ifndef VUORUNNERCOCOACONVERSION_H
#define VUORUNNERCOCOACONVERSION_H

#include <string>
using namespace std;

#include "VuoRunnerCocoa.h"

extern "C" {
#include "VuoImage.h"
}

/**
 * Methods for converting between Cocoa types and Vuo types.
 */
@interface VuoRunnerCocoa (Conversion)
+ (id)cocoaObjectWithVuoValue:(json_object *)vuoValue ofType:(string)type;
+ (json_object *)vuoValueWithCocoaObject:(id)value;
+ (NSImage *)nsImageWithVuoImage:(VuoImage)vi;
@end

/**
 * Methods for converting between Cocoa types and Vuo types.
 */
@interface NSNumber (VuoRunnerCocoaConversion)
- (json_object *)vuoValue;
@end

/**
 * Methods for converting between Cocoa types and Vuo types.
 */
@interface NSString (VuoRunnerCocoaConversion)
- (json_object *)vuoValue;
@end

/**
 * Methods for converting between Cocoa types and Vuo types.
 */
@interface NSColor (VuoRunnerCocoaConversion)
- (json_object *)vuoValue;
@end

/**
 * Methods for converting between Cocoa types and Vuo types.
 */
@interface CIColor (VuoRunnerCocoaConversion)
- (json_object *)vuoValue;
@end

/**
 * Methods for converting between Cocoa types and Vuo types.
 */
@interface NSImage (VuoRunnerCocoaConversion)
- (json_object *)vuoValue;
@end

/**
 * Methods for converting between Cocoa types and Vuo types.
 */
@interface NSBitmapImageRep (VuoRunnerCocoaConversion)
- (json_object *)vuoValue;
@end

/**
 * Methods for converting between Cocoa types and Vuo types.
 */
@interface NSValue (VuoRunnerCocoaConversion)
- (json_object *)vuoValue;
@end

/**
 * Methods for converting between Cocoa types and Vuo types.
 */
@interface NSData (VuoRunnerCocoaConversion)
- (json_object *)vuoValue;
@end

/**
 * Methods for converting between Cocoa types and Vuo types.
 */
@interface NSArray (VuoRunnerCocoaConversion)
- (json_object *)vuoValue;
@end

#endif // VUORUNNERCOCOACONVERSION_H

#endif // defined(__OBJC__) || defined(DOXYGEN)

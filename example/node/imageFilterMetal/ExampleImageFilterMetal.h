/**
 * @file
 * ExampleImageFilterMetal interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#import <IOSurface/IOSurface.h>

void *ExampleImageFilterMetal_make(void);
IOSurfaceRef ExampleImageFilterMetal_processImage(void *t, IOSurfaceRef image, float phase);

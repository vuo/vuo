/**
 * @file
 * ExampleImageFilterCoreImage interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoMacOSSDKWorkaround.h"
#import <OpenGL/CGLMacro.h>

void *ExampleImageFilterCoreImage_make(CGLContextObj cgl_ctx);
VuoImage ExampleImageFilterCoreImage_processImage(void *t, VuoImage inputImage, VuoPoint2d position, float radius, float angle);

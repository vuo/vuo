/**
 * @file
 * VuoColorspace implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "module.h"

#include "VuoColorspace.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
    "title" : "VuoColorspace",
});
#endif

/// Disable NS_RETURNS_INNER_POINTER (new in Mac OS 10.10's system headers), since Clang 3.2 doesn't support it.
/// https://b33p.net/kosada/node/9140
#ifndef NS_RETURNS_INNER_POINTER
#define NS_RETURNS_INNER_POINTER
#endif
#import <AppKit/AppKit.h>
#undef NS_RETURNS_INNER_POINTER


/**
 * Creates a new VuoColor from CMYK values.
 *
 * `colorspace`: 0 = linear, 1 = generic
 */
VuoColor VuoColorspace_makeCMYKAColor(VuoReal c, VuoReal m, VuoReal y, VuoReal k, VuoReal a, int colorspace)
{
	if (colorspace == 0)
		// http://www.rapidtables.com/convert/color/cmyk-to-rgb.htm
		return VuoColor_makeWithRGBA(
			(1. - c) * (1. - k),
			(1. - m) * (1. - k),
			(1. - y) * (1. - k),
			a);
	else
	{
		CGFloat components[5] = { c, m, y, k, a };
		NSColor *inputColor = [NSColor colorWithColorSpace:NSColorSpace.genericCMYKColorSpace components:components count:5];
		NSColor *outputColor = [inputColor colorUsingColorSpace:NSColorSpace.sRGBColorSpace];
		return VuoColor_makeWithRGBA(outputColor.redComponent, outputColor.greenComponent, outputColor.blueComponent, outputColor.alphaComponent);
	}
}

/**
 * Get the Cyan, Magenta, Yellow, Black, and Alpha values from a VuoColor.
 *
 * `colorspace`: 0 = linear, 1 = generic
 */
void VuoColorspace_getCMYKA(VuoColor color, int colorspace, VuoReal *c, VuoReal *m, VuoReal *y, VuoReal *k, VuoReal *a)
{
	if (colorspace == 0)
	{
		// http://www.rapidtables.com/convert/color/rgb-to-cmyk.htm

		double r = color.r,
			   g = color.g,
			   b = color.b;

		*k = 1. - MAX(MAX(r, g), b);
		if (VuoReal_areEqual(*k, 1))
			*c = *m = *y = 0;
		else
		{
			*c = (1. - r - *k) / (1. - *k);
			*m = (1. - g - *k) / (1. - *k);
			*y = (1. - b - *k) / (1. - *k);
		}
		*a = color.a;
	}
	else
	{
		CGFloat components[4] = { color.r, color.g, color.b, color.a };
		NSColor *inputColor = [NSColor colorWithColorSpace:NSColorSpace.sRGBColorSpace components:components count:4];
		NSColor *outputColor = [inputColor colorUsingColorSpace:NSColorSpace.genericCMYKColorSpace];
		*c = outputColor.cyanComponent;
		*m = outputColor.magentaComponent;
		*y = outputColor.yellowComponent;
		*k = outputColor.blackComponent;
		*a = outputColor.alphaComponent;
	}
}

static NSColorSpace *_VuoColorspace_colorspaceForInteger(VuoInteger colorspace)
{
	if (colorspace == 0)
		return [NSColorSpace.sRGBColorSpace retain];
	else if (colorspace == 1)
		return [NSColorSpace.adobeRGB1998ColorSpace retain];
	else if (colorspace == 2)
		return [NSColorSpace.genericRGBColorSpace retain];
	else if (colorspace == 3)
		return [NSColorSpace.genericCMYKColorSpace retain];
	else if (colorspace == 4)
		return [[NSColorSpace alloc] initWithICCProfileData:[NSData dataWithContentsOfFile:@"/System/Library/ColorSync/Profiles/Generic Lab Profile.icc"]];
	else if (colorspace == 5)
		return [[NSColorSpace alloc] initWithICCProfileData:[NSData dataWithContentsOfFile:@"/System/Library/ColorSync/Profiles/Generic XYZ Profile.icc"]];

	return nil;
}

static NSColorSpace *_VuoColorspace_colorspaceForData(VuoData colorspace)
{
	if (!colorspace.data || colorspace.size <= 0)
		return nil;

	NSData *data = [NSData dataWithBytesNoCopy:colorspace.data length:colorspace.size freeWhenDone:NO];
	if (!data)
		return nil;

	return [[NSColorSpace alloc] initWithICCProfileData:data];
}

static VuoColor _VuoColorspace_convertFromICC(NSColorSpace *cs, VuoList_VuoReal components)
{
	if (!cs)
	{
		VUserLog("Error: Couldn't load colorspace.");
		return VuoColor_makeWithRGBA(0,0,0,0);
	}

	unsigned long providedComponentCount = VuoListGetCount_VuoReal(components);
	unsigned long allowedComponentCount = cs.numberOfColorComponents + 1;
	CGFloat cfComponents[allowedComponentCount];
	for (int i = 0; i < allowedComponentCount - 1; ++i)
		cfComponents[i] = 0;
	cfComponents[allowedComponentCount - 1] = 1;  // Alpha
	for (int i = 0; i < MIN(providedComponentCount, allowedComponentCount); ++i)
		cfComponents[i] = VuoListGetValue_VuoReal(components, i + 1);

	NSColor *inputColor = [NSColor colorWithColorSpace:cs components:cfComponents count:allowedComponentCount];
	[cs release];
	NSColor *outputColor = [inputColor colorUsingColorSpace:NSColorSpace.sRGBColorSpace];
	return VuoColor_makeWithRGBA(outputColor.redComponent, outputColor.greenComponent, outputColor.blueComponent, outputColor.alphaComponent);
}

/**
 * Converts the specified color `components` (color channels + alpha) into an sRGB `VuoColor`.
 *
 * `colorspace`: 0 = sRGB, 1 = Adobe RGB (1998), 2 = Generic RGB, 3 = Generic CMYK, 4 = Generic L*a*b*, 5 = Generic XYZ
 */
VuoColor VuoColorspace_makeICCColor_VuoInteger(VuoInteger colorspace, VuoList_VuoReal components)
{
    return _VuoColorspace_convertFromICC(_VuoColorspace_colorspaceForInteger(colorspace), components);
}

/**
 * Converts the specified color `components` (color channels + alpha) into an sRGB `VuoColor`
 * using the ICC profile `colorspace`.
 */
VuoColor VuoColorspace_makeICCColor_VuoData(VuoData colorspace, VuoList_VuoReal components)
{
    return _VuoColorspace_convertFromICC(_VuoColorspace_colorspaceForData(colorspace), components);
}

static VuoList_VuoReal _VuoColorspace_convertToICC(NSColorSpace *cs, VuoColor color)
{
	if (!cs)
	{
		VUserLog("Error: Couldn't load colorspace.");
		return NULL;
	}

	NSColor *inputColor = [NSColor colorWithSRGBRed:color.r green:color.g blue:color.b alpha:color.a];
	NSColor *outputColor = [inputColor colorUsingColorSpace:cs];
	[cs release];

	unsigned long componentCount = outputColor.numberOfComponents;
	CGFloat outputComponents[componentCount];
	[outputColor getComponents:outputComponents];

	VuoList_VuoReal outputList = VuoListCreate_VuoReal();
	for (int i = 0; i < componentCount; ++i)
		VuoListAppendValue_VuoReal(outputList, outputComponents[i]);

	return outputList;
}

/**
 * Converts the specified sRGB `color` into components in the specified colorspace.
 *
 * `colorspace`: 0 = sRGB, 1 = Adobe RGB (1998), 2 = Generic RGB, 3 = Generic CMYK, 4 = Generic L*a*b*, 5 = Generic XYZ
 */
VuoList_VuoReal VuoColorspace_getICC_VuoInteger(VuoInteger colorspace, VuoColor color)
{
    return _VuoColorspace_convertToICC(_VuoColorspace_colorspaceForInteger(colorspace), color);
}

/**
 * Converts the specified sRGB `color` into components in the ICC profile `colorspace`.
 */
VuoList_VuoReal VuoColorspace_getICC_VuoData(VuoData colorspace, VuoColor color)
{
    return _VuoColorspace_convertToICC(_VuoColorspace_colorspaceForData(colorspace), color);
}

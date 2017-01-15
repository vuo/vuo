/**
 * @file
 * VuoImageColorDepth implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoImageColorDepth.h"
#include "VuoGlPool.h"
#include "VuoList_VuoImageColorDepth.h"
#include "VuoText.h"
#include <OpenGL/CGLMacro.h>

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Image Color Depth",
					  "description" : "An image's color bit-depth",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						"VuoList_VuoImageColorDepth",
						"VuoText",
						"VuoGlPool"
					  ]
				  });
#endif
/// @}

/**
 * Returns the OpenGL internal format constant for a texture with the specified `baseFormat` and `imageColorDepth`.
 *
 * `baseFormat` can be:
 *
 *    - GL_RGB
 *    - GL_BGR
 *    - GL_YCBCR_422_APPLE
 *    - GL_RGBA
 *    - GL_BGRA
 *    - GL_LUMINANCE
 *    - GL_LUMINANCE_ALPHA
 */
unsigned int VuoImageColorDepth_getGlInternalFormat(unsigned int baseFormat, VuoImageColorDepth imageColorDepth)
{
	if (baseFormat == GL_RGB
	 || baseFormat == GL_BGR
	 || baseFormat == GL_YCBCR_422_APPLE)
		return (imageColorDepth == VuoImageColorDepth_16) ? GL_RGB16F_ARB             : GL_RGB;
	else if (baseFormat == GL_RGBA
		  || baseFormat == GL_BGRA)
		return (imageColorDepth == VuoImageColorDepth_16) ? GL_RGBA16F_ARB            : GL_RGBA;
	else if (baseFormat == GL_LUMINANCE)
		return (imageColorDepth == VuoImageColorDepth_16) ? GL_LUMINANCE16F_ARB       : GL_LUMINANCE8;
	else if (baseFormat == GL_LUMINANCE_ALPHA)
		return (imageColorDepth == VuoImageColorDepth_16) ? GL_LUMINANCE_ALPHA16F_ARB : GL_LUMINANCE8_ALPHA8;

	char *formatString = VuoGl_stringForConstant(baseFormat);
	VUserLog("Error: Unknown baseFormat %x (%s)", baseFormat, formatString);
	free(formatString);
	return GL_RGB;
}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{"8bpc"}
 */
VuoImageColorDepth VuoImageColorDepth_makeFromJson(json_object * js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	VuoImageColorDepth value = VuoImageColorDepth_8;

	if (! strcmp(valueAsString, "16bpc"))
		value = VuoImageColorDepth_16;

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoImageColorDepth_getJson(const VuoImageColorDepth value)
{
	char *valueAsString = "";

	switch (value)
	{
		case VuoImageColorDepth_8:
			valueAsString = "8bpc";
			break;
		case VuoImageColorDepth_16:
			valueAsString = "16bpc";
			break;
	}

	return json_object_new_string(valueAsString);
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoImageColorDepth VuoImageColorDepth_getAllowedValues(void)
{
	VuoList_VuoImageColorDepth l = VuoListCreate_VuoImageColorDepth();
	VuoListAppendValue_VuoImageColorDepth(l, VuoImageColorDepth_8);
	VuoListAppendValue_VuoImageColorDepth(l, VuoImageColorDepth_16);
	return l;
}

/**
 * Returns a compact string representation of @c value.
 */
char * VuoImageColorDepth_getSummary(const VuoImageColorDepth value)
{
	int bits = 0;
	if (value == VuoImageColorDepth_8)
		bits = 8;
	else if (value == VuoImageColorDepth_16)
		bits = 16;

	return VuoText_format("%d bits per channel", bits);
}

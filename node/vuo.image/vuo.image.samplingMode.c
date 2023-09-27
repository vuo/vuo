/**
 * @file
 * vuo.image.samplingMode node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
	"title": "Change Sampling Mode",
	"keywords": [
		"filtering", "interpolation", "resampling", "blending",
		"enlarge", "reduce", "resize", "scale",
		"linear", "bilinear",
		"nearest-neighbor", "blocky", "chunky",
		"pixellate", "pixels", "lofi", "simplify", "mosaic", "censor", "tile", "grid",
		"rectangle", "linear", "cube", "square", "box", "overenlarge",
		"pixelate", // American spelling
	],
	"version": "1.0.0",
	"node": {
		"exampleCompositions": [ "CompareSamplingModes.vuo", "MakeCyberspaceAvatar.vuo" ]
	}
});

void nodeEvent(
	VuoInputData(VuoImage) image,
	VuoInputData(VuoInteger, {"menuItems":[
		{"value": 0, "name":"Nearest-Neighbor"},
		{"value": 1, "name":"Bilinear"},
	], "default":0}) samplingMode,
	VuoOutputData(VuoImage) outputImage)
{
	if (!image)
	{
		*outputImage = NULL;
		return;
	}

	VuoImage img = VuoImage_makeCopy(image, false, 0, 0, false);
	VuoImage_setSamplingMode(img, samplingMode);
	*outputImage = img;
}

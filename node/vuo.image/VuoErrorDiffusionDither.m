/**
 * @file
 * VuoErrorDiffusionDither implementation.
 *
 * @copyright Copyright © 2012–2026 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#import <Metal/Metal.h>
#import <simd/vector_types.h>
#import <simd/matrix_types.h>
#import <simd/math.h>

#include "VuoErrorDiffusionDither.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
	"title": "VuoErrorDiffusionDither",
	"dependencies": [
		"Metal.framework",
	]
});
#endif

/**
 * Keep this in sync with the Metal MSL copy below.
 */
typedef struct
{
	/**
	 * tileSize represents the size of each GPU parallel workunit,
	 * an isolated environment in which error-diffusion dithering happens.
	 * tileSize should be large enough to accumulate enough quantization error
	 * so that the characteristic Floyd-Steinberg dot-arches become visible
	 * and to minimize the number of tile boundary artifacts
	 * (values above about 32 are probably suitable for that),
	 * but small enough that there are enough tiles to keep
	 * the GPU's concurrent execution units busy
	 * (the Apple M1 Pro has 256 execution units; values below
	 * about 60 ((1280/60) * (720/60) = 256) should keep that hardware fully occupied).
	 */
	uint8_t tileSize;

	/**
	 * tileStride represents the relative placement of each tile;
	 * (tileSize - tileStride) is the number of pixels of overlap,
	 * the amount of redundant dithering work performed.
	 * tileStride must be less than or equal to tileSize (else there would be gaps).
	 * Having a tileStride that's a few pixels smaller than tileSize
	 * enables us to crop off a few pixels from the top/left of each tile,
	 * in order to give the error buffer a chance to "warm up"
	 * (i.e. accumulate some error, for better consistency with surrounding tiles).
	 */
	uint8_t tileStride;

	uint8_t cropShift;                         ///< The number of warm-up pixels to crop off the top/left of each tile.
	uint8_t gridShiftX;                        ///< The number of pixels to offset the start of the grid relative to the left edge of the image.
	uint8_t gridShiftY;                        ///< The number of pixels to offset the start of the grid relative to the bottom edge of the image.
	float ditheringIntensity;                  ///< How much of the quantization error to distribute.  A value of 1 means the quantization error will be clamped to +/- 0.5.
	uint8_t paletteCount;                      ///< The number of entries in the `palette` buffer.
	float colorDistanceThresholdSquared;       ///< Ends palette-matching early when a palette-color distance is less than this.
	VuoDitherDiffusionMatrix diffusionMatrix;  ///< The sparse matrix across which to distribute the quantization error.
} VuoErrorDiffusionDitherParameters;

/**
 * Converts sRGB-D65 -> Linear RGB-D65.
 */
static inline __attribute__((always_inline)) VuoColor VuoErrorDiffusionDither_sRGBToLinearRGB(VuoColor sRGB)
{
	// Rough approximation; see sRGBToLinearRGB.
	return (VuoColor){
		sRGB.r * sRGB.r,
		sRGB.g * sRGB.g,
		sRGB.b * sRGB.b,
		sRGB.a,
	};
}

/**
 * Extracted from <simd/simd.h> (which includes other files that don't work with Vuo's Clang 5.x).
 */
static simd_float4 SIMD_CFUNC simd_mul(simd_float4x4 __x, simd_float4 __y) {
	simd_float4 __r = __x.columns[0] * __y[0];
	__r = simd_muladd(__x.columns[1], __y[1], __r);
	__r = simd_muladd(__x.columns[2], __y[2], __r);
	__r = simd_muladd(__x.columns[3], __y[3], __r);
	return __r;
}

/**
 * Extracted from <simd/simd.h> (which includes other files that don't work with Vuo's Clang 5.x).
 */
static simd_float4x4 SIMD_CFUNC simd_transpose(simd_float4x4 __x) {
    return (matrix_float4x4){{(simd_float4){__x.columns[0][0], __x.columns[1][0], __x.columns[2][0], __x.columns[3][0]},
                              (simd_float4){__x.columns[0][1], __x.columns[1][1], __x.columns[2][1], __x.columns[3][1]},
                              (simd_float4){__x.columns[0][2], __x.columns[1][2], __x.columns[2][2], __x.columns[3][2]},
                              (simd_float4){__x.columns[0][3], __x.columns[1][3], __x.columns[2][3], __x.columns[3][3]}}};
}

/**
 * Converts Linear RGB-D65 -> YUV/YCbCr (broadcast-limited-luma, SD / BT.601).
 */
static inline __attribute__((always_inline)) VuoColor VuoErrorDiffusionDither_linearRGBToYUV(VuoColor linearRGB)
{
	// https://support.medialooks.com/hc/en-us/articles/360030737152-Color-correction-with-matrix-transformation
	matrix_float4x4 linearRGBToYCbCr = (matrix_float4x4){{
		(simd_float4){ .257,  .504,  .098,  .0625},
		(simd_float4){-.148, -.291,  .439,  .5   },
		(simd_float4){ .439, -.368, -.071,  .5   },
		(simd_float4){ .0  ,  .0  ,  .0  , 1.    }}};
	simd_float4 f = simd_mul(simd_transpose(linearRGBToYCbCr), (simd_float4){linearRGB.r, linearRGB.g, linearRGB.b, 1.});
	return (VuoColor){f.r, f.g, f.b, linearRGB.a};
}

/**
 * A state object for performing error-diffusion dithering.
 */
@interface VuoErrorDiffusionDither_internal : NSObject
@property(retain) id<MTLDevice> device;                       ///< The GPU on which to perform the calculations.
@property(retain) id<MTLBuffer> parametersBuffer;             ///< Data that's constant across all tiles in an image.
@property(retain) id<MTLBuffer> errorAccumulatorBuffer;       ///< Workbuffer, containing YUV pixel data and accumulated error.  Contains full-size tiles; depending on (tileSize - tileStride) this may be significantly larger than the input/output texture.
@property(retain) id<MTLBuffer> paletteBuffer;                ///< In YUV.
@property(retain) id<MTLCommandQueue> commandQueue;           ///< For scheduling GPU commands.
@property(retain) id<MTLComputePipelineState> pipelineState;  ///< Wraps the GPU errorDiffusion function.
@property MTLResourceOptions hazardTracking;                  ///< MTLResourceHazardTrackingModeUntracked (if available on the active macOS version).
@end

@implementation VuoErrorDiffusionDither_internal
- (id)init
{
	if (self = [super init])
	{
		_device = MTLCreateSystemDefaultDevice();
		_commandQueue = [_device newCommandQueue];
		NSString *shaderSource = @VUO_STRINGIFY(
			\n#include <metal_stdlib>\n
			using namespace metal;

			/**
			 * Keep this in sync with the C copy in VuoErrorDiffusionDither.h.
			 */
			typedef struct {
				int8_t xOffset;
				uint8_t yOffset;
				uint8_t weight;
			} VuoDitherDiffusionStep;

			/**
			 * Keep this in sync with the C copy in VuoErrorDiffusionDither.h.
			 */
			typedef struct {
				VuoDitherDiffusionStep steps[12];  // VuoErrorDiffusionDither_maxSteps
				uint8_t stepCount;
				uint8_t divisor;
			} VuoDitherDiffusionMatrix;

			/**
			 * Keep this in sync with the C copy above.
			 */
			typedef struct
			{
				uint8_t tileSize;
				uint8_t tileStride;
				uint8_t cropShift;
				uint8_t gridShiftX;
				uint8_t gridShiftY;
				float ditheringIntensity;
				uint8_t paletteCount;
				float colorDistanceThresholdSquared;
				VuoDitherDiffusionMatrix diffusionMatrix;
			} VuoErrorDiffusionDitherParameters;

			/**
			 * Converts sRGB-D65 -> Linear RGB-D65.
			 */
			static half3 sRGBToLinearRGB(half3 sRGB)
			{
				// return half3(
				//  sRGB.r <= 0.04045 ? sRGB.r / 12.92 : pow((sRGB.r + 0.055) / 1.055, 2.4),
				//  sRGB.g <= 0.04045 ? sRGB.g / 12.92 : pow((sRGB.g + 0.055) / 1.055, 2.4),
				//  sRGB.b <= 0.04045 ? sRGB.b / 12.92 : pow((sRGB.b + 0.055) / 1.055, 2.4));
				// This rough approximation is about 5% faster than the full conversion.
				//    - full   — GPUTime 11.18 msec/frame
				//    - simple — GPUTime 10.62 msec/frame
				return sRGB * sRGB;
			}

			/**
			 * Converts Linear RGB-D65 -> sRGB-D65.
			 */
			static half3 linearRGBToSRGB(half3 linearRGB)
			{
				// return half3(
				//  linearRGB.r <= 0.0031308 ? linearRGB.r * 12.92 : (1.055 * pow(linearRGB.r, (half)0.41667)) - 0.055,
				//  linearRGB.g <= 0.0031308 ? linearRGB.g * 12.92 : (1.055 * pow(linearRGB.g, (half)0.41667)) - 0.055,
				//  linearRGB.b <= 0.0031308 ? linearRGB.b * 12.92 : (1.055 * pow(linearRGB.b, (half)0.41667)) - 0.055);
				// Rough approximation; see sRGBToLinearRGB.
				return sqrt(linearRGB);
			}

			/**
			 * Converts Linear RGB-D65 -> YUV/YCbCr (broadcast-limited-luma, SD / BT.601).
			 */
			static half3 linearRGBToYUV(half3 linearRGB)
			{
				// https://support.medialooks.com/hc/en-us/articles/360030737152-Color-correction-with-matrix-transformation
				half4x4 linearRGBToYCbCr(
					half4( .257,  .504,  .098,  .0625),
					half4(-.148, -.291,  .439,  .5   ),
					half4( .439, -.368, -.071,  .5   ),
					half4( .0  ,  .0  ,  .0  , 1.    ));
				return (half4(linearRGB, 1.) * linearRGBToYCbCr).rgb;
			}

			/**
			 * Converts YUV/YCbCr (broadcast-limited-luma, SD / BT.601) -> Linear RGB-D65.
			 */
			static half3 yuvToLinearRGB(half3 yuv)
			{
				// https://support.medialooks.com/hc/en-us/articles/360030737152-Color-correction-with-matrix-transformation
				half4x4 yCbCrToLinearRGB(
					half4(1.164, - .002,  1.596, - .87075),
					half4(1.164, - .391, - .813,   .52925),  // On the medialooks page, -.391 and -.813 are swapped (incorrect compared to the matrix inverse of linearRGBToYCbCr)
					half4(1.164,  2.018, - .001, -1.08175),
					half4( .0  ,   .0  ,   .0  ,  1.     ));
				return (half4(yuv, 1.) * yCbCrToLinearRGB).rgb;
			}

			static half4 findNearestPaletteColor(constant VuoErrorDiffusionDitherParameters &parameters, device float4 *palette, half3 sourceColorYUV)
			{
				half4 nearestPaletteColor(-1);
				float shortestDistanceSquared = 999;
				for (int c = 0; c < parameters.paletteCount; ++c)
				{
					float4 pc(palette[c]);
					half4 paletteColorYUV(pc.r, pc.g, pc.b, pc.a);
					half3 linearDifference = paletteColorYUV.rgb - sourceColorYUV;
					float distanceSquared = dot(linearDifference, linearDifference);
					if (distanceSquared < shortestDistanceSquared)
					{
						nearestPaletteColor = paletteColorYUV;
						if (distanceSquared < parameters.colorDistanceThresholdSquared)
							break;
						shortestDistanceSquared = distanceSquared;
					}
				}
				return nearestPaletteColor;
			}

			static void adjust(device half3 *errorAccumulator, constant VuoErrorDiffusionDitherParameters &parameters, uint2 textureSize, uint2 tileStartInErrorAccumulator, int tileCountX, int stepIndex, int x, int y, half3 error)
			{
				VuoDitherDiffusionStep step = parameters.diffusionMatrix.steps[stepIndex];

				// We're working with a flipped OpenGL buffer, so flip the y offset.
				x += step.xOffset;
				y -= step.yOffset;

				// Skipping this (apparently-unnecessary?) bounds check improves performance by about 10%.
				// if (x >= 0 && x < parameters.tileSize
				//  && y >= 0 && y < parameters.tileSize)
				{
					uint indexIntoErrorAccumulator = tileStartInErrorAccumulator.y * tileCountX * parameters.tileSize + tileStartInErrorAccumulator.x
						+ y * tileCountX * parameters.tileSize
						+ x;
					errorAccumulator[indexIntoErrorAccumulator] += error * step.weight / parameters.diffusionMatrix.divisor;
				}
			}

			/**
			 * Performs standard (sequential/linear) error diffusion on a subset (tile) of an image.
			 */
			kernel void errorDiffusion(
				constant VuoErrorDiffusionDitherParameters &parameters       [[buffer(0)]],
				texture2d<half, access::read>               inputTexture     [[texture(0)]],
				device half3                               *errorAccumulator [[buffer(1)]],
				texture2d<half, access::write>              outputTexture    [[texture(1)]],
				device float4                              *palette          [[buffer(2)]],
				uint2                                       gid              [[thread_position_in_grid]])
			{
				int2 gidShifted(gid.x - 1, gid.y - 1);
				int2 tileStartInInputTexture      = gidShifted * parameters.tileStride + int2(parameters.gridShiftX, parameters.gridShiftY);
				uint2 tileStartInErrorAccumulator = gid        * parameters.tileSize;
				uint2 textureSize(inputTexture.get_width(), inputTexture.get_height());
				uint tileCountX = ceil((float)textureSize.x / parameters.tileStride) + 1;

				int2 tileSizeLimited(parameters.tileSize);
				if (tileStartInInputTexture.x + parameters.tileSize > (int)textureSize.x)
					tileSizeLimited.x = parameters.tileSize + (textureSize.x - (tileStartInInputTexture.x + parameters.tileSize));
				if (tileStartInInputTexture.y + parameters.tileSize > (int)textureSize.y)
					tileSizeLimited.y = parameters.tileSize + (textureSize.y - (tileStartInInputTexture.y + parameters.tileSize));

				// Copy into our workbuffer, un-premultiplying and converting sRGB -> linear.
				for (int y = 0; y < tileSizeLimited.y; ++y)
					for (int x = 0; x < tileSizeLimited.x; ++x)
					{
						uint indexIntoErrorAccumulator = tileStartInErrorAccumulator.y * tileCountX * parameters.tileSize + tileStartInErrorAccumulator.x
							+ y * tileCountX * parameters.tileSize
							+ x;

						int2 inputCoord = tileStartInInputTexture + int2(x, y);
						uint2 inputCoordClamped(max(0, inputCoord.x), max(0, inputCoord.y));
						half4 input = inputTexture.read(inputCoordClamped);
						input.rgb /= input.a > 0 ? input.a : 1;

						half3 inputLinear = sRGBToLinearRGB(input.rgb);
						half3 inputYUV = linearRGBToYUV(inputLinear);
						errorAccumulator[indexIntoErrorAccumulator] = inputYUV;
					}


				for (int y = tileSizeLimited.y - 1; y >= 0; --y)
					for (int x = 0; x < tileSizeLimited.x; ++x)
					{
						uint2 inputCoord(tileStartInInputTexture.x + x, tileStartInInputTexture.y + y);
						half inputAlpha = inputTexture.read(inputCoord).a;

						uint indexIntoErrorAccumulator = tileStartInErrorAccumulator.y * tileCountX * parameters.tileSize + tileStartInErrorAccumulator.x
							+ y * tileCountX * parameters.tileSize
							+ x;
						half3 input = errorAccumulator[indexIntoErrorAccumulator];

						// Quantize to the nearest 3-channel-composite color:
						half4 nearestPaletteColor = findNearestPaletteColor(parameters, palette, input);

						// Then distribute quantization error individually per channel:
						half3 cError = input.rgb - nearestPaletteColor.rgb;

						// Clamp the distributed error to some arbitrary range, to keep too much error from accumulating,
						// since that manifests as certain high-contrast transitions drooping/sagging/bleeding toward the southeast.
						half di2 = parameters.ditheringIntensity / 2.;
						cError.r = max(-di2, min(di2, cError.r));
						cError.g = max(-di2, min(di2, cError.g));
						cError.b = max(-di2, min(di2, cError.b));
						cError *= parameters.ditheringIntensity;

						// Colors in transparent pixels shouldn't influence nearby opaque pixels.
						cError *= inputAlpha;

						// Using a hardcoded inner loop size (4) here improves performance by about 8%.
						//    - baseline     — GPUTime  9.43 msec/frame
						//    - static count — GPUTime  8.67 msec/frame
						// for (uint i = 0; i < parameters.diffusionMatrix.stepCount; ++i)
						//     adjust(errorAccumulator, parameters, textureSize, tileStartInErrorAccumulator, tileCountX, i, x, y, cError);
						for (uint outer = 0; outer < parameters.diffusionMatrix.stepCount; outer += 4)
							for (uint i = 0; i < 4; ++i)
								adjust(errorAccumulator, parameters, textureSize, tileStartInErrorAccumulator, tileCountX, outer + i, x, y, cError);

						half4 output = nearestPaletteColor;

						if ((gid.x == 0 || x >= parameters.cropShift) && x < parameters.tileStride + parameters.cropShift
						 && y < parameters.tileStride)
						{
							output.rgb = yuvToLinearRGB(output.rgb);
							output.rgb = linearRGBToSRGB(output.rgb);

							output.a *= inputAlpha;
							output.rgb *= output.a;
							outputTexture.write(output, inputCoord);
						}
					}
			}
		);

		MTLCompileOptions *compileOptions = [MTLCompileOptions new];
		compileOptions.languageVersion = MTLLanguageVersion1_1;  // For consistency between Vuo CLI (which seems to default to latest macOS-supported MSL version) and Vuo editor (which seems to default to an earlier MSL version, possibly related to CMAKE_OSX_DEPLOYMENT_TARGET).

		NSError *error = nil;
		id<MTLLibrary> library = [_device newLibraryWithSource:shaderSource options:compileOptions error:&error];
		if (!library)
		{
			VUserLog("Error: Couldn't initialize MTLLibrary: %s, shader source: {{%s}}", error.localizedDescription.UTF8String, shaderSource.UTF8String);
			return nil;
		}
		if (error)
			VUserLog("Warning: %s, shader source: {{%s}}", error.localizedDescription.UTF8String, shaderSource.UTF8String);
		[compileOptions release];

		id<MTLFunction> errorDiffusionFunction = [library newFunctionWithName:@"errorDiffusion"];
		if (errorDiffusionFunction == nil)
		{
			VUserLog("Error: Couldn't find the errorDiffusion function in the MTLLibrary.");
			return nil;
		}

		error = nil;
		_pipelineState = [_device newComputePipelineStateWithFunction:errorDiffusionFunction error:&error];
		[library release];
		if (!_pipelineState)
		{
			VUserLog("Error: Couldn't create MTLComputePipelineState: %s", error.localizedDescription.UTF8String);
			return nil;
		}

		if ([NSProcessInfo.processInfo isOperatingSystemAtLeastVersion:(NSOperatingSystemVersion){10, 13, 0}])
			_hazardTracking = MTLResourceHazardTrackingModeUntracked;

		_parametersBuffer = [_device newBufferWithLength:sizeof(VuoErrorDiffusionDitherParameters) options: MTLResourceStorageModeShared | MTLResourceCPUCacheModeWriteCombined | _hazardTracking];
	}
	return self;
}

- (IOSurfaceRef)processImage:(IOSurfaceRef)image palette:(VuoList_VuoColor)palette colorDistanceThreshold:(float)colorDistanceThreshold diffusionMatrix:(VuoDitherDiffusionMatrix)diffusionMatrix ditheringIntensity:(float)ditheringIntensity
{
	@autoreleasepool {
		// Validate the sparse matrix:
		uint8_t weightSum = 0;
		for (uint8_t stepIndex = 0; stepIndex < VuoErrorDiffusionDither_maxSteps; ++stepIndex)
		{
			if (diffusionMatrix.steps[stepIndex].weight == 0)
				break;
			weightSum += diffusionMatrix.steps[stepIndex].weight;
		}
		if (weightSum > diffusionMatrix.divisor)
		{
			VUserLog("Error: The specified diffusionMatrix has weights that sum to greater than its divisor (i.e. it's nonconvergent).");
			return nil;
		}


		id<MTLCommandBuffer> commandBuffer = _commandQueue.commandBuffer;

		size_t width  = IOSurfaceGetWidth(image);
		size_t height = IOSurfaceGetHeight(image);
		MTLTextureDescriptor *outputTextureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:width height:height mipmapped:NO];
		outputTextureDescriptor.usage = MTLTextureUsageShaderWrite;
		if (_hazardTracking)
			outputTextureDescriptor.resourceOptions = MTLStorageModeManaged | _hazardTracking;
		IOSurfaceRef surf = IOSurfaceCreate((CFDictionaryRef)@{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
			(NSString *)kIOSurfaceIsGlobal:        @YES,
#pragma clang diagnostic pop
			(NSString *)kIOSurfaceWidth:           @(width),
			(NSString *)kIOSurfaceHeight:          @(height),
			(NSString *)kIOSurfaceBytesPerElement: @4,
		});
		id<MTLTexture> outputTexture = [_device newTextureWithDescriptor:outputTextureDescriptor iosurface:surf plane:0];

		MTLTextureDescriptor *inputTextureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:width height:height mipmapped:NO];
		if (_hazardTracking)
			inputTextureDescriptor.resourceOptions = MTLStorageModeManaged | _hazardTracking;
		id<MTLTexture> inputTexture = [_device newTextureWithDescriptor:inputTextureDescriptor iosurface:image plane:0];

		id<MTLComputeCommandEncoder> computeEncoder = commandBuffer.computeCommandEncoder;
		[computeEncoder setComputePipelineState:_pipelineState];

		VuoErrorDiffusionDitherParameters parameters;

		// See `VuoErrorDiffusionDitherParameters` above for explanation.
		parameters.tileSize = 48;
		parameters.tileStride = 40;
		parameters.cropShift = 6;

		// On each frame, randomly shift the grid,
		// to reduce the persistence-of-vision effect on the tile boundaries.
		parameters.gridShiftX = VuoInteger_random(0, parameters.tileStride - 1);
		parameters.gridShiftY = VuoInteger_random(0, parameters.tileStride - 1);

		// Higher values make the tile boundaries more obvious.
		parameters.ditheringIntensity = ditheringIntensity * .95;

		parameters.paletteCount = VuoListGetCount_VuoColor(palette);
		parameters.colorDistanceThresholdSquared = colorDistanceThreshold * colorDistanceThreshold;
		parameters.diffusionMatrix = diffusionMatrix;

		memcpy(_parametersBuffer.contents, &parameters, sizeof(parameters));
		[computeEncoder setBuffer:_parametersBuffer offset:0 atIndex:0];

		[computeEncoder setTexture:inputTexture atIndex:0];

		// Add 1 in each dimension to leave room for shifting the grid.
		unsigned int tileCountX = ceilf((float)width  / parameters.tileStride) + 1;
		unsigned int tileCountY = ceilf((float)height / parameters.tileStride) + 1;

		int sizeofHalf3 = 8;  // Metal Shading Language Specification, Table 2.3
		size_t errorAccumulatorBufferSize = sizeofHalf3 * parameters.tileSize * parameters.tileSize * tileCountX * tileCountY;
		if (!_errorAccumulatorBuffer || errorAccumulatorBufferSize != _errorAccumulatorBuffer.length)
		{
			[_errorAccumulatorBuffer release];
			_errorAccumulatorBuffer = [_device newBufferWithLength:errorAccumulatorBufferSize options: MTLResourceStorageModePrivate | _hazardTracking];
		}
		[computeEncoder setBuffer:_errorAccumulatorBuffer offset:0 atIndex:1];

		VuoColor *paletteDataSRGB = VuoListGetData_VuoColor(palette);
		unsigned int paletteCount = VuoListGetCount_VuoColor(palette);
		size_t paletteBufferSize = sizeof(float) * 4 * paletteCount;
		if (!_paletteBuffer || paletteBufferSize != _paletteBuffer.length)
		{
			[_paletteBuffer release];
			_paletteBuffer = [_device newBufferWithLength:paletteBufferSize options: MTLResourceStorageModeShared | MTLResourceCPUCacheModeWriteCombined | _hazardTracking];
		}
		for (int i = 0; i < paletteCount; ++i)
			((VuoColor *)_paletteBuffer.contents)[i] = VuoErrorDiffusionDither_linearRGBToYUV(VuoErrorDiffusionDither_sRGBToLinearRGB(paletteDataSRGB[i]));
		[computeEncoder setBuffer:_paletteBuffer offset:0 atIndex:2];

		[computeEncoder setTexture:outputTexture atIndex:1];

		// For a 1280x720 image (33x19 = 627 tiles) on Apple M1 Pro:
		//    - threadsPerThreadgroup= 1 — GPUTime 11.46 msec/frame
		//    - threadsPerThreadgroup= 4 — GPUTime 11.18 msec/frame
		//    - threadsPerThreadgroup= 8 — GPUTime 11.26 msec/frame
		//    - threadsPerThreadgroup=32 — GPUTime 18.80 msec/frame
		if ([NSProcessInfo.processInfo isOperatingSystemAtLeastVersion:(NSOperatingSystemVersion){10, 13, 0}])
			[computeEncoder dispatchThreads:MTLSizeMake(tileCountX, tileCountY, 1) threadsPerThreadgroup:MTLSizeMake(4, 4, 1)];
		else
			[computeEncoder dispatchThreadgroups:MTLSizeMake(tileCountX, tileCountY, 1) threadsPerThreadgroup:MTLSizeMake(4, 4, 1)];

		[computeEncoder endEncoding];

		[commandBuffer commit];
		[commandBuffer waitUntilCompleted];
		[outputTexture release];
		[inputTexture release];
		return surf;
	}
}

- (void)dealloc
{
	[_pipelineState release];
	[_commandQueue release];
	[_errorAccumulatorBuffer release];
	[_paletteBuffer release];
	[_parametersBuffer release];
	[_device release];

	[super dealloc];
}
@end

/**
 * Creates a state object for performing error-diffusion dithering.
 */
VuoErrorDiffusionDither VuoErrorDiffusionDither_make(void)
{
	void *edd = [VuoErrorDiffusionDither_internal new];
	VuoRegister(edd, (DeallocateFunctionType)CFRelease);
	return edd;
}

/**
 * Performs error-diffusion dithering on an image.
 */
IOSurfaceRef VuoErrorDiffusionDither_processImage(VuoErrorDiffusionDither edd, IOSurfaceRef image, VuoList_VuoColor palette, float colorDistanceThreshold, VuoDitherDiffusionMatrix diffusionMatrix, float ditheringIntensity)
{
	VuoErrorDiffusionDither_internal *eddi = edd;
	return [eddi processImage:image palette:palette colorDistanceThreshold:colorDistanceThreshold diffusionMatrix:diffusionMatrix ditheringIntensity:ditheringIntensity];
}

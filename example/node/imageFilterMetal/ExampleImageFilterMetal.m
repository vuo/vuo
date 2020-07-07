/**
 * @file
 * ExampleImageFilterMetal implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#import "ExampleImageFilterMetal.h"

#import "node.h"

#import <Metal/Metal.h>
#import <simd/vector_types.h>

#define STRING(source) @#source

typedef struct
{
    float phase;
} Uniforms;

@interface ExampleImageFilterMetal : NSObject
@property(retain) id<MTLDevice> device;
@property(retain) id<MTLCommandQueue> commandQueue;
@property(retain) id<MTLRenderPipelineState> pipelineState;
@end

@implementation ExampleImageFilterMetal
- (id)init
{
	if (self = [super init])
	{
		_device = MTLCreateSystemDefaultDevice();
		_commandQueue = [_device newCommandQueue];
		NSError *e = nil;
		id<MTLLibrary> library;

		NSString *lib = STRING(
			\n#include <metal_stdlib>\n
			using namespace metal;

			typedef struct
			{
				float4 clipSpacePosition [[position]];
				float2 textureCoordinate;
			} RasterizerData;

			vertex RasterizerData vertexShader(uint vertexID [[vertex_id]])
			{
				RasterizerData out;
				out.clipSpacePosition.x = -1. + float((vertexID & 1) << 2);
				out.clipSpacePosition.y = -1. + float((vertexID & 2) << 1);
				out.clipSpacePosition.z = 0.0;
				out.clipSpacePosition.w = 1.0;
				out.textureCoordinate.x = float((vertexID & 1) << 1);
				out.textureCoordinate.y = 1 - float((vertexID & 2));
				return out;
			}
		);

		library = [_device newLibraryWithSource:[lib stringByAppendingString:STRING(
			typedef struct
			{
			    float phase;
			} Uniforms;

			fragment half4 fragmentShader(RasterizerData in [[stage_in]],
				constant Uniforms &u [[ buffer(0) ]],
				texture2d<half> colorTexture [[ texture(0) ]])
			{
				constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
				return colorTexture.sample(textureSampler, in.textureCoordinate + float2(0, sin(in.textureCoordinate.x * 3.14*2 + u.phase)/10));
			}
		)] options:nil error:&e];
		if (e)
		{
			NSLog(@"%@",e);
			return 0;
		}

		MTLRenderPipelineDescriptor *pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
		pipelineStateDescriptor.vertexFunction = [[library newFunctionWithName:@"vertexShader"] autorelease];
		pipelineStateDescriptor.fragmentFunction = [[library newFunctionWithName:@"fragmentShader"] autorelease];
		pipelineStateDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
		[library release];

		NSError *error = nil;
		_pipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
		[pipelineStateDescriptor release];
		if (!_pipelineState)
		{
			NSLog(@"Failed to created pipeline state, error %@", error);
			return 0;
		}
	}
	return self;
}

- (IOSurfaceRef)processImage:(IOSurfaceRef)image phase:(float)phase
{
	@autoreleasepool {
		id<MTLCommandBuffer> commandBuffer = _commandQueue.commandBuffer;

		size_t width  = IOSurfaceGetWidth(image);
		size_t height = IOSurfaceGetHeight(image);
		MTLTextureDescriptor *outputTextureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:width height:height mipmapped:NO];
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

		MTLRenderPassDescriptor *renderpass = MTLRenderPassDescriptor.renderPassDescriptor;
		renderpass.colorAttachments[0].texture = outputTexture;
		renderpass.colorAttachments[0].storeAction = MTLStoreActionStore;
		id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderpass];
		[renderEncoder setRenderPipelineState:_pipelineState];

		MTLTextureDescriptor *inputTextureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:width height:height mipmapped:NO];
		id<MTLTexture> inputTexture = [_device newTextureWithDescriptor:inputTextureDescriptor iosurface:image plane:0];
		[renderEncoder setFragmentTexture:inputTexture atIndex:0];

		Uniforms u = { phase };
		[renderEncoder setFragmentBytes:&u length:sizeof(u) atIndex:0];

		[renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:3];

		[renderEncoder endEncoding];

		[commandBuffer commit];
		[outputTexture release];
		[inputTexture release];
		return surf;
	}
}

- (void)dealloc
{
	[_pipelineState release];
	[_commandQueue release];
	[_device release];

	[super dealloc];
}
@end

void *ExampleImageFilterMetal_make(void)
{
	void *t = [ExampleImageFilterMetal new];
	VuoRegister(t, (DeallocateFunctionType)CFRelease);
	return t;
}

IOSurfaceRef ExampleImageFilterMetal_processImage(void *t, IOSurfaceRef image, float phase)
{
	ExampleImageFilterMetal *eifm = t;
	return [eifm processImage:image phase:phase];
}

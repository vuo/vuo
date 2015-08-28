/**
 * @file
 * RunImageFilter implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#define __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES 0
#import <Cocoa/Cocoa.h>
#import <CoreVideo/CoreVideo.h>

#import <OpenGL/CGLMacro.h>
#define glGenVertexArrays glGenVertexArraysAPPLE
#define glBindVertexArray glBindVertexArrayAPPLE
#define glDeleteVertexArrays glDeleteVertexArraysAPPLE

#include <OpenGL/CGLCurrent.h>

#include <Vuo/Vuo.h>


#define GLSL_STRING(version,source) "#version " #version "\n" #source

static const char * vertexShaderSource = GLSL_STRING(120,
	attribute vec2 position;
	varying vec2 texcoord;

	void main()
	{
		gl_Position = vec4(position, 0.0, 1.0);
		texcoord = position * vec2(0.5) + vec2(0.5);
	}
);

static const char * fragmentShaderSource = GLSL_STRING(120,
	uniform sampler2D texture;
	varying vec2 texcoord;

	void main()
	{
		gl_FragColor = texture2D(texture, texcoord);
	}
);

static GLuint compileShader(CGLContextObj cgl_ctx, GLenum type, const char * source)
{
	GLint length = strlen(source);
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, (const GLchar**)&source, &length);
	glCompileShader(shader);
	return shader;
}

static const GLfloat quadPositions[] = { -1, -1, 1, -1, -1, 1, 1, 1 };
static const GLushort quadElements[] = { 0, 1, 2, 3 };



@interface RunImageFilterView : NSOpenGLView
{
	VuoRunner * runner;
	VuoRunner::Port * inputImagePort;

	VuoImage outputImage;

	CVDisplayLinkRef displayLink;
	bool displayLinkShouldStop;
	NSCondition *displayLinkStopped;
}
@end

@implementation RunImageFilterView

- (void)awakeFromNib
{
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowWillClose:) name:NSWindowWillCloseNotification object:[self window]];

	// Create a context with the pixelformat Vuo uses.
	NSOpenGLPixelFormatAttribute attrs[] =
	{
		NSOpenGLPFAAccelerated,
		NSOpenGLPFAWindow,
		NSOpenGLPFANoRecovery,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAColorSize, 24,
		NSOpenGLPFADepthSize, 16,
		// Multisampling breaks point rendering on some GPUs.  https://b33p.net/kosada/node/8225#comment-31324
//		NSOpenGLPFAMultisample,
//		NSOpenGLPFASampleBuffers, 1,
//		NSOpenGLPFASamples, 4,
		0
	};
	NSOpenGLPixelFormat *pf = [[[NSOpenGLPixelFormat alloc] initWithAttributes:attrs] autorelease];
	NSOpenGLContext *context = [[[NSOpenGLContext alloc] initWithFormat:pf shareContext:nil] autorelease];
	[self setPixelFormat:pf];
	[self setOpenGLContext:context];
}

- (void)prepareOpenGL
{
	CGLContextObj cgl_ctx = (CGLContextObj)[[self openGLContext] CGLContextObj];

	// Share the GL Context with the Vuo Composition
	VuoGlContext_setGlobalRootContext((void *)cgl_ctx);

	// Compile, link, and run the composition
	const char *compositionFile = [[[NSBundle mainBundle] pathForResource:@"RippleImage" ofType:@"vuo"] UTF8String];
	runner = VuoCompiler::newCurrentProcessRunnerFromCompositionFile(compositionFile);
	runner->start();

	// Upload a quad, for rendering the texture later on
	GLuint vertexArray;
	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);

	GLuint quadPositionBuffer;
	glGenBuffers(1, &quadPositionBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, quadPositionBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadPositions), quadPositions, GL_STATIC_DRAW);

	GLuint quadElementBuffer;
	glGenBuffers(1, &quadElementBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadElementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadElements), quadElements, GL_STATIC_DRAW);

	// Load an image from disk into a GL Texture
	GLuint inputTexture;
	glGenTextures(1,&inputTexture);
	glBindTexture(GL_TEXTURE_2D, inputTexture);
	NSImage * ni = [NSImage imageNamed:@"OttoOperatesTheRoller.jpg"];
	NSImage * niFlipped = [[NSImage alloc] initWithSize:[ni size]];
	float scale = 1;
	if ([niFlipped respondsToSelector:@selector(recommendedLayerContentsScale:)])
	{
		// If we're on 10.7 or later, we need to check whether we're running on a retina display, and scale accordingly if so.
		typedef CGFloat (*funcType)(id receiver, SEL selector, CGFloat);
		funcType recommendedLayerContentsScale = (funcType)[[niFlipped class] instanceMethodForSelector:@selector(recommendedLayerContentsScale:)];
		scale = recommendedLayerContentsScale(niFlipped, @selector(recommendedLayerContentsScale:), 0);
	}
	[niFlipped setFlipped:YES];
	[niFlipped lockFocus];
	[ni drawInRect:NSMakeRect(0,0,[ni size].width,[ni size].height) fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:1.0];
	[niFlipped unlockFocus];
	NSBitmapImageRep * nbir = [NSBitmapImageRep imageRepWithData:[niFlipped TIFFRepresentation]];
	GLenum internalformat = GL_RGBA;
	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, [ni size].width*scale, [ni size].height*scale, 0, GL_RGBA, GL_UNSIGNED_BYTE, [nbir bitmapData]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	[niFlipped release];

	// Prepare a shader for displaying the GL Texture onscreen
	GLint vertexShader = compileShader(cgl_ctx, GL_VERTEX_SHADER, vertexShaderSource);
	GLint fragmentShader = compileShader(cgl_ctx, GL_FRAGMENT_SHADER, fragmentShaderSource);

	GLint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	glUseProgram(program);

	GLint positionAttribute = glGetAttribLocation(program, "position");
	glVertexAttribPointer(positionAttribute, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*2, (void*)0);
	glEnableVertexAttribArray(positionAttribute);

	GLint textureUniform = glGetUniformLocation(program, "texture");
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(textureUniform, 0);

	// Pass the GL Texture to the Vuo Composition
	inputImagePort = runner->getPublishedInputPortWithName("inputImage");
	VuoImage t = VuoImage_make(inputTexture, internalformat, [ni size].width, [ni size].height);
	VuoRetain(t);
	json_object *o = VuoImage_jsonFromValue(t);
	runner->setPublishedInputPortValue(inputImagePort, o);
	json_object_put(o);

	// Call RunImageFilterViewDisplayCallback every vertical refresh
	displayLinkShouldStop = NO;
	displayLinkStopped = [[NSCondition alloc] init];
	
	CVDisplayLinkCreateWithCGDisplay(CGMainDisplayID(), &displayLink);
	CVDisplayLinkSetOutputCallback(displayLink, RunImageFilterViewDisplayCallback, self);
	CVDisplayLinkStart(displayLink);
}

CVReturn RunImageFilterViewDisplayCallback(CVDisplayLinkRef displayLink, const CVTimeStamp *inNow, const CVTimeStamp *inOutputTime, CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *displayLinkContext)
{
	RunImageFilterView * view = (RunImageFilterView *)displayLinkContext;
	NSAutoreleasePool * p = [[NSAutoreleasePool alloc] init];

	CGLContextObj cgl_ctx = (CGLContextObj)[[view openGLContext] CGLContextObj];

	// Display the latest GL Texture onscreen
	if (view->outputImage)
	{
		glBindTexture(GL_TEXTURE_2D, view->outputImage->glTextureName);
		glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, (void*)0);
	}

	[[view openGLContext] flushBuffer];

	VuoRelease(view->outputImage);
	view->outputImage = nil;

	// Execute the Vuo Composition
	view->runner->firePublishedInputPortEvent(view->inputImagePort);
	view->runner->waitForAnyPublishedOutputPortEvent();

	// Retrieve the output GL Texture from the Vuo Composition, to be displayed during the next vertical refresh
	VuoRunner::Port * outputImagePort = view->runner->getPublishedOutputPortWithName("outputImage");
	json_object *o = view->runner->getPublishedOutputPortValue(outputImagePort);
	view->outputImage = VuoImage_valueFromJson(o);
	VuoRetain(view->outputImage);
	json_object_put(o);

	if (view->displayLinkShouldStop)
	{
		CVDisplayLinkStop(displayLink);
		view->runner->stop();
		[view->displayLinkStopped lock];
		[view->displayLinkStopped signal];
		[view->displayLinkStopped unlock];
	}
	[p drain];
	return kCVReturnSuccess;
}

- (void)windowWillClose:(NSNotification *)notification
{
	[displayLinkStopped lock];
	displayLinkShouldStop = YES;

	// Ensure the CVDisplayLink has finished executing for the last time before continuing.
	[displayLinkStopped wait];
	[displayLinkStopped unlock];
}

@end

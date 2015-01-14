/**
 * @file
 * RunImageFilter implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#define __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES 0
#import <Cocoa/Cocoa.h>
#import <CoreVideo/CoreVideo.h>

#import <OpenGL/gl.h>
//#import <OpenGL/gl3.h>
/// @todo After we drop 10.6 support, switch back to gl3 and remove the below 4 lines.  See also r15430 for shader changes.
#include <OpenGL/glext.h>
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

static GLuint compileShader(GLenum type, const char * source)
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
	GLuint vertexArray;
	GLuint quadPositionBuffer;
	GLuint quadElementBuffer;
	GLuint inputTexture;
	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint program;
	VuoRunner::Port * inputImagePort;
	GLint positionAttribute;
	GLint textureUniform;
}
@end

@implementation RunImageFilterView

- (void)awakeFromNib
{
	// Create a context with the pixelformat Vuo uses.
	NSOpenGLPixelFormatAttribute attrs[] =
	{
		NSOpenGLPFAAccelerated,
		NSOpenGLPFAWindow,
		NSOpenGLPFANoRecovery,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAColorSize, 24,
		NSOpenGLPFADepthSize, 16,
		NSOpenGLPFAMultisample,
		NSOpenGLPFASampleBuffers, 1,
		NSOpenGLPFASamples, 4,
		0
	};
	NSOpenGLPixelFormat *pf = [[[NSOpenGLPixelFormat alloc] initWithAttributes:attrs] autorelease];
	NSOpenGLContext *context = [[[NSOpenGLContext alloc] initWithFormat:pf shareContext:nil] autorelease];
	[self setPixelFormat:pf];
	[self setOpenGLContext:context];
}

- (void)prepareOpenGL
{
	// Share the GL Context with the Vuo Composition
	CGLContextObj ctx = CGLGetCurrentContext();
	VuoGlContext_setGlobalRootContext((void *)ctx);

	// Compile, link, and run the composition
	const char *compositionFile = [[[NSBundle mainBundle] pathForResource:@"RippleImage" ofType:@"vuo"] UTF8String];
	runner = VuoCompiler::newCurrentProcessRunnerFromCompositionFile(compositionFile);
	runner->start();

	// Restore the context (Vuo node initializations may reset it).
	CGLSetCurrentContext(ctx);

	// Upload a quad, for rendering the texture later on
	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);

	glGenBuffers(1, &quadPositionBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, quadPositionBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadPositions), quadPositions, GL_STATIC_DRAW);

	glGenBuffers(1, &quadElementBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadElementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadElements), quadElements, GL_STATIC_DRAW);

	// Load an image from disk into a GL Texture
	glGenTextures(1,&inputTexture);
	glBindTexture(GL_TEXTURE_2D, inputTexture);
	NSImage * ni = [NSImage imageNamed:@"OttoOperatesTheRoller.jpg"];
	NSImage * niFlipped = [[NSImage alloc] initWithSize:[ni size]];
	float scale = 1;
#if MAC_OS_X_VERSION_MIN_REQUIRED > MAC_OS_X_VERSION_10_6
	// If we're building on 10.7 or later, we need to check whether we're running on a retina display, and scale accordingly if so.
	scale = [niFlipped recommendedLayerContentsScale:0];
#endif
	[niFlipped setFlipped:YES];
	[niFlipped lockFocus];
	[ni drawInRect:NSMakeRect(0,0,[ni size].width,[ni size].height) fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:1.0];
	[niFlipped unlockFocus];
	NSBitmapImageRep * nbir = [NSBitmapImageRep imageRepWithData:[niFlipped TIFFRepresentation]];
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, [ni size].width*scale, [ni size].height*scale, 0, GL_RGBA, GL_UNSIGNED_BYTE, [nbir bitmapData]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	[niFlipped release];

	// Prepare a shader for displaying the GL Texture onscreen
	vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
	fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

	program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	positionAttribute = glGetAttribLocation(program, "position");
	textureUniform = glGetUniformLocation(program, "texture");

	// Pass the GL Texture to the Vuo Composition
	inputImagePort = runner->getPublishedInputPortWithName("inputImage");
	VuoImage t = VuoImage_make(inputTexture, [ni size].width, [ni size].height);
	VuoRetain(t);
	json_object *o = VuoImage_jsonFromValue(t);
	runner->setPublishedInputPortValue(inputImagePort, o);
	json_object_put(o);

	// Call RunImageFilterViewDisplayCallback every vertical refresh
	CVDisplayLinkRef displayLink;
	CVDisplayLinkCreateWithCGDisplay(CGMainDisplayID(), &displayLink);
	CVDisplayLinkSetOutputCallback(displayLink, RunImageFilterViewDisplayCallback, self);
	CVDisplayLinkStart(displayLink);
}

CVReturn RunImageFilterViewDisplayCallback(CVDisplayLinkRef displayLink, const CVTimeStamp *inNow, const CVTimeStamp *inOutputTime, CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *displayLinkContext)
{
	RunImageFilterView * view = (RunImageFilterView *)displayLinkContext;
	NSAutoreleasePool * p = [[NSAutoreleasePool alloc] init];
	[view setNeedsDisplay:YES];
	[p drain];
	return kCVReturnSuccess;
}


- (void)drawRect:(NSRect)dirtyRect
{
	// Execute the Vuo Composition
	runner->firePublishedInputPortEvent(inputImagePort);
	runner->waitForAnyPublishedOutputPortEvent();

	// Retrieve the output GL Texture from the Vuo Composition
	VuoRunner::Port * outputImagePort = runner->getPublishedOutputPortWithName("outputImage");
	json_object *o = runner->getPublishedOutputPortValue(outputImagePort);
	VuoImage outputImage = VuoImage_valueFromJson(o);
	json_object_put(o);
	VuoRetain(outputImage);

	// Display the GL Texture onscreen
	glUseProgram(program);
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, outputImage->glTextureName);
		glUniform1i(textureUniform, 0);

		glBindBuffer(GL_ARRAY_BUFFER, quadPositionBuffer);
		glVertexAttribPointer(positionAttribute, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*2, (void*)0);
		glEnableVertexAttribArray(positionAttribute);
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadElementBuffer);
			glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, (void*)0);
		}
		glDisableVertexAttribArray(positionAttribute);
	}
	glUseProgram(0);

	CGLFlushDrawable(CGLGetCurrentContext());

	VuoRelease(outputImage);
}

@end

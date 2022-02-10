/**
 * @file
 * VuoGraphicsLayer implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#import "VuoGraphicsLayer.h"

#import "VuoApp.h"
#import "VuoCompositionState.h"
#import "VuoCglPixelFormat.h"
#import "VuoEventLoop.h"
#import "VuoGraphicsView.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#import <OpenGL/gl.h>
/// @{ Stub.
#define glGenVertexArrays glGenVertexArraysAPPLE
#define glBindVertexArray glBindVertexArrayAPPLE
#define glDeleteVertexArrays glDeleteVertexArraysAPPLE
/// @}

#import <IOSurface/IOSurface.h>

#ifdef VUO_COMPILER
VuoModuleMetadata({
	"title" : "VuoGraphicsLayer",
	"dependencies" : [
		"VuoCglPixelFormat",
		"VuoDisplayRefresh",
		"VuoGraphicsView",
		"OpenGL.framework",
		"QuartzCore.framework",
	]
});
#endif

/**
 * @todo this function is a temporary workaround until VuoShader can handle GLSL 1.50
 */
static GLuint CompileShader(GLenum type, const char *source)
{
	GLint length = (GLint)strlen(source);
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, (const GLchar**)&source, &length);
	glCompileShader(shader);

	int infologLength = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLength);
	if (infologLength > 0)
	{
		char *infoLog = (char *)malloc(infologLength);
		int charsWritten = 0;
		glGetShaderInfoLog(shader, infologLength, &charsWritten, infoLog);
		VUserLog("%s", infoLog);
		free(infoLog);
	}
	return shader;
}

/**
 * Private VuoGraphicsLayer data.
 */
@interface VuoGraphicsLayer ()
///@{ Private VuoGraphicsLayer data.
@property VuoGraphicsWindowInitCallback          initCallback;
@property VuoGraphicsWindowUpdateBackingCallback updateBackingCallback;
@property VuoGraphicsWindowResizeCallback        resizeCallback;
@property VuoGraphicsWindowDrawCallback          drawCallback;
@property void *userData;
///@}

@property dispatch_queue_t drawQueue;         ///< Serializes access to the root context.

@property double renderScheduled;                      ///< The time (@ref VuoLogGetElapsedTime) at which a render was most recently scheduled, or -INFINITY if none is scheduled.

@property VuoDisplayRefresh displayRefresh;   ///< Periodically invokes @ref VuoGraphicsLayer_drawOnIOSurface.

@property GLuint receiveTextureUniform;                ///< The shader's texture unit uniform.
@property GLuint receiveTextureOffsetUniform;          ///< The shader's texture offset uniform.

@property bool firstFrame;              ///< Ensures the resize callback is called before drawing.
@property bool closed;                  ///< True if the window is closed (and should stop rendering).  -isVisible isn't adequate (see https://b33p.net/kosada/node/12732).

@property uint64_t framesRenderedSinceProfileLogged; ///< Incremented every time -draw is called.  When the framerate is logged, this is reset to 0.
@property double lastProfileLoggedTime; ///< The time (in seconds) the last time the framerate was logged.
@end

static void VuoGraphicsLayer_drawOnIOSurface(VuoGraphicsLayer *l);

/**
 * Requests a new IOSurface from drawCallback.
 *
 * @threadQueue{drawQueue}
 */
static void VuoGraphicsLayer_drawOnIOSurface(VuoGraphicsLayer *l)
{
	if (l.ioSurface)
	{
		VuoIoSurfacePool_signal(VuoIoSurfacePool_getIOSurfaceRef(l.ioSurface));
		VuoIoSurfacePool_disuse(l.ioSurface, false);  // Guaranteed to have finished blitting to the window by this point, so no need to quarantine.
		l.ioSurface = NULL;
	}

	void *callerData = l.userData;
	VuoGraphicsWindow *gw = l.window;
	VuoGraphicsView *gv = (VuoGraphicsView *)gw.contentView;

	NSRect frame = l.frame;
	frame.origin.x *= gw.backingScaleFactor;
	frame.origin.y *= gw.backingScaleFactor;
	frame.size.width *= gw.backingScaleFactor;
	frame.size.height *= gw.backingScaleFactor;

	{
		// Dimensions, in pixels (not points).
		int x = 0;
		int y = 0;
		int width = frame.size.width;
		int height = frame.size.height;

		// When fullscreen and aspect-locked, letterbox the scene (render to a centered rectangle that fits the size of the screen and matches the aspect of the now-hidden window).
		NSSize desiredAspectSize = gw.contentAspectRatio;
		bool isAspectLocked = !NSEqualSizes(desiredAspectSize, NSMakeSize(0,0));
		if (gw.isFullScreen && isAspectLocked)
		{
			float desiredAspect = desiredAspectSize.width / desiredAspectSize.height;
			float screenAspect = frame.size.width / frame.size.height;

			if (desiredAspect > screenAspect)
			{
				// Match the screen's width.
				width = frame.size.width;
				height = frame.size.width / desiredAspect;
				x = 0;
				y = frame.size.height/2 - height/2;
			}
			else
			{
				// Match the screen's height.
				width = frame.size.height * desiredAspect;
				height = frame.size.height;
				x = frame.size.width/2 - width/2;
				y = 0;
			}
		}

		// When fullscreen and non-resizable, windowbox the scene (render to a centered rectangle matching the size of the now-hidden window).
		bool isWindowResizable = (gw.isFullScreen ? gw.styleMaskWhenWindowed : gw.styleMask) & NSResizableWindowMask;
		if (gw.isFullScreen && !isWindowResizable)
		{
			NSRect contentRectWhenWindowed = [gv convertRectToBacking:gw.contentRectWhenWindowed];
			width = contentRectWhenWindowed.size.width;
			height = contentRectWhenWindowed.size.height;
			x = frame.size.width/2 - width/2;
			y = frame.size.height/2 - height/2;
		}


		bool backingScaleFactorChanged = (gw.backingScaleFactorCached != gw.backingScaleFactor);
		NSRect newViewport = NSMakeRect(x, y, width, height);
		if (!NSEqualRects(gv.viewport, newViewport) || l.firstFrame || backingScaleFactorChanged)
		{
			// When moving a window between Retina and non-Retina displays,
			// viewDidChangeBackingProperties might be invoked either before or after the next
			// VuoGraphicsLayer_drawOnIOSurface execution.  Ensure that, if the backing has changed,
			// we call backingChangedCallback, then resizeCallback, then drawCallback
			// (since backingChangedCallback creates a new VuoSceneRenderer, without a size).
			// https://b33p.net/kosada/node/12273
			if (backingScaleFactorChanged)
				[l viewDidChangeBackingProperties];

			l.resizeCallback(callerData, width, height);
			gv.viewport = newViewport;
			l.firstFrame = NO;
		}
	}


	// Draw onto the IOSurface FBO.
	VuoIoSurface vis = l.drawCallback(callerData);
	if (!vis)
		return;

	[gw.recorder saveImage:vis];

	l.ioSurface = vis;
}

@implementation VuoGraphicsLayer

/**
 * Creates an OpenGL view that calls the given callbacks for rendering.
 *
 * @threadMain
 */
- (instancetype)initWithWindow:(VuoGraphicsWindow *)window
						initCallback:(VuoGraphicsWindowInitCallback)initCallback
			   updateBackingCallback:(VuoGraphicsWindowUpdateBackingCallback)updateBackingCallback
				  backingScaleFactor:(float)backingScaleFactor
					  resizeCallback:(VuoGraphicsWindowResizeCallback)resizeCallback
						drawCallback:(VuoGraphicsWindowDrawCallback)drawCallback
							userData:(void *)userData
{
	if (self = [super init])
	{
		_window = window;
		_initCallback = initCallback;
		_updateBackingCallback = updateBackingCallback;
		_resizeCallback = resizeCallback;
		_drawCallback = drawCallback;
		_userData = userData;

		if ([self respondsToSelector:@selector(setColorspace:)])
			[self performSelector:@selector(setColorspace:) withObject:(id)CGColorSpaceCreateWithName(kCGColorSpaceSRGB)];

		self.needsDisplayOnBoundsChange = YES;

		/// @todo Remove drawQueue after switching to single-context, since VuoGl_perform() will take care of that.
		_drawQueue = dispatch_queue_create("org.vuo.VuoGraphicsLayer", VuoEventLoop_getDispatchInteractiveAttribute());

		_renderScheduled = VuoLogGetElapsedTime();

		_firstFrame = YES;

		_initCallback(_userData, backingScaleFactor);

		_displayRefresh = VuoDisplayRefresh_make(self);
		VuoRetain(_displayRefresh);

		_lastProfileLoggedTime = VuoLogGetTime();
	}
	return self;
}

/**
 * Reimplemented from CAOpenGLLayer.
 */
- (CGLPixelFormatObj)copyCGLPixelFormatForDisplayMask:(uint32_t)mask
{
	return VuoGlContext_makePlatformPixelFormat(false, false, -1);
}

/**
 * Apple says, "This is also an ideal location to do any initialization that is necessary for the context returned".
 *
 * Reimplemented from CAOpenGLLayer.
 */
- (CGLContextObj)copyCGLContextForPixelFormat:(CGLPixelFormatObj)pixelFormat
{
	CGLContextObj context = [super copyCGLContextForPixelFormat:pixelFormat];

	// Initialize stuff for rendering the IOSurface to the window.
	{
		CGLSetCurrentContext(context);

		GLuint vertexArray;
		glGenVertexArrays(1, &vertexArray);
		glBindVertexArray(vertexArray);

		const GLfloat trianglePositions[] = {
			-1, -1,
			 3, -1,
			-1,  3,
		};
		GLuint trianglePositionBuffer;
		glGenBuffers(1, &trianglePositionBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, trianglePositionBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(trianglePositions), trianglePositions, GL_STATIC_DRAW);
		VuoGlPool_logVRAMAllocated(sizeof(trianglePositions));

		const char *vertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
			attribute vec2 position;
			void main()
			{
				gl_Position = vec4(position.x, position.y, 0., 1.);
			}
		);
		const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
			uniform sampler2DRect t;
			uniform vec2 textureOrigin;
			void main()
			{
				gl_FragColor = texture2DRect(t, gl_FragCoord.xy - textureOrigin);
			}
		);
		GLuint   vertexShader = CompileShader(GL_VERTEX_SHADER,     vertexShaderSource);
		GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
		GLuint program = glCreateProgram();
		glAttachShader(program, vertexShader);
		glAttachShader(program, fragmentShader);
		glLinkProgram(program);

		GLuint positionAttribute = glGetAttribLocation(program, "position");

		_receiveTextureUniform = glGetUniformLocation(program, "t");
		_receiveTextureOffsetUniform = glGetUniformLocation(program, "textureOrigin");
		glUseProgram(program);
		glVertexAttribPointer(positionAttribute, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*2, (void*)0);
		glEnableVertexAttribArray(positionAttribute);

		CGLSetCurrentContext(NULL);
	}

	return context;
}

/**
 * Called by @ref VuoGraphicsView.
 *
 * @threadMain
 */
- (void)viewDidChangeBackingProperties
{
	float backingScaleFactor = _window.backingScaleFactor;
	if (backingScaleFactor != _window.backingScaleFactorCached)
	{
		VUserLog("backingScaleFactor changed from %g to %g", _window.backingScaleFactorCached, backingScaleFactor);
		float oldBackingScaleFactor = _window.backingScaleFactorCached;
		NSSize contentSize = [_window contentRectForFrameRect:_window.frame].size;

		_window.backingScaleFactorCached = backingScaleFactor;
		self.contentsScale = backingScaleFactor;

		_updateBackingCallback(_userData, backingScaleFactor);

		if (_window.maintainsPixelSizeWhenBackingChanges)
		{
			float backingScaleRatio = oldBackingScaleFactor / backingScaleFactor;
			[_window scheduleResize:NSMakeSize(contentSize.width  * backingScaleRatio,
											   contentSize.height * backingScaleRatio)];
		}

		// _updateBackingCallback destroys and recreates the VuoSceneRenderer;
		// ensure the new one knows its size.
		NSSize newContentSize = [_window contentRectForFrameRect:_window.frame].size;
		_resizeCallback(_userData, newContentSize.width  * _window.backingScaleFactorCached,
								   newContentSize.height * _window.backingScaleFactorCached);
	}
}

/**
 * @threadAny
 */
- (void)enableTriggers
{
	VuoGraphicsWindow *gw = self.window;
	VuoDisplayRefresh_enableTriggers(_displayRefresh, gw.requestedFrame, NULL);
}

/**
 * @threadAny
 */
- (void)disableTriggers
{
	VuoDisplayRefresh_disableTriggers(_displayRefresh);
}

/**
 * Reimplemented from CALayer.
 */
- (void)setBounds:(CGRect)bounds
{
	[super setBounds:bounds];
}

/**
 * Schedules the view to be repainted (which invokes the draw callback and blits it to the window).
 *
 * @threadAny
 */
- (void)draw
{
	double now = VuoLogGetElapsedTime();
	if (now - _renderScheduled > 0.4)
	{
		_renderScheduled = now;
		dispatch_async(dispatch_get_main_queue(), ^{
			[self setNeedsDisplay];
		});
	}
}

/**
 * Cocoa calls this method when it needs us to repaint the view.
 *
 * Reimplemented from CAOpenGLLayer.
 * @threadMain
 */
- (void)drawInCGLContext:(CGLContextObj)context pixelFormat:(CGLPixelFormatObj)pixelFormat forLayerTime:(CFTimeInterval)timeInterval displayTime:(const CVTimeStamp *)timeStamp
{
	_renderScheduled = -INFINITY;

	if (_closed)
		return;

	dispatch_sync(_drawQueue, ^{
		VuoGraphicsLayer_drawOnIOSurface(self);
		if (!_ioSurface)
			return;

		NSRect frameInPoints = self.frame;
		double s = _window.backingScaleFactor;
		NSRect frameInPixels = NSMakeRect(frameInPoints.origin.x * s, frameInPoints.origin.y * s, frameInPoints.size.width * s, frameInPoints.size.height * s);
		size_t ioSurfaceWidth  = VuoIoSurfacePool_getWidth(_ioSurface);
		size_t ioSurfaceHeight = VuoIoSurfacePool_getHeight(_ioSurface);

		VuoGraphicsView *gv = (VuoGraphicsView *)_window.contentView;
		NSRect viewport = gv.viewport;

		// Prepare a texture to receive the IOSurface onto the local OpenGL context.
		GLuint receiveTexture;
		GLuint receiveTextureBytes;
		{
			glGenTextures(1, &receiveTexture);  // This can't participate in the VuoGlTexturePool since it's not on Vuo's shared context.
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_RECTANGLE_EXT, receiveTexture);
			glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//			glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glUniform1i(_receiveTextureUniform, 0);

			CGLError err = CGLTexImageIOSurface2D(context, GL_TEXTURE_RECTANGLE_EXT, GL_RGB, ioSurfaceWidth, ioSurfaceHeight, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, VuoIoSurfacePool_getIOSurfaceRef(_ioSurface), 0);
			if(err != kCGLNoError)
			{
				VUserLog("Error in CGLTexImageIOSurface2D(context, GL_TEXTURE_RECTANGLE_EXT, GL_RGB, %zu, %zu, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, %d, 0) 2: %s", ioSurfaceWidth, ioSurfaceHeight, VuoIoSurfacePool_getId(_ioSurface), CGLErrorString(err));
				return;
			}

			receiveTextureBytes = ioSurfaceWidth * ioSurfaceHeight * 4;
			VuoGlPool_logVRAMAllocated(receiveTextureBytes);

			glUniform2f(_receiveTextureOffsetUniform, viewport.origin.x, viewport.origin.y);
			glViewport(viewport.origin.x, viewport.origin.y, viewport.size.width, viewport.size.height);
		}


		// Draw the active IOSurface onto the window, using the local OpenGL context.
		{
			if (!NSEqualRects(viewport, frameInPixels))
			{
				// Clear the parts of the view that we aren't rendering over.
				glClearColor(0,0,0,1);
				glClear(GL_COLOR_BUFFER_BIT);
			}

			glBindTexture(GL_TEXTURE_RECTANGLE_EXT, receiveTexture);
			glDrawArrays(GL_TRIANGLES, 0, 3);

			glBindTexture(GL_TEXTURE_RECTANGLE_EXT, 0);
			glDeleteTextures(1, &receiveTexture);
			VuoGlPool_logVRAMFreed(receiveTextureBytes);

			glFlush();
		}
	});

	if (VuoIsDebugEnabled())
	{
		++_framesRenderedSinceProfileLogged;
		double t = VuoLogGetTime();
		const double profileSeconds = 10;
		if (t > _lastProfileLoggedTime + profileSeconds)
		{
			VDebugLog("%4.1f fps", _framesRenderedSinceProfileLogged / profileSeconds);
			_lastProfileLoggedTime = t;
			_framesRenderedSinceProfileLogged = 0;
		}
	}
}


/**
 * Indicates that the view should stop drawing.
 */
- (void)close
{
	_closed = YES;
	VuoRelease(_displayRefresh);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"
/**
 * Releases instance variables.
 */
- (void)dealloc
{
	// Dynamically changing the layer's contentsScale causes dealloc to be called
	// even though the layer has a nonzero retain count (!),
	// so we can't do anything here.

//	dispatch_release(_drawQueue);
//	if (_ioSurface)
//		VuoIoSurfacePool_disuse(_ioSurface);
//	[super dealloc];
}
#pragma clang diagnostic pop

@end
#pragma clang diagnostic pop

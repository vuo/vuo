/**
 * @file
 * VuoGraphicsView implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#import "VuoGraphicsView.h"
#import "VuoApp.h"
#import "VuoCompositionState.h"
#import "VuoCglPixelFormat.h"
#import "VuoEventLoop.h"

#import <OpenGL/gl3.h>
#import <OpenGL/gl3ext.h>
#import <IOSurface/IOSurface.h>

#ifdef VUO_COMPILER
VuoModuleMetadata({
	"title" : "VuoGraphicsView",
	"dependencies" : [
		"VuoCglPixelFormat",
		"VuoDisplayRefresh",
		"OpenGL.framework"
	]
});
#endif

/**
 * @todo this function is a temporary workaround until VuoShader can handle GLSL 1.50
 */
GLuint CompileShader(GLenum type, const char *source)
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
 * @todo this function is a temporary workaround until VuoGlContext can create OpenGL 3.2 contexts
 */
CGLContextObj createContext(void)
{
	CGLPixelFormatObj pixelFormat = (CGLPixelFormatObj)VuoGlContext_makePlatformPixelFormat(false, true, -1);
	CGLContextObj context;
	CGLError error = CGLCreateContext(pixelFormat, NULL, &context);
	CGLDestroyPixelFormat(pixelFormat);
	if (error != kCGLNoError)
	{
		VUserLog("Error: %s", CGLErrorString(error));
		return NULL;
	}

	return context;
}

/**
 * Private VuoGraphicsView data.
 */
@interface VuoGraphicsView ()
///@{
@property VuoGraphicsWindowInitCallback          initCallback;
@property VuoGraphicsWindowUpdateBackingCallback updateBackingCallback;
@property VuoGraphicsWindowResizeCallback        resizeCallback;
@property VuoGraphicsWindowDrawCallback          drawCallback;
@property void *userData;
///@}

@property bool ioSurfaceChanged;              ///< @ref VuoGraphicsView_drawOnIOSurface sets this to true when it's changed the size of the IOSurface.

@property dispatch_queue_t drawQueue;         ///< Serializes access to the root context.

@property bool redrawIOSurfaceDuringDrawRect; ///< When true, drawRect should force the IOSurface to redraw, then display its contents in the view before returning (ensures the view has an appropriately-sized texture while live-resizing).

@property VuoDisplayRefresh displayRefresh;	  ///< Periodically invokes @ref VuoGraphicsView_drawOnIOSurface.

@property(retain) NSOpenGLContext *localOpenGLContext; ///< The window's non-shared OpenGL context.
@property GLuint receiveTexture;                       ///< The texture connected to the IOSurface on the window's local context.
@property GLuint receiveTextureUniform;                ///< The shader's texture unit uniform.
@property GLuint receiveTextureOffsetUniform;          ///< The shader's texture offset uniform.

@property(retain) NSImage *circleImage; ///< The touch-circle mouse cursor.
@property NSRect circleRect;            ///< The bounding box of `circleImage`.

@property bool firstFrame;              ///< Ensures the resize callback is called before drawing.
@property bool closed;                  ///< True if the window is closed (and should stop rendering).  -isVisible isn't adequate (see https://b33p.net/kosada/node/12732).
@end

static const VuoReal locked = -INFINITY; ///< Special `time` value for @ref VuoGraphicsView_drawOnIOSurface.

static void VuoGraphicsView_drawOnIOSurface(VuoGraphicsView *view);

/**
 * Called by `displayRefresh`.
 */
static void VuoGraphicsView_frameRequestCallback(VuoReal time, void *userData)
{
	VuoGraphicsView *view = userData;
	if (!view.needsIOSurfaceRedraw)
		return;

	view.needsIOSurfaceRedraw = false;

	dispatch_sync_f(view.drawQueue, view, (dispatch_function_t)VuoGraphicsView_drawOnIOSurface);

	// `needsDisplay` is slow, and it disrupts moving/sizing the window via the accessibility API.
	// Instead, just call `drawRect:` ourself.
//	view.needsDisplay = true;
	// Doesn't need to be called on the main thread since we're locking the OpenGL context.
	[view drawRect:NSMakeRect(0,0,0,0)];
}

/**
 * Requests a new IOSurface from drawCallback.
 *
 * @threadQueue{drawQueue}
 */
static void VuoGraphicsView_drawOnIOSurface(VuoGraphicsView *view)
{
	NSRect frame = [view convertRectToBacking:view.frame];
	void *callerData = view.userData;
	__block VuoIoSurface vis = view.ioSurface;
	VuoGraphicsWindow *gw = (VuoGraphicsWindow *)view.window;

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
			NSRect contentRectWhenWindowed = [view convertRectToBacking:gw.contentRectWhenWindowed];
			width = contentRectWhenWindowed.size.width;
			height = contentRectWhenWindowed.size.height;
			x = frame.size.width/2 - width/2;
			y = frame.size.height/2 - height/2;
		}


		bool backingScaleFactorChanged = (gw.backingScaleFactorCached != gw.backingScaleFactor);
		NSRect newViewport = NSMakeRect(x, y, width, height);
		if (!NSEqualRects(view.viewport, newViewport) || view.firstFrame || backingScaleFactorChanged)
		{
			// When moving a window between Retina and non-Retina displays,
			// viewDidChangeBackingProperties might be invoked either before or after the next
			// VuoGraphicsView_drawOnIOSurface execution.  Ensure that, if the backing has changed,
			// we call backingChangedCallback, then resizeCallback, then drawCallback
			// (since backingChangedCallback creates a new VuoSceneRenderer, without a size).
			// https://b33p.net/kosada/node/12273
			if (backingScaleFactorChanged)
				[view viewDidChangeBackingProperties];

			view.resizeCallback(callerData, width, height);
			view.viewport = newViewport;
			view.firstFrame = NO;
		}
	}


	// Draw onto the IOSurface FBO.

#ifdef PROFILE
	static double lastFrameTime = 0;
	double start = VuoLogGetTime();
#endif

	vis = view.drawCallback(callerData);
	if (!vis)
		return;

#ifdef PROFILE
	double end = VuoLogGetTime();
	if (lastFrameTime > 0 && start-lastFrameTime > 1.5 * 1./60.)
	{
		double dropTime = (start-lastFrameTime)*60.;
		fprintf(stderr, "Dropped frame; draw took %f frame%s\n", dropTime, fabs(dropTime-1)<.00001 ? "" : "s");
	}
	lastFrameTime = end;
#endif

	[gw.recorder saveImage:vis];

	VuoIoSurface oldIOSurface = view.ioSurface;
	if (oldIOSurface)
	{
		VuoIoSurfacePool_signal(VuoIoSurfacePool_getIOSurfaceRef(oldIOSurface));
		VuoIoSurfacePool_disuse(oldIOSurface);
	}

	view.ioSurface = vis;
	view.ioSurfaceChanged = true;
}



@implementation VuoGraphicsView

/**
 * Creates an OpenGL view that calls the given callbacks for rendering.
 *
 * @threadMain
 */
- (instancetype)initWithInitCallback:(VuoGraphicsWindowInitCallback)initCallback
			   updateBackingCallback:(VuoGraphicsWindowUpdateBackingCallback)updateBackingCallback
				  backingScaleFactor:(float)backingScaleFactor
					  resizeCallback:(VuoGraphicsWindowResizeCallback)resizeCallback
						drawCallback:(VuoGraphicsWindowDrawCallback)drawCallback
							userData:(void *)userData
{
	if (self = [super init])
	{
		_initCallback = initCallback;
		_updateBackingCallback = updateBackingCallback;
		_resizeCallback = resizeCallback;
		_drawCallback = drawCallback;
		_userData = userData;

		/// @todo Remove drawQueue after switching to single-context, since VuoGl_perform() will take care of that.
		_drawQueue = dispatch_queue_create("org.vuo.VuoGraphicsView", VuoEventLoop_getDispatchInteractiveAttribute());

		_firstFrame = YES;

		self.wantsBestResolutionOpenGLSurface = YES;
		self.focusRingType = NSFocusRingTypeNone;

		VuoGlContext_perform(^(CGLContextObj cgl_ctx){
			_initCallback(_userData, backingScaleFactor);
		});


		CGLContextObj context = createContext();
		_localOpenGLContext = [[NSOpenGLContext alloc] initWithCGLContextObj:context];

		// Initialize stuff for rendering the IOSurface to the window.
		{
			CGLSetCurrentContext(context);

			GLuint vertexArray;
			glGenVertexArrays(1, &vertexArray);
			glBindVertexArray(vertexArray);

			const GLfloat quadPositions[] = {
				-1, -1,
				1, -1,
				-1, 1,
				1, 1
			};
			GLuint quadPositionBuffer;
			glGenBuffers(1, &quadPositionBuffer);
			glBindBuffer(GL_ARRAY_BUFFER, quadPositionBuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadPositions), quadPositions, GL_STATIC_DRAW);

			const char *vertexShaderSource = VUOSHADER_GLSL_SOURCE(150,
				in vec2 position;
				void main()
				{
					gl_Position = vec4(position, 0., 1.);
				}
			);
			const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(150,
				uniform sampler2DRect t;
				uniform vec2 textureOrigin;
				out vec4 FragColor;
				void main()
				{
					FragColor = texture(t, gl_FragCoord.xy - textureOrigin);
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


		// Prepare the circle mouse cursor.
		{
			_circleRect = NSMakeRect(0,0,48,48);
			_circleImage = [[NSImage alloc] initWithSize:_circleRect.size];
			[_circleImage lockFocus];
			{
				[[NSColor colorWithDeviceWhite:1 alpha:0.75] setFill];
				[[NSColor colorWithDeviceWhite:0 alpha:0.15] setStroke];
				NSBezierPath *circlePath = [NSBezierPath bezierPathWithOvalInRect:NSInsetRect(_circleRect, 1, 1)];
				[circlePath fill];
				[circlePath stroke];
			}
			[_circleImage unlockFocus];
		}


		_needsIOSurfaceRedraw = true;
		_displayRefresh = VuoDisplayRefresh_make(self);
		VuoRetain(_displayRefresh);
	}
	return self;
}

/**
 * Ensures mouse coordinates can be accurately reported even before drawing.
 * https://b33p.net/kosada/node/12691
 */
- (void)viewDidMoveToWindow
{
	if (!self.window)
		return;

	_viewport = self.frame;
}

/**
 * @threadAny
 */
- (void)enableTriggers
{
	VuoGraphicsWindow *gw = (VuoGraphicsWindow *)self.window;
	VuoDisplayRefresh_enableTriggers(_displayRefresh, gw.requestedFrame, VuoGraphicsView_frameRequestCallback);
}

/**
 * @threadAny
 */
- (void)disableTriggers
{
	VuoDisplayRefresh_disableTriggers(_displayRefresh);
}


- (void)frameDidChange:(NSNotification*)notification
{
	CGLContextObj context = _localOpenGLContext.CGLContextObj;
	GLint oldRendererID;
	CGLGetParameter(context, kCGLCPCurrentRendererID, &oldRendererID);
	GLint oldVirtualScreen = _localOpenGLContext.currentVirtualScreen;

	[_localOpenGLContext update];

	GLint rendererID;
	CGLGetParameter(context, kCGLCPCurrentRendererID, &rendererID);
	if (rendererID != oldRendererID)
		VDebugLog("OpenGL context %p's renderer changed to %s", context, VuoCglRenderer_getText(rendererID));
	GLint virtualScreen = _localOpenGLContext.currentVirtualScreen;
	if (virtualScreen != oldVirtualScreen)
		VDebugLog("OpenGL context %p's virtual screen changed from %d to %d", context, oldVirtualScreen, virtualScreen);

//	_needsIOSurfaceRedraw = true;
	_redrawIOSurfaceDuringDrawRect = true;
}

/**
 * Called by [super init]
 */
- (instancetype)initWithFrame:(NSRect)frameRect
{
	if (self = [super initWithFrame:frameRect])
	{
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(frameDidChange:) name:NSViewGlobalFrameDidChangeNotification object:self];
	}
	return self;
}

/**
 * Cocoa calls this method when it needs us to repaint the view.
 */
- (void)drawRect:(NSRect)dirtyRect
{
	if (_closed)
		return;

	dispatch_sync(_drawQueue, ^{

	// If the window was resized, redraw the scene right now,
	// to avoid stretching the old-size texture.
	if (_redrawIOSurfaceDuringDrawRect)
	{
		VuoGraphicsView_drawOnIOSurface(self);
		_redrawIOSurfaceDuringDrawRect = NO;
	}


	if (_localOpenGLContext.view != self)
	{
		_localOpenGLContext.view = self;
		VDebugLog("OpenGL context %p's virtual screen changed to %d", _localOpenGLContext.CGLContextObj, _localOpenGLContext.currentVirtualScreen);
	}

	if (!_ioSurface)
		return;


	CGLContextObj context = [_localOpenGLContext CGLContextObj];
	CGLLockContext(context);
	CGLSetCurrentContext(context);

	NSRect frameInPoints = self.frame;
	NSRect frameInPixels = [self convertRectToBacking:frameInPoints];
	size_t ioSurfaceWidth  = VuoIoSurfacePool_getWidth(_ioSurface);
	size_t ioSurfaceHeight = VuoIoSurfacePool_getHeight(_ioSurface);


	// Prepare a texture to receive the IOSurface onto the local OpenGL context.
	if (_ioSurfaceChanged)
	{
		_ioSurfaceChanged = false;
		if (_receiveTexture)
			glDeleteTextures(1, &_receiveTexture);

		glGenTextures(1, &_receiveTexture);
		glBindTexture(GL_TEXTURE_RECTANGLE, _receiveTexture);
		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		CGLError err = CGLTexImageIOSurface2D(context, GL_TEXTURE_RECTANGLE, GL_RGB, ioSurfaceWidth, ioSurfaceHeight, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, VuoIoSurfacePool_getIOSurfaceRef(_ioSurface), 0);
		if(err != kCGLNoError)
		{
			VUserLog("Error in CGLTexImageIOSurface2D() 2: %s", CGLErrorString(err));
			return;
		}
		glBindTexture(GL_TEXTURE_RECTANGLE, 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_RECTANGLE, _receiveTexture);
		glUniform1i(_receiveTextureUniform, 0);

		glUniform2f(_receiveTextureOffsetUniform, _viewport.origin.x, _viewport.origin.y);
		glViewport(_viewport.origin.x, _viewport.origin.y, _viewport.size.width, _viewport.size.height);
	}


	// Draw the active IOSurface onto the window, using the local OpenGL context.
	{
		if (!NSEqualRects(_viewport, frameInPixels))
		{
			// Clear the parts of the view that we aren't rendering over.
			glClearColor(0,0,0,0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glFlush();
//		[_localOpenGLContext flushBuffer];
	}

	CGLSetCurrentContext(NULL);
	CGLUnlockContext(context);

	});
}

/**
 * Called when the view changes screens (and thus maybe the backingScaleFactor changed).
 *
 * @threadMain
 */
- (void)viewDidChangeBackingProperties
{
	float backingScaleFactor = self.window.backingScaleFactor;
	VuoGraphicsWindow *gw = (VuoGraphicsWindow *)_window;
	if (backingScaleFactor != gw.backingScaleFactorCached)
	{
		// Get the window's current size in pixels.
		NSRect contentRect = [gw contentRectForFrameRect:gw.frame];
		NSSize oldContentRectPixelSize = NSMakeSize(contentRect.size.width  * gw.backingScaleFactorCached,
													contentRect.size.height * gw.backingScaleFactorCached);

		gw.backingScaleFactorCached = backingScaleFactor;

		VuoGlContext_perform(^(CGLContextObj cgl_ctx){
			_updateBackingCallback(_userData, backingScaleFactor);
		});

		if (gw.maintainsPixelSizeWhenBackingChanges)
		{
			// Resize the window to maintain its pixel size.

			NSSize newContentRectPointSize = NSMakeSize(oldContentRectPixelSize.width  / gw.backingScaleFactorCached,
														oldContentRectPixelSize.height / gw.backingScaleFactorCached);


			NSRect contentRect = [gw contentRectForFrameRect:gw.frame];

			// Adjust the y position by the change in height, so that the window appears to be anchored in its top-left corner
			// (instead of its bottom-left corner as the system does by default).
			contentRect.origin.y += contentRect.size.height - newContentRectPointSize.height;

			contentRect.size = newContentRectPointSize;
			[gw setFrame:[gw frameRectForContentRect:contentRect] display:YES animate:NO];
		}
	}
}

- (void)resetCursorRects
{
	VuoCursor cursor = ((VuoGraphicsWindow *)_window).cursor;
	NSCursor *nsCursor = nil;

	if (cursor == VuoCursor_None)
	{
		NSImage *im = [[NSImage alloc] initWithSize:NSMakeSize(1,1)];
		nsCursor = [[[NSCursor alloc] initWithImage:im hotSpot:NSMakePoint(0,0)] autorelease];
		[im release];
	}
	else if (cursor == VuoCursor_Pointer)
		nsCursor = [NSCursor arrowCursor];
	else if (cursor == VuoCursor_Crosshair)
		nsCursor = [NSCursor crosshairCursor];
	else if (cursor == VuoCursor_HandOpen)
		nsCursor = [NSCursor openHandCursor];
	else if (cursor == VuoCursor_HandClosed)
		nsCursor = [NSCursor closedHandCursor];
	else if (cursor == VuoCursor_IBeam)
		nsCursor = [NSCursor IBeamCursor];
	else if (cursor == VuoCursor_Circle)
		nsCursor = [[[NSCursor alloc] initWithImage:_circleImage hotSpot:NSMakePoint(NSMidX(_circleRect),NSMidY(_circleRect))] autorelease];

	if (nsCursor)
		[self addCursorRect:[self visibleRect] cursor:nsCursor];
}

/**
 * Without this, double-clicks cause the window to minimize/maximize itself (depending on System Preferences).
 *
 * @threadMain
 */
- (BOOL)acceptsFirstResponder
{
	return YES;
}

/**
 * Indicates that the view should stop drawing.
 */
- (void)close
{
	_closed = YES;
}

/**
 * Releases instance variables.
 */
- (void)dealloc
{
	VuoRelease(_displayRefresh);
	dispatch_release(_drawQueue);
	if (_ioSurface)
		VuoIoSurfacePool_disuse(_ioSurface);
	[[NSNotificationCenter defaultCenter] removeObserver:self name:NSViewGlobalFrameDidChangeNotification object:self];
	[super dealloc];
}

@end

/**
 * @file
 * vuo.image.make.web node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoMacOSSDKWorkaround.h"
#import <OpenGL/CGLMacro.h>
#import <WebKit/WebKit.h>

#import "node.h"

#import "VuoApp.h"

VuoModuleMetadata({
	"title" : "Make Image from Web Page",
	"keywords" : [
		"html",
		"webkit",
	],
	"version" : "1.0.1",
	"dependencies" : [
		"WebKit.framework",
		"VuoUrl",
	],
	"node" : {
		"exampleCompositions" : [ "SpinSphereWithControlPanel.vuo", "ShowWebPage.vuo" ]
	},
	"compatibility": {
		"macos": {
			"min": "10.13",
		},
	},
});

@interface WKWebViewConfiguration (macOS1011)
@property(nonatomic, copy) NSString *applicationNameForUserAgent;
@end

@interface VuoWKNavigationDelegate : NSObject <WKNavigationDelegate>
@property struct nodeInstanceData *nodeContext;
@end

struct nodeInstanceData
{
	WKWebView *webView;
	VuoWKNavigationDelegate *delegate;
	int priorWidth, priorHeight;
	CGPoint priorMousePosition;

	dispatch_queue_t callbackQueue;  ///< Synchronizes access to the following callbacks.
	void (*updatedImage)(VuoImage);
	void (*updatedProgress)(VuoReal);
	void (*startedLoading)(VuoText);
	void (*finishedLoading)(VuoText);
};

@implementation VuoWKNavigationDelegate
@synthesize nodeContext;
- (instancetype)initWithNodeContext:(struct nodeInstanceData *)c
{
	if (self = [super init])
		self.nodeContext = c;
	return self;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
	dispatch_sync(nodeContext->callbackQueue, ^{
	if ([keyPath isEqualToString:@"estimatedProgress"] && self.nodeContext->updatedProgress)
		self.nodeContext->updatedProgress(self.nodeContext->webView.estimatedProgress);

	else if ([keyPath isEqualToString:@"URL"])
	{
		// If the URL has changed, intra-page navigation has probably occurred.
		// WebKit might not fire -webView:didFinishNavigation: in that case.
		dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
			[self fireImage];
		});

		if (!self.nodeContext->webView.loading
			&& self.nodeContext->webView.URL
			&& self.nodeContext->finishedLoading)
			self.nodeContext->finishedLoading(self.urlText);
	}
	});
}

- (VuoText)urlText
{
	NSString *url = self.nodeContext->webView.URL.absoluteString;

	if ([url hasPrefix:@"about:blank"])
	{
		// NSURL fails to parse `about:blank#fragment`, so parse it ourselves.

		url = [url substringFromIndex:[@"about:blank" length]];

		// If there's an improperly-encoded fragment marker, decode it.
		if ([url hasPrefix:@"%23"])
			url = [@"#" stringByAppendingString:[url substringFromIndex:[@"%23" length]]];
	}

	if (url)
		return VuoText_makeFromCFString(url);
	else
		return VuoText_make("");
}

- (void)webView:(WKWebView *)webView didStartProvisionalNavigation:(WKNavigation *)navigation
{
	dispatch_sync(nodeContext->callbackQueue, ^{
	if (self.nodeContext->startedLoading)
		self.nodeContext->startedLoading(self.urlText);
	});
}

//- (void)webView:(WKWebView *)webView didCommitNavigation:(WKNavigation *)navigation
//{
//}

- (void)webView:(WKWebView *)webView didFailProvisionalNavigation:(WKNavigation *)navigation withError:(NSError *)error
{
	// Ignore cancellations, since those are normal
	// when the user navigates to another page before the current page has finished loading.
	if (error.domain == NSURLErrorDomain && error.code == NSURLErrorCancelled)
		return;

	VUserLog("Error: %s — %s",
			 error.localizedDescription.UTF8String,
			 [[error.userInfo[NSUnderlyingErrorKey] localizedDescription] UTF8String]);
}

- (void)webView:(WKWebView *)webView didFailNavigation:(WKNavigation *)navigation withError:(NSError *)error
{
	VUserLog("Error: %s — %s",
			 error.localizedDescription.UTF8String,
			 [[error.userInfo[NSUnderlyingErrorKey] localizedDescription] UTF8String]);
}

- (void)loadURL:(VuoText)url
{
	if (VuoText_isEmpty(url))
		return;

	VuoText normalizedURL = VuoUrl_normalize(url, VuoUrlNormalize_assumeHttp);
	VuoLocal(normalizedURL);

	NSString *nss = [NSString stringWithUTF8String:normalizedURL];
	if (!nss)
	{
		VUserLog("Error: '%s' isn't valid UTF-8.", normalizedURL);
		return;
	}

	NSURL *nsu = [NSURL URLWithString:nss];
	if (!nsu)
	{
		VUserLog("Error: '%s' isn't a valid URL.", normalizedURL);
		return;
	}

	[self.nodeContext->webView loadRequest:[NSURLRequest requestWithURL:nsu]];
}

- (void)loadHTML:(VuoText)html
{
	if (VuoText_isEmpty(html))
		return;

	NSString *nss = [NSString stringWithUTF8String:html];
	if (!nss)
	{
		VUserLog("Error: '%s' isn't valid UTF-8.", html);
		return;
	}

	[self.nodeContext->webView loadHTMLString:nss baseURL:nil];
}

/**
 * @threadAny
 */
- (void)fireImage
{
	if (!self.nodeContext->updatedImage)
		return;

	__block CGWindowID windowNumber;
	VuoApp_executeOnMainThread(^{
		windowNumber = (CGWindowID)self.nodeContext->webView.window.windowNumber;
	});

	CGImageRef cgi = CGWindowListCreateImage(CGRectNull,
											 kCGWindowListOptionIncludingWindow,
											 windowNumber,
											 kCGWindowImageBoundsIgnoreFraming | kCGWindowImageNominalResolution);
	if (!cgi)
		return;

	size_t bpp = CGImageGetBitsPerPixel(cgi);
	if (bpp != 32)
	{
		VUserLog("Error: WindowServer gave us a CGImage with an unsupported BPP: %zu", bpp);
		return;
	}

	CGBitmapInfo bi = CGImageGetBitmapInfo(cgi);
	if (bi != (kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little)
	 && bi != (kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrderDefault))
	{
		VUserLog("Error: WindowServer gave us a CGImage with an unsupported alpha/byteorder: 0x%x", bi);
		return;
	}

	size_t width       = CGImageGetWidth(cgi);
	size_t height      = CGImageGetHeight(cgi);
	if (width <= 1 && height <= 1)
	{
		// Content hasn't loaded yet.
		CGImageRelease(cgi);
		return;
	}

	size_t bytesPerRow = CGImageGetBytesPerRow(cgi);
	CFDataRef  dataFromImageDataProvider = CGDataProviderCopyData(CGImageGetDataProvider(cgi));
	CGImageRelease(cgi);
	if (!dataFromImageDataProvider)
		return;

	GLubyte *bitmapData = (GLubyte *)CFDataGetBytePtr(dataFromImageDataProvider);
	if (!bitmapData)
		return;

	// Flip the image data (CGImage is unflipped, but VuoImage_makeFromBuffer() expects flipped).
	char *flippedBitmapData = (char *)malloc(bytesPerRow * height);
	for (unsigned long y = 0; y < height; ++y)
		memcpy(flippedBitmapData + width * 4 * (height - y - 1), bitmapData + bytesPerRow * y, width * 4);

	CFRelease(dataFromImageDataProvider);

	VuoImage vi = VuoImage_makeFromBuffer(flippedBitmapData, GL_BGRA, width, height, VuoImageColorDepth_8, ^(void *buffer){ free(flippedBitmapData); });
	dispatch_sync(nodeContext->callbackQueue, ^{
		if (self.nodeContext->updatedImage)
			self.nodeContext->updatedImage(vi);
		else
		{
			VuoRetain(vi);
			VuoRelease(vi);
		}
	});
}

- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation
{
	dispatch_sync(nodeContext->callbackQueue, ^{
	if (self.nodeContext->finishedLoading)
		self.nodeContext->finishedLoading(self.urlText);

	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
		[self fireImage];
	});

	// Sometimes -webView:didFinishNavigation: fires before rendering has completed.
	dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC * 0.2), dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
		[self fireImage];
	});
	});
}
@end

struct nodeInstanceData *nodeInstanceInit()
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(sizeof(struct nodeInstanceData), 1);
	VuoRegister(context, free);
	context->callbackQueue = dispatch_queue_create("org.vuo.image.make.web", NULL);
	return context;
}

@interface VuoImageMakeWebWindow : NSWindow
@end
@implementation VuoImageMakeWebWindow
- (CGFloat)backingScaleFactor
{
	return 1;
}
@end

void nodeInstanceTriggerStart(
	VuoInstanceData(struct nodeInstanceData *) context,
	VuoInputData(VuoText) loadURL,
	VuoInputData(VuoText) loadHTML,
	VuoInputData(VuoInteger) width,
	VuoInputData(VuoInteger) height,
	VuoOutputTrigger(startedLoading, VuoText),
	VuoOutputTrigger(updatedProgress, VuoReal),
	VuoOutputTrigger(finishedLoading, VuoText),
	VuoOutputTrigger(updatedImage, VuoImage))
{
	dispatch_sync((*context)->callbackQueue, ^{
		(*context)->updatedImage    = updatedImage;
		(*context)->updatedProgress = updatedProgress;
		(*context)->finishedLoading = finishedLoading;
		(*context)->startedLoading  = startedLoading;
	});

	if (!(*context)->webView)
		VuoApp_executeOnMainThread(^{
			VuoApp_init(true);

			WKWebViewConfiguration *config = [WKWebViewConfiguration new];
			config.applicationNameForUserAgent = [NSString stringWithUTF8String:VuoApp_getName()];
			(*context)->webView = [[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, width, height) configuration:config];
			(*context)->delegate = [[VuoWKNavigationDelegate alloc] initWithNodeContext:*context];
			(*context)->webView.navigationDelegate = (*context)->delegate;
			[(*context)->webView addObserver:(*context)->delegate forKeyPath:@"estimatedProgress" options:NSKeyValueObservingOptionNew context:nil];
			[(*context)->webView addObserver:(*context)->delegate forKeyPath:@"URL" options:NSKeyValueObservingOptionNew context:nil];
			(*context)->webView.layer.contentsScale = 1;

			// In order to get an image of the contents of WKWebView,
			// attach it to a window and capture the window.
			// (Don't use `-[WKWebView takeSnapshotWithConfiguration:completionHandler:]`
			// since it only exists on macOS 10.13+
			// and since it doesn't capture hardware-accelerated content (video, CSS3, WebGL, etc)
			// and since it doesn't provide a way to specify the backingScaleFactor.)
			// https://b33p.net/kosada/node/11013#comment-75585
			VuoImageMakeWebWindow *shamWindow = [[VuoImageMakeWebWindow alloc] initWithContentRect:NSMakeRect(-16000, -16000, width, height)
																						 styleMask:NSWindowStyleMaskBorderless
																						   backing:NSBackingStoreBuffered
																							 defer:NO];
			shamWindow.collectionBehavior = NSWindowCollectionBehaviorStationary | NSWindowCollectionBehaviorIgnoresCycle;
			shamWindow.contentView = (*context)->webView;
			shamWindow.acceptsMouseMovedEvents = YES;
			[shamWindow makeKeyAndOrderFront:nil];
			(*context)->priorWidth = width;
			(*context)->priorHeight = height;

			if (!VuoText_isEmpty(loadURL))
				[(*context)->delegate loadURL:loadURL];
			else
				[(*context)->delegate loadHTML:loadHTML];
		});
}

/**
 * Converts `vp`, in Vuo Coordinates — (-1,-aspect) in bottom left to (1,aspect) in top right —
 * to NSView coordinates — (0,0) in bottom left to (width-1,height-1) in top right.
 */
static NSPoint convertVuoToViewCoordinates(struct nodeInstanceData *context, VuoPoint2d vp)
{
	NSSize s = context->webView.frame.size;
	return NSMakePoint((vp.x + 1)/2 * (s.width-1),
					   (vp.y + s.height/s.width)/2 * (s.width-1));
}

void nodeInstanceEvent(
	VuoInstanceData(struct nodeInstanceData *) context,
	VuoInputData(VuoText, { "name" : "Load URL", "default" : "https://example.com" }) loadURL,
	VuoInputEvent({ "eventBlocking" : "none", "data" : "loadURL" }) loadURLEvent,
	VuoInputData(VuoText, { "name" : "Load HTML", "default" : "" }) loadHTML,
	VuoInputEvent({ "eventBlocking" : "none", "data" : "loadHTML" }) loadHTMLEvent,
	VuoInputEvent({ "name" : "Re-render" }) rerender,
//	VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) moveMouse,
//	VuoInputEvent({ "eventBlocking" : "none", "data" : "moveMouse" }) moveMouseEvent,
	VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) scrollMouse,
	VuoInputEvent({ "eventBlocking" : "none", "data" : "scrollMouse" }) scrollMouseEvent,
	VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) clickMouse,
	VuoInputEvent({ "eventBlocking" : "none", "data" : "clickMouse" }) clickMouseEvent,
	VuoInputEvent() goBack,
	VuoInputEvent() goForward,
	VuoInputData(VuoInteger, { "default" : 1024, "suggestedMin" : 1, "suggestedStep" : 32 }) width,
	VuoInputData(VuoInteger, { "default" : 768, "suggestedMin" : 1, "suggestedStep" : 32 }) height,
	VuoOutputTrigger(startedLoading, VuoText),
	VuoOutputTrigger(updatedProgress, VuoReal, { "eventThrottling" : "drop" }),
	VuoOutputTrigger(finishedLoading, VuoText),
	VuoOutputTrigger(updatedImage, VuoImage, { "eventThrottling" : "drop" }))
{
	if (width  != (*context)->priorWidth
	 || height != (*context)->priorHeight)
		VuoApp_executeOnMainThread(^{
			(*context)->webView.window.contentSize = NSMakeSize(width, height);
			(*context)->priorWidth  = width;
			(*context)->priorHeight = height;
		});

	if (loadURLEvent)
		VuoApp_executeOnMainThread(^{
			[(*context)->delegate loadURL:loadURL];
		});

	if (loadHTMLEvent)
		VuoApp_executeOnMainThread(^{
			[(*context)->delegate loadHTML:loadHTML];
		});

#if 0
	if (moveMouseEvent)
	{
		VuoApp_executeOnMainThread(^{
			[(*context)->webView mouseMoved:[NSEvent mouseEventWithType:NSMouseMoved
															   location:convertVuoToViewCoordinates(*context, moveMouse)
														  modifierFlags:0
															  timestamp:[NSDate timeIntervalSinceReferenceDate]
														   windowNumber:0
																context:nil
															eventNumber:0
															 clickCount:0
															   pressure:0]];
		});
		[(*context)->delegate fireImage];
	}
#endif

	if (scrollMouseEvent)
	{
		VuoApp_executeOnMainThread(^{
			CGEventRef cgevent = CGEventCreateScrollWheelEvent(NULL, kCGScrollEventUnitPixel, 2, (int32_t)(scrollMouse.y * 20), (int32_t)(scrollMouse.x * -20));
			NSEvent *e = [NSEvent eventWithCGEvent:cgevent];
			CFRelease(cgevent);
			[(*context)->webView scrollWheel:e];
		});
		[(*context)->delegate fireImage];
	}

	if (clickMouseEvent)
	{
		VuoApp_executeOnMainThread(^{
			NSPoint p = convertVuoToViewCoordinates(*context, clickMouse);
			[(*context)->webView mouseDown:[NSEvent mouseEventWithType:NSEventTypeLeftMouseDown
															  location:p
														 modifierFlags:0
															 timestamp:[NSDate timeIntervalSinceReferenceDate]
														  windowNumber:0
															   context:nil
														   eventNumber:0
															clickCount:1
															  pressure:1]];

			[(*context)->webView mouseUp:[NSEvent mouseEventWithType:NSEventTypeLeftMouseUp
															location:p
													   modifierFlags:0
														   timestamp:[NSDate timeIntervalSinceReferenceDate]
														windowNumber:0
															 context:nil
														 eventNumber:0
														  clickCount:1
															pressure:1]];

		});
		[(*context)->delegate fireImage];
	}

	if (goBack)
		VuoApp_executeOnMainThread(^{
			[(*context)->webView goBack];
		});

	if (goForward)
		VuoApp_executeOnMainThread(^{
			[(*context)->webView goForward];
		});

	if (rerender)
		[(*context)->delegate fireImage];
}

void nodeInstanceTriggerStop(
	VuoInstanceData(struct nodeInstanceData *) context)
{
	dispatch_sync((*context)->callbackQueue, ^{
		(*context)->updatedImage    = NULL;
		(*context)->updatedProgress = NULL;
		(*context)->finishedLoading = NULL;
		(*context)->startedLoading  = NULL;
	});
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) context)
{
	VuoApp_executeOnMainThread(^{
		(*context)->webView.navigationDelegate = nil;

		[(*context)->webView removeObserver:(*context)->delegate forKeyPath:@"estimatedProgress"];
		[(*context)->webView removeObserver:(*context)->delegate forKeyPath:@"URL"];

		[(*context)->webView release];
	});
	dispatch_sync((*context)->callbackQueue, ^{});
	dispatch_release((*context)->callbackQueue);
}

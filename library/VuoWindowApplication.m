/**
 * @file
 * VuoWindowApplication implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#import "VuoWindowApplication.h"

#include "module.h"

#include <mach-o/dyld.h> // for _NSGetExecutablePath()
#include <libgen.h> // for dirname()
#include <dirent.h>

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoWindowApplication",
					 "dependencies" : [
						"AppKit.framework"
					 ]
				 });
#endif


/**
 * Delegate for VuoWindowApplication.
 */
@interface VuoWindowApplicationDelegate : NSObject<NSApplicationDelegate>
@end

@implementation VuoWindowApplicationDelegate

/**
 * When the user tries to quit the application, cleanly stops the composition.
 */
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
	VuoStopComposition();
	return NSTerminateCancel;
}

@end


@implementation VuoWindowApplication

/**
 * Creates the application and its menu.
 */
- (id)init
{
	if ((self = [super init]))
	{
		// http://stackoverflow.com/a/11010614/238387

		NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

		delegate = [VuoWindowApplicationDelegate new];
		[self setDelegate:delegate];

		[self setActivationPolicy:NSApplicationActivationPolicyRegular];
		NSMenu *menubar = [[NSMenu new] autorelease];
		NSMenuItem *appMenuItem = [[NSMenuItem new] autorelease];
		[menubar addItem:appMenuItem];
		[self setMainMenu:menubar];

		NSMenu *appMenu = [[NSMenu new] autorelease];
		NSString *appName = [[NSProcessInfo processInfo] processName];

		NSString *aboutTitle = [@"About " stringByAppendingString:appName];
		NSMenuItem *aboutMenuItem = [[[NSMenuItem alloc] initWithTitle:aboutTitle action:@selector(displayAboutPanel:) keyEquivalent:@""] autorelease];
		[appMenu addItem:aboutMenuItem];

		[appMenu addItem:[NSMenuItem separatorItem]];

		NSString *quitTitle = [@"Quit " stringByAppendingString:appName];
		NSMenuItem *quitMenuItem = [[[NSMenuItem alloc] initWithTitle:quitTitle action:@selector(terminate:) keyEquivalent:@"q"] autorelease];
		[appMenu addItem:quitMenuItem];

		[appMenuItem setSubmenu:appMenu];

		windowMenuItems = nil;

		[pool release];
	}
	return self;
}

/**
 * Releases this application instance.
 */
- (void)dealloc
{
	[delegate release];
	[super dealloc];
}

/**
 * Loads license data and sends it to the standard "About" panel.
 */
- (void)displayAboutPanel:(id)sender
{
	[self orderFrontStandardAboutPanelWithOptions:[NSDictionary dictionaryWithObject:[self licenseString] forKey:@"Credits"]];
}

/**
 * Builds an attributed string from the contents of each file in the app bundle's "Licenses" folder.
 */
- (NSAttributedString *)licenseString
{
	NSMutableAttributedString *mas = [NSMutableAttributedString new];

	[mas appendAttributedString:[[[NSAttributedString new] initWithHTML:[@"<p>This composition may include software licensed under the following terms:</p>" dataUsingEncoding:NSUTF8StringEncoding] documentAttributes:nil] autorelease]];


	// Find Vuo.framework.
	char frameworkPath[PATH_MAX+1] = "";
	for(unsigned int i=0; i<_dyld_image_count(); ++i)
	{
		const char *dylibPath = _dyld_get_image_name(i);
		char *pos;
		if ( (pos = strstr(dylibPath, "/Vuo.framework/")) )
		{
			strncpy(frameworkPath, dylibPath, pos-dylibPath);
			break;
		}
	}

	// Derive the path of "Licenses" directory.
	char licensesPath[PATH_MAX+1];
	strncpy(licensesPath, frameworkPath, PATH_MAX);
	strncat(licensesPath, "/Vuo.framework/Versions/" VUO_VERSION_STRING "/Documentation/Licenses", PATH_MAX);


	bool foundLicenses = false;
	if (access(licensesPath, 0) == 0)
	{
		DIR *dirp = opendir(licensesPath);
		struct dirent *dp;
		while ((dp = readdir(dirp)) != NULL)
		{
			if (dp->d_name[0] == '.')
				continue;

			char *nameWithoutExtension = strdup(dp->d_name);
			nameWithoutExtension[strlen(nameWithoutExtension) - 4] = 0;
			[mas appendAttributedString:[[[NSAttributedString new] initWithHTML:[[NSString stringWithFormat:@"<h2>%s</h2>", nameWithoutExtension] dataUsingEncoding:NSUTF8StringEncoding] documentAttributes:nil] autorelease]];
			free(nameWithoutExtension);

			char licensePath[strlen(licensesPath) + dp->d_namlen + 2];
			strcpy(licensePath, licensesPath);
			strcat(licensePath, "/");
			strcat(licensePath, dp->d_name);

			NSMutableData *mdata = [NSMutableData new];
			[mdata appendData:[[NSString stringWithFormat:@"<pre>"] dataUsingEncoding:NSUTF8StringEncoding]];
			[mdata appendData:[NSData dataWithContentsOfFile:[NSString stringWithUTF8String:licensePath]]];
			[mdata appendData:[[NSString stringWithFormat:@"</pre><br>"] dataUsingEncoding:NSUTF8StringEncoding]];
			[mas appendAttributedString:[[[NSAttributedString new]
										   initWithHTML:mdata
										   options:[NSDictionary dictionaryWithObject:[NSNumber numberWithInteger:NSUTF8StringEncoding] forKey:NSCharacterEncodingDocumentOption]
										   documentAttributes:nil] autorelease]];
			[mdata release];

			foundLicenses = true;
		}
		closedir(dirp);
	}

	if (!foundLicenses)
		[mas appendAttributedString:[[[NSAttributedString new] initWithHTML:[@"<p>(No license information found.)</p>" dataUsingEncoding:NSUTF8StringEncoding] documentAttributes:nil] autorelease]];

	return [mas autorelease];
}

/**
 * Starts the event loop.
 */
- (void)run
{
	// http://www.cocoawithlove.com/2009/01/demystifying-nsapplication-by.html
	// http://stackoverflow.com/questions/6732400/cocoa-integrate-nsapplication-into-an-existing-c-mainloop

	[self finishLaunching];

	timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER,0,0,dispatch_get_main_queue());
	dispatch_source_set_timer(timer, dispatch_walltime(NULL,0), NSEC_PER_SEC/100, NSEC_PER_SEC/100);
	dispatch_source_set_event_handler(timer, ^{
		NSAutoreleasePool *pool = [NSAutoreleasePool new];
		NSEvent *event;
		do {
			event = [self nextEventMatchingMask:NSAnyEventMask
									  untilDate:[NSDate distantPast]
										 inMode:NSDefaultRunLoopMode
										dequeue:YES];
			[self sendEvent:event];
		} while (event != nil);
		[self updateWindows];
		[pool drain];
	});

	dispatch_resume(timer);
}

/**
 * Stops the event loop.
 */
- (void)stop:(id)sender
{
	dispatch_source_cancel(timer);
	dispatch_release(timer);
}

/**
 * Replaces the top-level menus in the menu bar, except for application-wide menus,
 * with the given menus.
 */
- (void)replaceWindowMenu:(NSArray *)newMenuItems
{
	NSMenu *menubar = [self mainMenu];

	for (NSMenuItem *oldMenu in windowMenuItems)
		[menubar removeItem:oldMenu];

	for (NSMenuItem *newMenu in newMenuItems)
		[menubar addItem:newMenu];

	[windowMenuItems release];
	windowMenuItems = [newMenuItems retain];
}

@end

/**
 * @file
 * VuoWindowApplication implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
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

		VuoWindowApplicationDelegate *delegate = [VuoWindowApplicationDelegate new];
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

	[mas appendAttributedString:[[NSAttributedString new] initWithHTML:[@"<p>This composition may include software licensed under the following terms:</p>" dataUsingEncoding:NSUTF8StringEncoding] documentAttributes:nil]];

	// Get the exported executable path.
	char rawExecutablePath[PATH_MAX+1];
	uint32_t size = sizeof(rawExecutablePath);
	_NSGetExecutablePath(rawExecutablePath, &size);

	char cleanedExecutablePath[PATH_MAX+1];
	realpath(rawExecutablePath, cleanedExecutablePath);

	// Derive the path of the app bundle's "Licenses" directory from its executable path.
	char executableDir[PATH_MAX+1];
	strcpy(executableDir, dirname(cleanedExecutablePath));

	const char *licensesPathFromExecutable = "/../Frameworks/Vuo.framework/Versions/" VUO_VERSION_STRING "/Licenses";
	char rawLicensesPath[strlen(executableDir)+strlen(licensesPathFromExecutable)+1];
	strcpy(rawLicensesPath, executableDir);
	strcat(rawLicensesPath, licensesPathFromExecutable);

	char cleanedLicensesPath[PATH_MAX+1];
	realpath(rawLicensesPath, cleanedLicensesPath);

	bool foundLicenses = false;
	if (access(cleanedLicensesPath, 0) == 0)
	{
		DIR *dirp = opendir(cleanedLicensesPath);
		struct dirent *dp;
		while ((dp = readdir(dirp)) != NULL)
		{
			if (dp->d_name[0] == '.')
				continue;

			[mas appendAttributedString:[[NSAttributedString new] initWithHTML:[[NSString stringWithFormat:@"<h2>%s</h2>", dp->d_name] dataUsingEncoding:NSUTF8StringEncoding] documentAttributes:nil]];

			char licensePath[strlen(cleanedLicensesPath) + dp->d_namlen + 2];
			strcpy(licensePath, cleanedLicensesPath);
			strcat(licensePath, "/");
			strcat(licensePath, dp->d_name);

			int fd = open(licensePath, O_RDONLY);
			char data[1024];
			int bytesRead;
			NSMutableData *mdata = [NSMutableData new];
			[mdata appendData:[[NSString stringWithFormat:@"<pre>"] dataUsingEncoding:NSUTF8StringEncoding]];
			while((bytesRead = read(fd, data, 1024)) > 0)
				[mdata appendBytes:data length:bytesRead];
			close(fd);
			[mdata appendData:[[NSString stringWithFormat:@"</pre>"] dataUsingEncoding:NSUTF8StringEncoding]];
			[mas appendAttributedString:[[NSAttributedString new] initWithHTML:mdata documentAttributes:nil]];

			foundLicenses = true;
		}
		closedir(dirp);
	}

	if (!foundLicenses)
		[mas appendAttributedString:[[NSAttributedString new] initWithHTML:[@"<p>(No license information found.)</p>" dataUsingEncoding:NSUTF8StringEncoding] documentAttributes:nil]];

	return mas;
}

/**
 * Starts the event loop.
 */
- (void)run
{
	// http://www.cocoawithlove.com/2009/01/demystifying-nsapplication-by.html
	// http://stackoverflow.com/questions/6732400/cocoa-integrate-nsapplication-into-an-existing-c-mainloop

	[self finishLaunching];

	/// @todo event tracking run loop mode (https://b33p.net/kosada/node/5961)
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

/**
 * @file
 * VuoAppAboutBox implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#import "VuoAppAboutBox.h"

#import <libgen.h>
#import <dirent.h>

#import "VuoAppSplashView.h"
#import "VuoConfig.h"
#import "module.h"

/**
 * Converts an NSColor to a CSS color.
 */
@interface NSColor (VuoAppAboutBox)
@end

@implementation NSColor (VuoAppAboutBox)
- (NSString *)cssString
{
	NSColor *srgbColor = [self colorUsingColorSpace:NSColorSpace.sRGBColorSpace];
	return [NSString stringWithFormat:@"rgba(%g,%g,%g,%g)",
									  255 * srgbColor.redComponent,
									  255 * srgbColor.greenComponent,
									  255 * srgbColor.blueComponent,
											srgbColor.alphaComponent];
}
@end

/**
 * A read-only text view that provides its ideal layout size and has some predefined CSS.
 */
@interface VuoAppAboutTextView : NSTextView
@property (retain, nonatomic) NSString *mostRecentText;  ///< The text most recently assigned to this widget; used to repopulate after switching between macOS dark/light mode.
@end

@implementation VuoAppAboutTextView

- (instancetype)init
{
	if (self = [super init])
	{
		self.linkTextAttributes = @{ NSForegroundColorAttributeName:NSColor.secondaryLabelColor };
		self.editable = NO;
	}
	return self;
}

- (NSSize)intrinsicContentSize
{
	[self.layoutManager ensureLayoutForTextContainer:self.textContainer];
	return [self.layoutManager usedRectForTextContainer:self.textContainer].size;
}

- (void)didChangeText
{
	[super didChangeText];
	[self invalidateIntrinsicContentSize];
}

- (void)viewDidChangeEffectiveAppearance
{
	[self setText:self.mostRecentText];
}

- (void)setText:(NSString *)text
{
	self.mostRecentText = text;
	NSMutableString *s = [NSMutableString new];
	[s appendString:@"<meta http-equiv='Content-Type' content='text/html; charset=utf-8'>"];

	NSString *headingCSS    = NSColor.labelColor.cssString;
	NSString *subheadingCSS = NSColor.tertiaryLabelColor.cssString;
	NSString *bodyCSS       = NSColor.secondaryLabelColor.cssString;

	[s appendFormat:@"<style> \
						body { font: 10pt 'Helvetica Neue'; color: %@; } \
						p { padding-top: 30px; } \
						h1 { padding-top: 30px; } \
						.name { font-size: 28pt; font-weight: 300; } \
						.version { color: %@; font-size: 22pt; font-weight: 200; } \
						.copyright { color: %@; } \
						.description { font-weight: bold; } \
						.licenses { color: %@; font-size: 8pt; } \
						pre { font: 'Monaco'; } \
					  </style>",
					headingCSS,
					subheadingCSS,
					subheadingCSS,
					bodyCSS];
	[s appendString:text];
	NSAttributedString *as = [[NSAttributedString alloc] initWithHTML:[s dataUsingEncoding:NSUTF8StringEncoding] documentAttributes:nil];
	[s release];
	self.textStorage.mutableString.string = @"";
	[self.textStorage appendAttributedString:as];
	[as release];
}

@end


/**
 * Dismisses the window when the user presses escape.
 */
@interface VuoAppAboutContentView : NSView
@end

@implementation VuoAppAboutContentView
- (void)cancelOperation:(id)sender
{
	[self.window close];
}
@end


/**
 * The About dialog for VuoCompositionLoader and exported Vuo apps.
 */
@interface VuoAppAboutWindow : NSWindow
@end

@implementation VuoAppAboutWindow

- (instancetype)init
{
	if (self = [super initWithContentRect:NSMakeRect(0, 0, 820, 500)
								styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskFullSizeContentView
								  backing:NSBackingStoreBuffered
									defer:NO])
	{
		self.releasedWhenClosed = NO;
		self.titlebarAppearsTransparent = YES;


		NSBox *leftColumn = [NSBox new];
		leftColumn.translatesAutoresizingMaskIntoConstraints = NO;
		leftColumn.boxType = NSBoxCustom;
		leftColumn.borderWidth = 0;
		leftColumn.fillColor = NSColor.controlBackgroundColor;

		// Contents of left column.
		{
			NSImageView *icon = [NSImageView new];
			icon.translatesAutoresizingMaskIntoConstraints = NO;
			icon.image = NSApplication.sharedApplication.applicationIconImage;
			[leftColumn addSubview:icon];

			VuoAppSplashView *poweredByVuo = [VuoAppSplashView new];
			poweredByVuo.translatesAutoresizingMaskIntoConstraints = NO;
			poweredByVuo.borderVisible = NO;
			[leftColumn addSubview:poweredByVuo];

			NSDictionary *views = NSDictionaryOfVariableBindings(icon, poweredByVuo);
			[NSLayoutConstraint activateConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:|-[icon(160)]-|" options:0 metrics:nil views:views]];
			[NSLayoutConstraint activateConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:|-[poweredByVuo]-|" options:0 metrics:nil views:views]];
			[NSLayoutConstraint activateConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:|-[icon(160)]-(>=0)-[poweredByVuo(120)]-|" options:0 metrics:nil views:views]];

			[poweredByVuo release];
			[icon release];
		}


		NSBox *rightColumn = [NSBox new];
		rightColumn.translatesAutoresizingMaskIntoConstraints = NO;
		rightColumn.boxType = NSBoxCustom;
		rightColumn.borderWidth = 0;
		rightColumn.fillColor = NSColor.windowBackgroundColor;

		// Contents of right column.
		{
			// Description.
			VuoAppAboutTextView *description = [VuoAppAboutTextView new];
			description.frame = NSMakeRect(0, 0, 560, 0);
			description.translatesAutoresizingMaskIntoConstraints = NO;
			description.drawsBackground = NO;
			{
				NSMutableString *s = [NSMutableString new];

				NSString *name = NSBundle.mainBundle.infoDictionary[@"CFBundleName"];
				if (!name)
					name = NSProcessInfo.processInfo.processName;
				[s appendFormat:@"<span class='name'>%@</span>", name];

				NSString *version = NSBundle.mainBundle.infoDictionary[@"CFBundleShortVersionString"];
				if (version.length)
					[s appendFormat:@" <span class='version'>%@</span>", version];

				NSString *copyright = NSBundle.mainBundle.infoDictionary[@"NSHumanReadableCopyright"];
				if (copyright)
					[s appendFormat:@"<p class='copyright'>%@</p>", copyright];

				NSString *desc = NSBundle.mainBundle.infoDictionary[@"description"];
				if (desc)
					[s appendFormat:@"<div class='description'>%@</div><br>", desc];

				NSString *homepage = NSBundle.mainBundle.infoDictionary[@"homepageURL"];
				NSString *documentation = NSBundle.mainBundle.infoDictionary[@"documentationURL"];
				if (homepage)
					[s appendFormat:@"<p><a href='%@'>%@</a></p>", homepage, homepage];
				if (documentation)
					[s appendFormat:@"<p><a href='%@'>%@</a></p>", documentation, documentation];

				[description setText:s];
				[s release];
			}
			[rightColumn addSubview:description];


			// Licenses.
			VuoAppAboutTextView *licenses = [VuoAppAboutTextView new];
			{
				NSMutableString *s = [NSMutableString new];
				[s appendString:@"<div class='licenses'>"];
				[s appendString:[self licenseString]];
				[s appendString:@"</div>"];
				[licenses setText:s];
				[s release];
			}
			NSScrollView *licensesScroller = [[NSScrollView alloc] initWithFrame:NSMakeRect(20, 20, 560, 280)];
			licensesScroller.hasVerticalScroller = YES;
			licensesScroller.documentView = licenses;
			licenses.frame = NSMakeRect(0, 0, licensesScroller.contentSize.width, licensesScroller.contentSize.height);
			[rightColumn addSubview:licensesScroller];


			NSDictionary *views = NSDictionaryOfVariableBindings(description, licensesScroller);
			[NSLayoutConstraint activateConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:|-[description]-|" options:0 metrics:nil views:views]];
			[NSLayoutConstraint activateConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:|-[licensesScroller]-|" options:0 metrics:nil views:views]];
			[NSLayoutConstraint activateConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:|-[description][licensesScroller]-|" options:0 metrics:nil views:views]];

			[licenses release];
			[licensesScroller release];
			[description release];
		}


		VuoAppAboutContentView *c = [VuoAppAboutContentView new];
		self.contentView = c;
		[c addSubview:leftColumn];
		[c addSubview:rightColumn];

		NSDictionary *views = NSDictionaryOfVariableBindings(leftColumn,rightColumn);
		[NSLayoutConstraint activateConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:|[leftColumn][rightColumn]|" options:0 metrics:nil views:views]];
		[NSLayoutConstraint activateConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:|[leftColumn]|" options:0 metrics:nil views:views]];
		[NSLayoutConstraint activateConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:|[rightColumn]|" options:0 metrics:nil views:views]];

		[c release];
		[leftColumn release];
		[rightColumn release];
	}

	return self;
}

/**
 * Builds an attributed string from the contents of each file in the app bundle's "Licenses" folder.
 */
- (NSString *)licenseString
{
	NSMutableString *s = [NSMutableString new];


	[s appendString:@"<p>This app may include software and resources licensed under the following terms:</p>"];


	// Derive the path of "Licenses" directory.
	char licensesPath[PATH_MAX+1];
	licensesPath[0] = 0;
	if (strlen(VuoGetFrameworkPath()))
	{
		strncpy(licensesPath, VuoGetFrameworkPath(), PATH_MAX);
		strncat(licensesPath, "/Vuo.framework/Versions/" VUO_FRAMEWORK_VERSION_STRING "/Documentation/Licenses", PATH_MAX);
	}
	else if (strlen(VuoGetRunnerFrameworkPath()))
	{
		strncpy(licensesPath, VuoGetRunnerFrameworkPath(), PATH_MAX);
		strncat(licensesPath, "/VuoRunner.framework/Versions/" VUO_FRAMEWORK_VERSION_STRING "/Documentation/Licenses", PATH_MAX);
	}


	bool foundLicenses = false;
	DIR *dirp = licensesPath[0] ? opendir(licensesPath) : NULL;
	if (dirp)
	{
		struct dirent *dp;
		while ((dp = readdir(dirp)) != NULL)
		{
			if (dp->d_name[0] == '.')
				continue;

			char *nameWithoutExtension = strdup(dp->d_name);
			nameWithoutExtension[strlen(nameWithoutExtension) - 4] = 0;
			[s appendString:[NSString stringWithFormat:@"<h2>%s</h2>", nameWithoutExtension]];
			free(nameWithoutExtension);

			size_t pathSize = strlen(licensesPath) + dp->d_namlen + 2;
			char licensePath[pathSize];
			strlcpy(licensePath, licensesPath, pathSize);
			strlcat(licensePath, "/", pathSize);
			strlcat(licensePath, dp->d_name, pathSize);

			[s appendString:[NSString stringWithFormat:@"<pre>%@</pre><br>",
				[NSString stringWithContentsOfFile:[NSString stringWithUTF8String:licensePath] usedEncoding:nil error:nil]]];

			foundLicenses = true;
		}
		closedir(dirp);
	}

	if (!foundLicenses)
		[s appendString:@"<p>(No license information found.)</p>"];

	return [s autorelease];
}

@end

/**
 * Stores the About window.
 */
@interface VuoAppAboutBox ()
@property(nonatomic, strong) NSWindow *window; ///< The About window.
@end

@implementation VuoAppAboutBox

/**
 * Displays the About window, pulling information from the app bundle's Info.plist and the Vuo.framework licenses folder.
 */
- (void)displayAboutPanel:(id)sender
{
	if (!_window)
		_window = [VuoAppAboutWindow new];
	[_window center];
	[_window makeKeyAndOrderFront:sender];
}

@end

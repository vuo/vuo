/**
 * @file
 * VuoAppAboutBox implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#import "VuoAppAboutBox.h"

#import "VuoApp.h"

#import <libgen.h>
#import <dirent.h>

@implementation VuoAppAboutBox

/**
 * Loads license data and sends it to the standard "About" panel.
 */
- (void)displayAboutPanel:(id)sender
{
	[NSApp orderFrontStandardAboutPanelWithOptions:[NSDictionary dictionaryWithObject:[self licenseString] forKey:@"Credits"]];
}

/**
 * Builds an attributed string from the contents of each file in the app bundle's "Licenses" folder.
 */
- (NSAttributedString *)licenseString
{
	NSMutableString *ms = [NSMutableString new];
	[ms appendString:@"<meta http-equiv='Content-Type' content='text/html; charset=utf-8'>"];
	[ms appendString:@"<style>body { font: 10pt Helvetica; }  hr { background: #ddd; font-size: 1px; }  pre { font-size: 8pt; }</style>"];

	bool haveTopContent = false;

	// Include the user-specified description, if present.
	NSString *description = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"description"];
	if (description)
	{
		[ms appendString:description];
		haveTopContent = true;
	}

	// Include the homepage and documentation links, if present.
	NSString *homepage = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"homepageURL"];
	NSString *documentation = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"documentationURL"];
	if (homepage || documentation)
	{
		if (homepage)
			[ms appendString:[NSString stringWithFormat:@"<p><a href='%@'>Homepage</a></p>", homepage]];
		if (documentation)
			[ms appendString:[NSString stringWithFormat:@"<p><a href='%@'>Documentation</a></p>", documentation]];
		haveTopContent = true;
	}

	if (haveTopContent)
		[ms appendString:@"<br /><hr /><br /><br />"];

	if (![[[NSBundle mainBundle] bundleIdentifier] isEqualToString:@"org.vuo.VuoCompositionLoader"])
		[ms appendString:[NSString stringWithFormat:@"<p>Created with <a href='http://vuo.org'>Vuo %s</a>.</p>", VUO_VERSION_STRING]];

	[ms appendString:@"<p>This app may include software licensed under the following terms:</p>"];


	// Derive the path of "Licenses" directory.
	char licensesPath[PATH_MAX+1];
	strncpy(licensesPath, VuoApp_getVuoFrameworkPath(), PATH_MAX);
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
			[ms appendString:[NSString stringWithFormat:@"<h2>%s</h2>", nameWithoutExtension]];
			free(nameWithoutExtension);

			char licensePath[strlen(licensesPath) + dp->d_namlen + 2];
			strcpy(licensePath, licensesPath);
			strcat(licensePath, "/");
			strcat(licensePath, dp->d_name);

			[ms appendString:[NSString stringWithFormat:@"<pre>%@</pre><br>",
				[NSString stringWithContentsOfFile:[NSString stringWithUTF8String:licensePath] usedEncoding:nil error:nil]]];

			foundLicenses = true;
		}
		closedir(dirp);
	}

	if (!foundLicenses)
		[ms appendString:@"<p>(No license information found.)</p>"];

	return [[[NSAttributedString new] initWithHTML:[ms dataUsingEncoding:NSUTF8StringEncoding] documentAttributes:nil] autorelease];
}

@end
